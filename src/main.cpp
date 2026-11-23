#include <Arduino.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <time.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include "panel.h"
#include "secrets.h"

// ================== CONFIGURACIÓN ==================
// Pines
#define PIN_SDA     21
#define PIN_SCL     22
#define PIN_DHT      4
#define PIN_BOMBA   26   // relé 1: bomba
#define PIN_LUZ     27   // relé 2: luz

// Muchos módulos de relé activan con LOW. Si el tuyo activa con HIGH, pon false.
#define RELE_ACTIVO_EN_LOW true

// ---- MODO DE PRUEBA (simulación) ----
// true  = ignora la hora real: ciclo de 20 s (luz 10 s encendida / 10 s apagada)
//         y la bomba trabaja 5 s, del segundo 12 al 17 del ciclo.
//         En este modo el panel muestra una semana de datos SIMULADOS (clima y riegos)
//         y no guarda las lecturas reales en el historial.
// false = funcionamiento normal con la hora de internet.
#define MODO_PRUEBA true


// Zona horaria del centro de México (sin horario de verano).
// OFFSET_ZONA_S debe coincidir con ZONA_HORARIA: CST6 = UTC-6 = -21600 s.
const char* ZONA_HORARIA = "CST6";
const int32_t OFFSET_ZONA_S = -6L * 3600;

// Horario de luz
const uint32_t HORA_ENCENDIDO_S = 6UL * 3600;    // 06:00
const uint32_t HORA_APAGADO_S   = 21UL * 3600;   // 21:00
const uint32_t DIA_S            = 24UL * 3600;

// Riego diario: a las 21:10, durante 10 minutos
const uint32_t HORA_RIEGO_S     = 21UL * 3600 + 10UL * 60;  // 21:10
const uint32_t DURACION_RIEGO_S = 10UL * 60;                // 10 min

// Valores de la simulación
const uint32_t INICIO_RIEGO_PRUEBA_S   = 12;
const uint32_t DURACION_RIEGO_PRUEBA_S = 5;

// Seguridad
const uint32_t MARGEN_BOMBA_S = 60;   // la bomba nunca corre más que DURACION + este margen
const uint32_t WDT_TIMEOUT_S  = 20;   // si el programa se congela 20 s, el ESP32 se reinicia

// Cada cuántos segundos se lee el sensor (mínimo 2 s)
const uint32_t INTERVALO_DHT_S  = 2;

// Cada cuánto se reintenta el WiFi si se pierde la conexión
const uint32_t REINTENTO_WIFI_MS = 30000;

// Historial para el panel web (vive en la RAM: se borra al reiniciar)
const uint32_t INTERVALO_HISTORIAL_S = 1800;   // una lectura cada 30 min
const uint16_t HIST_MAX   = 336;               // 7 días
const uint8_t  RIEGOS_MAX = 14;                // últimos 14 riegos
const char*    NOMBRE_MDNS = "invernadero";    // http://invernadero.local
// ===================================================

// Constructor que ya comprobaste que funciona con tu pantalla (clock=SCL, data=SDA)
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);
DHT dht(PIN_DHT, DHT22);   // cambia a DHT11 si tu sensor realmente es DHT11
WebServer server(80);

float temperatura = NAN;
float humedad = NAN;
bool luzActual = false;
bool bombaActual = false;
uint32_t ultimoSegundo = 0xFFFFFFFF;
uint32_t ultimoIntentoWifi = 0;
bool mdnsListo = false;

struct Muestra { uint32_t t; int16_t T; int16_t H; };   // T y H en décimas
struct Riego   { uint32_t t; uint16_t dur; };           // inicio (epoch) y duración (s)
Muestra hist[HIST_MAX];
Riego riegos[RIEGOS_MAX];
uint16_t histN = 0, histSig = 0;
uint8_t riegosN = 0, riegosSig = 0;
uint32_t ultimaMuestra = 0;

const uint8_t NIVEL_APAGADO = RELE_ACTIVO_EN_LOW ? HIGH : LOW;

// "encendido" = bobina activada. Luz y bomba van en COM y NO:
// bobina activada = carga encendida; sin energía o con el chip reiniciando = carga apagada.
void setRele(uint8_t pin, bool encendido) {
  bool nivelActivo = RELE_ACTIVO_EN_LOW ? LOW : HIGH;
  digitalWrite(pin, encendido ? nivelActivo : NIVEL_APAGADO);
}

void iniciarRele(uint8_t pin) {
  digitalWrite(pin, NIVEL_APAGADO);   // fija el nivel antes de configurar como salida
  pinMode(pin, OUTPUT);
  digitalWrite(pin, NIVEL_APAGADO);
}

void formatoReloj(uint32_t seg, char* buf) {
  snprintf(buf, 12, "%02lu:%02lu:%02lu",
           (unsigned long)(seg / 3600),
           (unsigned long)((seg % 3600) / 60),
           (unsigned long)(seg % 60));
}

// Hora UTC en segundos (epoch), o 0 si todavía no hay hora de internet.
uint32_t ahoraEpoch() {
  time_t n = time(nullptr);
  return n > 1700000000L ? (uint32_t)n : 0;
}

// Devuelve false si todavía no hay hora válida de internet.
// Si hay hora: indica si la luz va encendida, cuántos segundos faltan para el cambio
// y qué segundo del día es (para decidir el riego).
bool calcularCiclo(uint32_t& restante, bool& encendida, uint32_t& segDelDia) {
  struct tm ti;
  if (!getLocalTime(&ti, 0)) return false;

  segDelDia = ti.tm_hour * 3600UL + ti.tm_min * 60UL + ti.tm_sec;

  if (segDelDia >= HORA_ENCENDIDO_S && segDelDia < HORA_APAGADO_S) {
    encendida = true;
    restante = HORA_APAGADO_S - segDelDia;
  } else {
    encendida = false;
    restante = (segDelDia >= HORA_APAGADO_S)
                 ? (DIA_S - segDelDia + HORA_ENCENDIDO_S)
                 : (HORA_ENCENDIDO_S - segDelDia);
  }
  return true;
}

// Límite de seguridad: aunque el horario diga que siga, la bomba no corre más de
// duración + margen seguida; queda bloqueada hasta que termine la ventana de riego.
bool bombaConLimite(bool deseada, uint32_t duracionS) {
  static bool activa = false;
  static bool bloqueada = false;
  static uint32_t inicioMs = 0;

  if (!deseada) {
    activa = false;
    bloqueada = false;
    return false;
  }
  if (!activa) {
    activa = true;
    inicioMs = millis();
  }
  if (millis() - inicioMs > (duracionS + MARGEN_BOMBA_S) * 1000UL) bloqueada = true;
  return !bloqueada;
}

// ---------- Historial para el panel ----------
// Guarda la lectura actual cada 30 min (y una al arrancar, en cuanto hay hora y sensor).
void registrarMuestra() {
  uint32_t n = ahoraEpoch();
  if (!n || isnan(temperatura) || isnan(humedad)) return;
  if (ultimaMuestra && n / INTERVALO_HISTORIAL_S == ultimaMuestra / INTERVALO_HISTORIAL_S) return;

  ultimaMuestra = n;
  hist[histSig] = { n, (int16_t)lroundf(temperatura * 10), (int16_t)lroundf(humedad * 10) };
  histSig = (histSig + 1) % HIST_MAX;
  if (histN < HIST_MAX) histN++;
}

// Guarda cada riego terminado: cuándo empezó y cuántos segundos duró la bomba encendida.
void registrarRiego(bool bomba) {
  static bool previa = false;
  static uint32_t inicioEpoch = 0, inicioMs = 0;

  if (bomba && !previa) {
    inicioEpoch = ahoraEpoch();
    inicioMs = millis();
  }
  if (!bomba && previa) {
    uint32_t dur = (millis() - inicioMs) / 1000UL;
    if (inicioEpoch && !MODO_PRUEBA && dur > 0) {
      riegos[riegosSig] = { inicioEpoch, (uint16_t)(dur > 65535UL ? 65535UL : dur) };
      riegosSig = (riegosSig + 1) % RIEGOS_MAX;
      if (riegosN < RIEGOS_MAX) riegosN++;
    }
  }
  previa = bomba;
}

// ---------- Servidor web ----------
void jsonNum(String& j, float v) {
  if (isnan(v)) j += "null";
  else j += String(v, 1);
}

#if MODO_PRUEBA
// Datos simulados: una semana de clima y riegos para ver el panel completo.
static float simRuido(uint32_t i) {
  uint32_t x = i * 2654435761u;
  x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
  return (x & 0xFFFF) / 65535.0f - 0.5f;   // -0.5 a 0.5
}

static void simClima(uint32_t t, float& T, float& H) {
  int64_t tl = (int64_t)t + OFFSET_ZONA_S;
  float hr = (tl % 86400) / 3600.0f;
  uint32_t dia = (uint32_t)(tl / 86400);
  float g = expf(-powf((hr - 15.0f) / 5.4f, 2.0f));
  T = 15.5f + 1.6f * sinf(dia * 1.1f) + 11.5f * g + ((hr >= 6.0f && hr < 21.0f) ? 1.2f : 0.0f)
      + simRuido(t / 1800) * 1.2f;
  H = 80.0f - 33.0f * g + simRuido(t / 1800 + 7777) * 8.0f;
  if (H < 30.0f) H = 30.0f;
  if (H > 96.0f) H = 96.0f;
}
#endif

void manejarDatos() {
  uint32_t n = ahoraEpoch();
#if MODO_PRUEBA
  if (!n) n = 1794200000UL + millis() / 1000UL;   // sin internet: hora inventada para la demo
#endif
  String j;
  j.reserve(histN * 26 + riegosN * 22 + 400);

  j += "{\"ok\":";    j += n ? "true" : "false";
  j += ",\"t\":";     j += n;
  j += ",\"off\":";   j += OFFSET_ZONA_S;
  j += ",\"test\":";  j += MODO_PRUEBA ? "true" : "false";
  float tAct = temperatura, hAct = humedad;
#if MODO_PRUEBA
  if (isnan(tAct) || isnan(hAct)) simClima(n, tAct, hAct);   // sin sensor: valor simulado
#endif
  j += ",\"temp\":";  jsonNum(j, tAct);
  j += ",\"hum\":";   jsonNum(j, hAct);
  j += ",\"luz\":";   j += luzActual ? "true" : "false";
  j += ",\"bomba\":"; j += bombaActual ? "true" : "false";
  j += ",\"cfg\":{\"on\":";   j += HORA_ENCENDIDO_S;
  j += ",\"off\":";           j += HORA_APAGADO_S;
  j += ",\"riego\":";         j += HORA_RIEGO_S;
  j += ",\"dur\":";           j += DURACION_RIEGO_S;
  j += "}";

#if MODO_PRUEBA
  // Historial simulado: 7 días, una lectura cada 30 min, terminando ahora
  j += ",\"h\":[";
  uint32_t ult = (n / INTERVALO_HISTORIAL_S) * INTERVALO_HISTORIAL_S;
  for (uint16_t i = 0; i < HIST_MAX; i++) {
    uint32_t t = ult - (uint32_t)(HIST_MAX - 1 - i) * INTERVALO_HISTORIAL_S;
    float T, H;
    simClima(t, T, H);
    if (i) j += ',';
    j += '['; j += t; j += ','; j += (int)lroundf(T * 10); j += ','; j += (int)lroundf(H * 10); j += ']';
  }
  // Riegos simulados: uno por día a la hora configurada, los que ya terminaron
  j += "],\"r\":[";
  int64_t hoyL = ((int64_t)n + OFFSET_ZONA_S) / 86400 * 86400;
  bool primero = true;
  for (int d = 6; d >= 0; d--) {
    uint32_t dur = DURACION_RIEGO_S - (d % 2) * 30;
    uint32_t t = (uint32_t)(hoyL - (int64_t)d * 86400 + HORA_RIEGO_S - OFFSET_ZONA_S);
    if (t + dur > n) continue;
    if (!primero) j += ',';
    primero = false;
    j += '['; j += t; j += ','; j += dur; j += ']';
  }
#else
  j += ",\"h\":[";
  uint16_t iniH = histN < HIST_MAX ? 0 : histSig;
  for (uint16_t i = 0; i < histN; i++) {
    const Muestra& m = hist[(iniH + i) % HIST_MAX];
    if (i) j += ',';
    j += '['; j += m.t; j += ','; j += m.T; j += ','; j += m.H; j += ']';
  }
  j += "],\"r\":[";
  uint8_t iniR = riegosN < RIEGOS_MAX ? 0 : riegosSig;
  for (uint8_t i = 0; i < riegosN; i++) {
    const Riego& r = riegos[(iniR + i) % RIEGOS_MAX];
    if (i) j += ',';
    j += '['; j += r.t; j += ','; j += r.dur; j += ']';
  }
#endif
  j += "]}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void manejarPanel() {
  server.send_P(200, "text/html; charset=utf-8", PANEL_HTML);
}

void manejarNoEncontrado() {
  server.send(404, "text/plain", "No encontrado");
}

// ---------- Pantalla ----------
// Izquierda: estado y contador. Derecha: temperatura y humedad.
void dibujar(uint32_t restante, bool encendida, bool horaOk, bool bomba) {
  char reloj[12];
  char linea[16];

  u8g2.clearBuffer();

  // Lado izquierdo
  u8g2.setFont(u8g2_font_6x10_tf);
  if (horaOk) {
    formatoReloj(restante, reloj);
    // Fuente más angosta (5 px por letra) para que quepa "LUZ OFF RI OFF" antes de la línea
    u8g2.setFont(u8g2_font_5x8_tf);
    snprintf(linea, sizeof(linea), "%s %s",
             encendida ? "LUZ ON" : "LUZ OFF",
             bomba ? "RI ON" : "RI OFF");
    u8g2.drawStr(0, 8, linea);
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.drawStr(0, 30, reloj);
  } else {
    u8g2.drawStr(0, 9, "SIN HORA");
    u8g2.drawStr(0, 22, "Conectando");
    u8g2.drawStr(0, 31, "a WiFi...");
  }

  // Línea divisoria
  u8g2.drawVLine(78, 0, 32);

  // Lado derecho
  if (isnan(temperatura)) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(84, 14, "Error");
    u8g2.drawStr(84, 26, "DHT");
  } else {
    u8g2.setFont(u8g2_font_helvB12_tf);
    snprintf(linea, sizeof(linea), "%dC", (int)roundf(temperatura));
    u8g2.drawStr(82, 16, linea);

    u8g2.setFont(u8g2_font_6x10_tf);
    if (isnan(humedad)) {
      u8g2.drawStr(82, 29, "HR --");
    } else {
      snprintf(linea, sizeof(linea), "HR %d%%", (int)humedad);
      u8g2.drawStr(82, 29, linea);
    }
  }
  u8g2.sendBuffer();
}

void leerDHT() {
  temperatura = dht.readTemperature();   // queda en NAN si falla la lectura
  humedad = dht.readHumidity();
  Serial.printf("DHT -> temp: %.2f C, humedad: %.2f %%\n", temperatura, humedad);
}

void iniciarWatchdog() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = WDT_TIMEOUT_S * 1000;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_task_wdt_deinit();
  esp_task_wdt_init(&cfg);
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);
}

void setup() {
  Serial.begin(115200);

  // Luz y bomba apagadas desde el primer instante
  iniciarRele(PIN_LUZ);
  iniciarRele(PIN_BOMBA);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "Conectando WiFi...");
  u8g2.sendBuffer();

  dht.begin();

  // Hora por internet: se configura una vez; el ESP32 la sincroniza solo
  // en cuanto haya WiFi y la vuelve a sincronizar periódicamente.
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);   // respuestas más rápidas del panel
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  configTzTime(ZONA_HORARIA, "pool.ntp.org", "time.google.com");

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(250);
  bool conectado = WiFi.status() == WL_CONNECTED;
  Serial.println(conectado ? "WiFi conectado" : "WiFi no conectado");

  // Servidor del panel
  server.on("/", HTTP_GET, manejarPanel);
  server.on("/datos", HTTP_GET, manejarDatos);
  server.onNotFound(manejarNoEncontrado);
  server.begin();

  if (conectado) {
    String ip = WiFi.localIP().toString();
    Serial.println("Panel en http://" + ip + "/");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "Panel en:");
    u8g2.drawStr(0, 24, ip.c_str());
    u8g2.sendBuffer();
    delay(4000);
  }

  struct tm ti;
  getLocalTime(&ti, 8000);   // espera hasta 8 s la primera sincronización
  leerDHT();

  iniciarWatchdog();   // se activa al final, cuando ya terminaron las esperas largas
}

void loop() {
  esp_task_wdt_reset();   // "sigo vivo"; si el programa se congela, el ESP32 se reinicia

  // Reintenta el WiFi si se cayó
  if (WiFi.status() != WL_CONNECTED && millis() - ultimoIntentoWifi > REINTENTO_WIFI_MS) {
    ultimoIntentoWifi = millis();
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  // Nombre http://invernadero.local en cuanto haya WiFi
  if (!mdnsListo && WiFi.status() == WL_CONNECTED) {
    mdnsListo = MDNS.begin(NOMBRE_MDNS);
    if (mdnsListo) MDNS.addService("http", "tcp", 80);
  }

  server.handleClient();

  uint32_t restante = 0;
  uint32_t segCiclo = 0;
  bool encendida = false;
  uint32_t inicioRiego, duracionRiego;

#if MODO_PRUEBA
  // Ciclo rápido de 20 s: 10 s encendida, 10 s apagada
  uint32_t sPrueba = (millis() / 1000UL) % 20;
  bool horaOk = true;
  encendida = sPrueba < 10;
  restante = encendida ? (10 - sPrueba) : (20 - sPrueba);
  segCiclo = sPrueba;
  inicioRiego = INICIO_RIEGO_PRUEBA_S;
  duracionRiego = DURACION_RIEGO_PRUEBA_S;
#else
  bool horaOk = calcularCiclo(restante, encendida, segCiclo);
  inicioRiego = HORA_RIEGO_S;
  duracionRiego = DURACION_RIEGO_S;
#endif

  // Sin hora válida, luz y bomba se mantienen apagadas (no se adivina el horario)
  if (!horaOk) encendida = false;

  // Riego: se calcula con el reloj, así un reinicio no lo repite ni lo extiende
  bool bombaDeseada = horaOk && segCiclo >= inicioRiego && segCiclo < inicioRiego + duracionRiego;
  bool bomba = bombaConLimite(bombaDeseada, duracionRiego);

  setRele(PIN_LUZ, encendida);
  setRele(PIN_BOMBA, bomba);
  luzActual = encendida;
  bombaActual = bomba;
  registrarRiego(bomba);

  // Redibuja una vez por segundo; el sensor se lee cada INTERVALO_DHT_S
  uint32_t seg = millis() / 1000UL;
  if (seg != ultimoSegundo) {
    ultimoSegundo = seg;
    if (seg % INTERVALO_DHT_S == 0) leerDHT();
    registrarMuestra();
    dibujar(restante, encendida, horaOk, bomba);
  }

  delay(20);
}