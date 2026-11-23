# Sistema de Riego e Iluminación con ESP32

Sistema de control automático de **luz** y **riego** para un invernadero, basado en un ESP32 y programado con PlatformIO (framework Arduino). Mide temperatura y humedad, controla una lámpara y una bomba de agua con relevadores, muestra el estado en una pantalla OLED y publica un **panel web** para ver el historial desde cualquier dispositivo de la misma red.

## Cómo funciona

### Horario automático
La hora se obtiene por internet (NTP) con la zona horaria del centro de México (CST, UTC-6). Con esa hora, el ESP32 decide:

| Función | Horario |
|---|---|
| Luz encendida | 06:00 a 21:00 |
| Riego (bomba) | Todos los días a las 21:10, durante 10 minutos |

Los horarios y la duración se cambian en las constantes de configuración al inicio de `src/main.cpp`.

### Pantalla OLED (128x32, I2C)
- **Izquierda:** estado de la luz y del riego, y un contador con el tiempo que falta para el siguiente cambio de la luz.
- **Derecha:** temperatura y humedad relativa actuales.
- Al arrancar muestra la IP donde está el panel web.

### Panel web
El ESP32 sirve el panel desde su propia IP, sin servicios externos. Con el celular o la PC en la misma red WiFi, abre:

- `http://invernadero.local`, o
- `http://<IP del ESP32>/` (la IP aparece en la OLED y en el monitor serie).

El panel muestra temperatura y humedad (24 h y 7 días), mapa de calor de temperatura, horario de la luz, riegos registrados y agua estimada según el caudal de la bomba. Los datos se piden a `/datos` (JSON) cada 15 segundos.

> El historial se guarda en la **RAM** del ESP32: una lectura cada 30 minutos (hasta 7 días) y los últimos 14 riegos. Se borra al reiniciar o cortar la corriente. Esto se resuelve en la siguiente etapa (base de datos).

### Seguridad
- Luz y bomba arrancan **apagadas**; con el chip sin energía o reiniciando, quedan apagadas.
- **Sin hora válida de internet**, luz y bomba permanecen apagadas (no se adivina el horario).
- **Límite de la bomba:** nunca corre más que la duración del riego más 60 s de margen.
- **Watchdog:** si el programa se congela 20 s, el ESP32 se reinicia.
- Reintento automático de WiFi cada 30 s si se pierde la conexión.

### Modo de prueba
Con `#define MODO_PRUEBA true` se ignora la hora real: la luz cambia cada 10 s, la bomba trabaja 5 s por ciclo de 20 s y el panel muestra una semana de datos **simulados**. Para el funcionamiento normal, cambiarlo a `false`.

## Hardware

| Componente | Pin del ESP32 |
|---|---|
| OLED 128x32 (I2C) SDA / SCL | 21 / 22 |
| Sensor de temperatura y humedad (DHT) | 4 |
| Relevador 1: bomba | 26 |
| Relevador 2: luz | 27 |

- La bomba y la lámpara son cargas de corriente alterna, cada una con su propio relevador.
- Los relevadores se configuran con `RELE_ACTIVO_EN_LOW` (muchos módulos activan con nivel bajo).
- Si tu sensor es DHT11 en lugar de DHT22, cámbialo en la línea `DHT dht(PIN_DHT, DHT22);`.

## Instalación

1. Instala [VS Code](https://code.visualstudio.com/) con la extensión **PlatformIO IDE**.
2. Clona el repositorio y ábrelo en VS Code.
3. Crea `include/secrets.h` con tus credenciales de WiFi (el ESP32 solo usa redes de **2.4 GHz**). Este archivo no se sube a Git:
   ```cpp
   #pragma once
   #define WIFI_SSID "NOMBRE_DE_TU_WIFI"
   #define WIFI_PASS "CONTRASEÑA_DE_TU_WIFI"
   ```
4. Compila y sube el código: `pio run -t upload`.
5. Abre el monitor serie (115200 baudios) para ver la IP y el estado.

## Estructura del proyecto

```
├── include/
│   └── secrets.h      # credenciales (no versionado)
├── src/
│   ├── main.cpp       # lógica de luz, riego, sensor, pantalla y servidor web
│   └── panel.h        # HTML del panel web
└── platformio.ini
```

## Próximas actualizaciones

- [ ] **Base de datos:** guardar todos los datos (temperatura, humedad, estado de la luz y riegos) para no perder el historial al reiniciar el ESP32.
- [ ] **ESP-NOW:** que los datos de los sensores lleguen al sistema por ESP-NOW, para usar varios nodos sin depender del WiFi.
- [ ] **Agente de IA:** un agente que revise los datos y envíe notificaciones sobre cómo está todo (estado del invernadero, alertas y resúmenes).
