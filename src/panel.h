#pragma once
#include <Arduino.h>

// Panel web que sirve el ESP32 en http://<IP>/  (los datos llegan de /datos)
const char PANEL_HTML[] PROGMEM = R"rawliteral(<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><style>body{margin:0}</style>
<title>Panel de Invernadero</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=DM+Mono:wght@400;500&family=Manrope:wght@400;500;600;700&display=swap">
<style>
/* Layout: marco oscuro de una página. Barra superior con pastilla de navegación, título grande,
   fila de cuatro indicadores y una cuadrícula de 12 columnas con tarjetas redondeadas.
   Un color por magnitud: agua azul, temperatura naranja, humedad aqua, luz ámbar. */
:root {
  color-scheme: dark;
  --bg: #0b0c0c;
  --surface: #151716;
  --surface-2: #1d201f;
  --line: rgba(255,255,255,.08);
  --line-2: rgba(255,255,255,.15);
  --fg: #eceee9;
  --fg-2: #b7bbb3;
  --muted: #8a8f86;
  --accent: #b8cfd4;
  --accent-ink: #0f1a1d;
  --c-water: #3987e5;
  --c-temp: #d95926;
  --c-hum: #199e70;
  --c-light: #c98500;
  --heat-0: #27211e;
  --heat-1: #e5703f;
  --ok-ink: #0ca30c;
  --shadow: 0 8px 24px rgba(0,0,0,.45);
  --font: "Manrope", system-ui, -apple-system, "Segoe UI", sans-serif;
  --mono: "DM Mono", ui-monospace, "SFMono-Regular", Menlo, monospace;
}
@media (prefers-color-scheme: light) {
  :root:not([data-theme="dark"]) {
    color-scheme: light;
    --bg: #eceee9;
    --surface: #ffffff;
    --surface-2: #f1f3ef;
    --line: rgba(14,18,14,.09);
    --line-2: rgba(14,18,14,.18);
    --fg: #121412;
    --fg-2: #4b504a;
    --muted: #6a6f67;
    --accent: #1c2b2e;
    --accent-ink: #ffffff;
    --c-water: #2a78d6;
    --c-temp: #eb6834;
    --c-hum: #1baf7a;
    --c-light: #eda100;
    --heat-0: #f4e8e1;
    --heat-1: #d9531f;
    --ok-ink: #006300;
    --shadow: 0 8px 24px rgba(20,30,20,.14);
  }
}
:root[data-theme="light"] {
  color-scheme: light;
  --bg: #eceee9;
  --surface: #ffffff;
  --surface-2: #f1f3ef;
  --line: rgba(14,18,14,.09);
  --line-2: rgba(14,18,14,.18);
  --fg: #121412;
  --fg-2: #4b504a;
  --muted: #6a6f67;
  --accent: #1c2b2e;
  --accent-ink: #ffffff;
  --c-water: #2a78d6;
  --c-temp: #eb6834;
  --c-hum: #1baf7a;
  --c-light: #eda100;
  --heat-0: #f4e8e1;
  --heat-1: #d9531f;
  --ok-ink: #006300;
  --shadow: 0 8px 24px rgba(20,30,20,.14);
}
*, *::before, *::after { box-sizing: border-box; }
html { scroll-behavior: smooth; }
@media (prefers-reduced-motion: reduce) { html { scroll-behavior: auto; } }
body {
  background: var(--bg);
  color: var(--fg);
  font: 14px/1.5 var(--font);
  padding-left: max(16px, env(safe-area-inset-left, 0px));
  padding-right: max(16px, env(safe-area-inset-right, 0px));
  padding-block: 20px 56px;
}
:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
.wrap { max-width: 1240px; margin: 0 auto; display: flex; flex-direction: column; gap: 20px; }
h1, h2, p { margin: 0; }
section[id] { scroll-margin-top: 16px; }

/* barra superior */
.top { display: flex; align-items: center; justify-content: space-between; gap: 12px; flex-wrap: wrap; }
.brand { display: flex; align-items: center; gap: 10px; font-weight: 700; font-size: 18px; color: var(--fg); text-decoration: none; letter-spacing: -.01em; }
.brand svg { width: 28px; height: 28px; stroke: var(--c-hum); fill: none; stroke-width: 1.7; stroke-linecap: round; stroke-linejoin: round; }
.pill { display: flex; gap: 4px; padding: 5px; border-radius: 999px; background: var(--surface); border: 1px solid var(--line); overflow-x: auto; max-width: 100%; }
.pill a { padding: 8px 16px; border-radius: 999px; color: var(--fg-2); text-decoration: none; white-space: nowrap; font-weight: 500; }
.pill a:hover { color: var(--fg); }
.pill a.on { background: var(--accent); color: var(--accent-ink); }
.chip { display: inline-flex; align-items: center; gap: 8px; padding: 7px 14px; border: 1px solid var(--line-2); border-radius: 999px; font-size: 12.5px; color: var(--fg-2); white-space: nowrap; }
.chip i { width: 8px; height: 8px; border-radius: 50%; background: var(--c-light); }
@media (max-width: 820px) { .pill { order: 3; width: 100%; } }

/* título */
.hero { padding-top: 12px; display: flex; flex-direction: column; gap: 6px; }
.eyebrow { font-size: 12px; letter-spacing: .12em; text-transform: uppercase; color: var(--muted); font-weight: 600; }
h1 { font-size: clamp(34px, 6vw, 60px); line-height: 1.04; font-weight: 700; letter-spacing: -.03em; text-wrap: balance; }
.range { color: var(--fg-2); font-size: 15px; }
.note { display: flex; gap: 10px; align-items: flex-start; padding: 12px 16px; border: 1px dashed var(--line-2); border-radius: 16px; color: var(--fg-2); }
.note svg { flex: none; width: 18px; height: 18px; margin-top: 2px; stroke: var(--fg-2); fill: none; stroke-width: 1.7; stroke-linecap: round; }

/* indicadores */
.kpis { display: grid; grid-template-columns: repeat(auto-fit, minmax(230px, 1fr)); gap: 12px; }
.tile { display: flex; align-items: center; gap: 14px; padding: 16px; background: var(--surface); border: 1px solid var(--line); border-radius: 22px; min-width: 0; }
.ti { flex: none; width: 54px; height: 54px; display: grid; place-items: center; border-radius: 17px; background: var(--surface-2); }
.ti svg { width: 26px; height: 26px; fill: none; stroke-width: 1.7; stroke-linecap: round; stroke-linejoin: round; }
.tx { min-width: 0; display: flex; flex-direction: column; }
.tl { font-size: 12.5px; color: var(--muted); }
.tv { font-size: 30px; line-height: 1.15; font-weight: 600; letter-spacing: -.02em; }
.tv small { font-size: 15px; color: var(--fg-2); font-weight: 500; letter-spacing: 0; }
.td { font-size: 12px; color: var(--fg-2); }

/* tarjetas */
.grid { display: grid; grid-template-columns: repeat(12, minmax(0, 1fr)); gap: 16px; }
.card { background: var(--surface); border: 1px solid var(--line); border-radius: 26px; padding: 20px; display: flex; flex-direction: column; gap: 18px; min-width: 0; }
#clima { grid-column: span 8; } #luz { grid-column: span 4; }
#mapa { grid-column: span 8; } #riego { grid-column: span 4; }
#datos { grid-column: span 12; }
@media (max-width: 980px) { .grid > * { grid-column: 1 / -1 !important; } }
.ch { display: flex; align-items: center; justify-content: space-between; gap: 12px; flex-wrap: wrap; }
.chl { display: flex; align-items: center; gap: 12px; min-width: 0; }
.ci { flex: none; width: 42px; height: 42px; border-radius: 50%; background: var(--surface-2); display: grid; place-items: center; }
.ci svg { width: 21px; height: 21px; fill: none; stroke-width: 1.7; stroke-linecap: round; stroke-linejoin: round; }
.ch h2 { font-size: 17px; font-weight: 600; letter-spacing: -.01em; }
.ch p { font-size: 12.5px; color: var(--muted); }
.seg { display: flex; gap: 2px; padding: 3px; border: 1px solid var(--line); border-radius: 999px; }
.seg button { font: inherit; background: none; border: 0; color: var(--fg-2); padding: 6px 14px; border-radius: 999px; cursor: pointer; }
.seg button[aria-pressed="true"] { background: var(--surface-2); color: var(--fg); box-shadow: inset 0 0 0 1px var(--line-2); }

/* gráficas de línea */
.chart-t { display: flex; align-items: center; gap: 8px; font-weight: 600; font-size: 13.5px; margin-bottom: 4px; }
.sw { width: 10px; height: 10px; border-radius: 3px; flex: none; }
.chart-t span:last-child { color: var(--muted); font-weight: 400; }
.hostc { min-width: 0; width: 100%; }
svg.line { display: block; max-width: 100%; height: auto; touch-action: pan-y; }
.gl { stroke: var(--line); stroke-width: 1; }
.sep { stroke: var(--line-2); stroke-dasharray: 3 4; }
.ln { fill: none; stroke-width: 2; stroke-linejoin: round; stroke-linecap: round; }
.yt, .xt { font: 11px var(--mono); fill: var(--muted); }
.end { font: 600 12.5px var(--font); fill: var(--fg); }
.dot, .hd { stroke: var(--surface); stroke-width: 2.5; }
.cx { stroke: var(--fg-2); stroke-dasharray: 3 3; }
.hit { fill: transparent; }
.two { display: flex; flex-direction: column; gap: 20px; }

/* luz */
.big { display: flex; align-items: baseline; gap: 10px; flex-wrap: wrap; }
.big b { font-size: 40px; line-height: 1; font-weight: 600; letter-spacing: -.03em; }
.big span { color: var(--fg-2); }
.legend { display: flex; gap: 16px; flex-wrap: wrap; color: var(--fg-2); font-size: 12.5px; }
.legend span { display: inline-flex; align-items: center; gap: 7px; }
.gantt { display: flex; flex-direction: column; gap: 10px; }
.gr { display: grid; grid-template-columns: 46px minmax(0, 1fr) 36px; align-items: center; gap: 10px; }
.gd { font-size: 12.5px; color: var(--fg-2); }
.gv { font: 12px var(--mono); color: var(--fg-2); text-align: right; }
.gt { position: relative; height: 16px; border-radius: 8px; background: var(--surface-2); }
.gb { position: absolute; top: 0; bottom: 0; border-radius: 8px; background: var(--c-light); }
.gp { position: absolute; top: -3px; bottom: -3px; width: 6px; border-radius: 3px; background: var(--c-water); box-shadow: 0 0 0 2px var(--surface); }
.ga { position: relative; height: 16px; margin-left: 56px; margin-right: 46px; }
.ga span { position: absolute; top: 0; font: 11px var(--mono); color: var(--muted); transform: translateX(-50%); }
.ga span:first-child { transform: none; }
.ga span:last-child { transform: translateX(-100%); }

/* agua */
.ctl { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; color: var(--fg-2); font-size: 12.5px; }
.ctl input { width: 76px; font: 500 13px var(--mono); color: var(--fg); background: var(--surface-2); border: 1px solid var(--line-2); border-radius: 10px; padding: 6px 10px; }
.bars { display: grid; grid-template-columns: repeat(7, minmax(0, 1fr)); gap: 8px; height: 156px; align-items: end; border-bottom: 1px solid var(--line-2); }
.bc { display: flex; flex-direction: column; align-items: center; justify-content: flex-end; gap: 5px; height: 100%; position: relative; }
.bv { font: 12px var(--mono); color: var(--fg-2); }
.bar { width: 100%; max-width: 30px; border-radius: 7px 7px 2px 2px; background: var(--c-water); }
.bl { display: grid; grid-template-columns: repeat(7, minmax(0, 1fr)); gap: 8px; margin-top: -10px; text-align: center; font-size: 12px; color: var(--muted); }
.bl .today { color: var(--fg); font-weight: 600; }
.log { list-style: none; margin: 0; padding: 0; display: flex; flex-direction: column; gap: 10px; }
.log li { display: grid; grid-template-columns: 1fr auto; gap: 2px 12px; padding: 12px 14px; background: var(--surface-2); border-radius: 16px; }
.log b { font-weight: 600; }
.log small { color: var(--fg-2); font-size: 12.5px; }
.ok { display: inline-flex; align-items: center; gap: 6px; color: var(--ok-ink); font-size: 12.5px; font-weight: 600; grid-row: 1 / span 2; grid-column: 2; align-self: center; }
.ok svg { width: 15px; height: 15px; fill: none; stroke: currentColor; stroke-width: 2.4; stroke-linecap: round; stroke-linejoin: round; }

/* mapa de calor */
.heat { display: grid; grid-template-columns: 38px repeat(24, minmax(0, 1fr)); gap: 2px; align-items: center; }
.hl { font: 11px var(--mono); color: var(--muted); white-space: nowrap; }
.hc { height: 24px; border-radius: 4px; }
.hc.na { box-shadow: inset 0 0 0 1px var(--line); }
@media (max-width: 560px) { .hl.s3 { color: transparent; } .hc { height: 20px; } }
.hlg { display: flex; align-items: center; gap: 10px; font: 11px var(--mono); color: var(--muted); }
.hg { flex: 1; max-width: 220px; height: 8px; border-radius: 4px; background: linear-gradient(90deg, var(--heat-0), var(--heat-1)); }

/* tabla */
.tw { overflow-x: auto; }
table { width: 100%; border-collapse: collapse; min-width: 640px; }
th, td { padding: 12px 14px; text-align: right; font-variant-numeric: tabular-nums; border-bottom: 1px solid var(--line); white-space: nowrap; }
th { font-weight: 500; color: var(--muted); font-size: 12.5px; }
th:first-child, td:first-child { text-align: left; }
tbody tr:hover { background: var(--surface-2); }
tbody tr:last-child td { border-bottom: 0; }

/* globo de datos */
#tip { position: fixed; z-index: 50; pointer-events: none; max-width: 260px; background: var(--surface-2); border: 1px solid var(--line-2); border-radius: 12px; padding: 8px 11px; font-size: 12px; line-height: 1.45; box-shadow: var(--shadow); color: var(--fg); }
#tip b { display: block; font-weight: 600; }
#tip span { color: var(--fg-2); }
.empty{color:var(--muted);padding:28px 0;text-align:center}
#live i{background:var(--c-hum)}
#live.off i{background:var(--c-light)}
</style>

</head><body>
<div class="wrap">
  <header class="top">
    <a class="brand" href="#resumen">
      <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 20V11.5L12 4l9 7.5V20z"/><path d="M3 20h18M12 20v-7M7.5 20v-4M16.5 20v-4"/></svg>
      invernadero
    </a>
    <nav class="pill" aria-label="Secciones">
      <a href="#resumen" class="on">Resumen</a>
      <a href="#clima">Clima</a>
      <a href="#luz">Luz</a>
      <a href="#riego">Agua</a>
      <a href="#datos">Datos</a>
    </nav>
    <span class="chip" id="live"><i></i>Conectando…</span>
  </header>

  <section id="resumen" class="hero">
    <p class="eyebrow">Invernadero · ESP32</p>
    <h1>Riego e iluminación</h1>
    <p class="range" id="rng">Conectando con el ESP32…</p>
  </section>

  <p class="note">
    <svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="9"/><path d="M12 11v5M12 7.8v.2"/></svg>
    <span>Las lecturas viven en la memoria del ESP32: guarda una cada 30 minutos y el historial se borra si lo reinicias o le quitas la corriente.</span>
  </p>

  <section class="kpis" aria-label="Estado actual">
    <div class="tile">
      <div class="ti"><svg viewBox="0 0 24 24" style="stroke:var(--c-temp)"><path d="M10 14.5V5a2 2 0 0 1 4 0v9.5a4 4 0 1 1-4 0z"/><path d="M12 9v7"/></svg></div>
      <div class="tx"><span class="tl">Temperatura</span><span class="tv" id="kT"></span><span class="td" id="kTd"></span></div>
    </div>
    <div class="tile">
      <div class="ti"><svg viewBox="0 0 24 24" style="stroke:var(--c-hum)"><path d="M12 3c3.2 4.3 5.8 7.2 5.8 10.3a5.8 5.8 0 0 1-11.6 0C6.2 10.200 8.800 7.300 12 3z"/></svg></div>
      <div class="tx"><span class="tl">Humedad del aire</span><span class="tv" id="kH"></span><span class="td" id="kHd"></span></div>
    </div>
    <div class="tile">
      <div class="ti"><svg viewBox="0 0 24 24" style="stroke:var(--c-light)"><circle cx="12" cy="12" r="4"/><path d="M12 2.500v2.500M12 19v2.500M2.500 12H5M19 12h2.500M5.300 5.300l1.800 1.800M16.900 16.900l1.800 1.800M5.300 18.700l1.800-1.800M16.900 7.100l1.800-1.800"/></svg></div>
      <div class="tx"><span class="tl">Luz</span><span class="tv" id="kL">—</span><span class="td" id="kLd"></span></div>
    </div>
    <div class="tile">
      <div class="ti"><svg viewBox="0 0 24 24" style="stroke:var(--c-water)"><path d="M3 9c2-2 4-2 6 0s4 2 6 0 4-2 6 0"/><path d="M3 15c2-2 4-2 6 0s4 2 6 0 4-2 6 0"/></svg></div>
      <div class="tx"><span class="tl">Último riego</span><span class="tv" id="kW"></span><span class="td" id="kWd"></span></div>
    </div>
  </section>

  <main class="grid">
    <section class="card" id="clima">
      <div class="ch">
        <div class="chl">
          <div class="ci"><svg viewBox="0 0 24 24" style="stroke:var(--fg-2)"><path d="M4 18l5-6 4 3 7-9"/><path d="M4 21h16"/></svg></div>
          <div><h2>Temperatura y humedad</h2><p>Sensor ambiental, una lectura cada 30 min</p></div>
        </div>
        <div class="seg" role="group" aria-label="Rango de tiempo">
          <button type="button" id="r24" aria-pressed="true">24 h</button>
          <button type="button" id="r7" aria-pressed="false">7 días</button>
        </div>
      </div>
      <div class="two">
        <div>
          <div class="chart-t"><span class="sw" style="background:var(--c-temp)"></span><span>Temperatura</span><span>°C</span></div>
          <div class="hostc" id="chT"></div>
        </div>
        <div>
          <div class="chart-t"><span class="sw" style="background:var(--c-hum)"></span><span>Humedad relativa</span><span>%</span></div>
          <div class="hostc" id="chH"></div>
        </div>
      </div>
    </section>

    <section class="card" id="luz">
      <div class="ch">
        <div class="chl">
          <div class="ci"><svg viewBox="0 0 24 24" style="stroke:var(--c-light)"><circle cx="12" cy="12" r="4"/><path d="M12 2.500v2.500M12 19v2.500M2.500 12H5M19 12h2.500M5.300 5.300l1.800 1.800M16.900 16.900l1.800 1.800M5.300 18.700l1.800-1.800M16.900 7.100l1.800-1.800"/></svg></div>
          <div><h2>Luz</h2><p>Horario de la lámpara y riegos registrados</p></div>
        </div>
      </div>
      <div class="big"><b id="lzB">—</b><span id="lzS"></span></div>
      <div class="legend">
        <span><i class="sw" style="background:var(--c-light)"></i>Lámpara encendida</span>
        <span><i class="sw" style="background:var(--c-water)"></i>Riego</span>
      </div>
      <div>
        <div class="gantt" id="gantt"></div>
        <div class="ga" aria-hidden="true" style="margin-top:8px"><span style="left:0%">00</span><span style="left:25%">06</span><span style="left:50%">12</span><span style="left:75%">18</span><span style="left:100%">24</span></div>
      </div>
    </section>

    <section class="card" id="mapa">
      <div class="ch">
        <div class="chl">
          <div class="ci"><svg viewBox="0 0 24 24" style="stroke:var(--c-temp)"><rect x="4" y="4" width="6" height="6" rx="1.500"/><rect x="14" y="4" width="6" height="6" rx="1.500"/><rect x="4" y="14" width="6" height="6" rx="1.500"/><rect x="14" y="14" width="6" height="6" rx="1.500"/></svg></div>
          <div><h2>Mapa de temperatura</h2><p>Promedio por hora, últimos 7 días</p></div>
        </div>
      </div>
      <div class="heat" id="heat" role="img" aria-label="Mapa de calor de temperatura por día y hora"></div>
      <div class="hlg"><span id="hmin"></span><div class="hg"></div><span id="hmax"></span></div>
    </section>

    <section class="card" id="riego">
      <div class="ch">
        <div class="chl">
          <div class="ci"><svg viewBox="0 0 24 24" style="stroke:var(--c-water)"><path d="M12 3c3.200 4.300 5.800 7.200 5.800 10.300a5.800 5.800 0 0 1-11.600 0C6.200 10.200 8.800 7.300 12 3z"/></svg></div>
          <div><h2>Agua</h2><p id="rgS">Riego diario</p></div>
        </div>
      </div>
      <div class="big"><b id="wTot"></b><span>esta semana, estimado</span></div>
      <label class="ctl" for="caudal">Caudal de la bomba <input id="caudal" type="number" min="0.1" max="60" step="0.1" value="1"> L/min</label>
      <div>
        <div class="bars" id="bars"></div>
        <div class="bl" id="barl"></div>
      </div>
      <ul class="log" id="log"></ul>
    </section>

    <section class="card" id="datos">
      <div class="ch">
        <div class="chl">
          <div class="ci"><svg viewBox="0 0 24 24" style="stroke:var(--fg-2)"><path d="M4 6h16M4 12h16M4 18h16"/></svg></div>
          <div><h2>Resumen diario</h2><p>Los mismos datos de las gráficas, en tabla</p></div>
        </div>
      </div>
      <div class="tw"><table id="tbl"></table></div>
    </section>
  </main>
</div>
<div id="tip" role="tooltip" hidden></div>

<script>
(function () {
  var DN = ['Dom','Lun','Mar','Mié','Jue','Vie','Sáb'];
  var MES = ['ene','feb','mar','abr','may','jun','jul','ago','sep','oct','nov','dic'];
  var D = null, M = null, range = '24', lastW = 0, sig = '';

  function $(s) { return document.querySelector(s); }
  function pad(n) { return String(n).padStart(2, '0'); }
  function fmtHr(h) { var m = Math.round((h % 1) * 60); if (m === 60) { h += 1; m = 0; } return pad(Math.floor(h) % 24) + ':' + pad(m); }
  function hhmm(sec) { return pad(Math.floor(sec / 3600) % 24) + ':' + pad(Math.floor(sec % 3600 / 60)); }
  function fmtDur(sec) { var h = Math.floor(sec / 3600), m = Math.floor(sec % 3600 / 60); return h ? h + ' h ' + m + ' min' : m + ' min'; }
  function esc(s) { return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;'); }
  function num1(v) { return String(Math.round(v * 10) / 10); }

  /* modelo: últimos 7 días en hora local; t = horas desde las 00:00 de hace 6 días */
  function build() {
    var nowL = D.t + D.off, base = Math.floor(nowL / 86400) * 86400 - 6 * 86400;
    var m = { base: base, last: (nowL - base) / 3600, pts: [], days: [], min: [], ini: [], ev: [] };
    for (var i = 0; i < 7; i++) {
      var dt = new Date((base + i * 86400) * 1000);
      m.days.push({ n: DN[dt.getUTCDay()], num: dt.getUTCDate() }); m.min.push(0); m.ini.push(null);
    }
    D.h.forEach(function (r) {
      var t = (r[0] + D.off - base) / 3600; if (t < 0) return;
      m.pts.push({ t: t, d: Math.floor(t / 24), hr: t % 24, T: r[1] / 10, H: r[2] / 10 });
    });
    D.r.forEach(function (r) {
      var l = r[0] + D.off, d = Math.floor((l - base) / 86400); if (d < 0 || d > 6) return;
      m.min[d] += r[1] / 60; if (m.ini[d] === null) m.ini[d] = l % 86400;
      m.ev.push({ d: d, s: l % 86400, dur: r[1] });
    });
    return m;
  }
  function dayName(d) { return M.days[d].n + ' ' + M.days[d].num; }

  /* globo de datos */
  var tip = $('#tip');
  function showTip(html, x, y) {
    tip.innerHTML = html; tip.hidden = false;
    var w = tip.offsetWidth, h = tip.offsetHeight, L = x + 14, T = y - h - 12;
    if (L + w > innerWidth - 8) L = x - w - 14;
    if (L < 8) L = 8;
    if (T < 8) T = y + 18;
    tip.style.left = L + 'px'; tip.style.top = T + 'px';
  }
  function hideTip() { tip.hidden = true; }
  function onPoint(e) {
    var el = e.target.closest && e.target.closest('[data-tip]');
    if (el) showTip(el.getAttribute('data-tip'), e.clientX, e.clientY);
    else if (!(e.target.closest && e.target.closest('svg.line'))) hideTip();
  }
  document.addEventListener('pointermove', onPoint);
  document.addEventListener('pointerdown', onPoint);

  /* gráfica de línea */
  var uid = 0;
  function lineChart(host, o) {
    var W = Math.max(260, Math.round(host.clientWidth)), H = 176, m = { l: 34, r: 54, t: 14, b: 26 };
    var iw = W - m.l - m.r, ih = H - m.t - m.b, P = o.pts, t0 = o.x0, t1 = o.x1;
    var mn = Infinity, mx = -Infinity;
    P.forEach(function (p) { if (p.v < mn) mn = p.v; if (p.v > mx) mx = p.v; });
    var lo = Math.floor(mn / o.step) * o.step, hi = Math.ceil(mx / o.step) * o.step;
    if (hi === lo) hi = lo + o.step;
    function X(t) { return m.l + (t - t0) / (t1 - t0) * iw; }
    function Y(v) { return m.t + ih - (v - lo) / (hi - lo) * ih; }
    var id = 'lg' + (++uid), grid = '';
    for (var v = lo; v <= hi + 1e-9; v += o.step) {
      grid += '<line class="gl" x1="' + m.l + '" x2="' + (W - m.r) + '" y1="' + Y(v).toFixed(1) + '" y2="' + Y(v).toFixed(1) + '"/>' +
        '<text class="yt" x="' + (m.l - 8) + '" y="' + (Y(v) + 4).toFixed(1) + '" text-anchor="end">' + v + '</text>';
    }
    o.ticks(W).forEach(function (k) {
      if (k.t < t0 || k.t > t1) return;
      var x = X(k.t).toFixed(1);
      if (k.sep) grid += '<line class="sep" x1="' + x + '" x2="' + x + '" y1="' + m.t + '" y2="' + (m.t + ih) + '"/>';
      if (k.label) grid += '<text class="xt" x="' + x + '" y="' + (H - 7) + '" text-anchor="middle">' + k.label + '</text>';
    });
    var d = P.map(function (p, i) { return (i ? 'L' : 'M') + X(p.t).toFixed(1) + ' ' + Y(p.v).toFixed(1); }).join('');
    var base = (m.t + ih).toFixed(1), last = P[P.length - 1];
    host.innerHTML =
      '<svg class="line" viewBox="0 0 ' + W + ' ' + H + '" width="' + W + '" height="' + H + '" role="img" aria-label="' + o.label + '">' +
      '<defs><linearGradient id="' + id + '" x1="0" y1="0" x2="0" y2="1"><stop offset="0" style="stop-color:var(' + o.cv + ');stop-opacity:.34"/><stop offset="1" style="stop-color:var(' + o.cv + ');stop-opacity:0"/></linearGradient></defs>' +
      grid +
      '<path d="' + d + 'L' + X(last.t).toFixed(1) + ' ' + base + 'L' + X(P[0].t).toFixed(1) + ' ' + base + 'Z" style="fill:url(#' + id + ')"/>' +
      '<path class="ln" d="' + d + '" style="stroke:var(' + o.cv + ')"/>' +
      '<line class="cx" y1="' + m.t + '" y2="' + (m.t + ih) + '" visibility="hidden"/>' +
      '<circle class="hd" r="5" visibility="hidden" style="fill:var(' + o.cv + ')"/>' +
      '<circle class="dot" cx="' + X(last.t).toFixed(1) + '" cy="' + Y(last.v).toFixed(1) + '" r="4.5" style="fill:var(' + o.cv + ')"/>' +
      '<text class="end" x="' + (X(last.t) + 10).toFixed(1) + '" y="' + (Y(last.v) + 4).toFixed(1) + '">' + last.v.toFixed(o.dec) + o.unit + '</text>' +
      '<rect class="hit" x="' + m.l + '" y="0" width="' + iw + '" height="' + H + '"/></svg>';
    var svg = host.firstChild, cx = svg.querySelector('.cx'), hd = svg.querySelector('.hd');
    function move(e) {
      var r = svg.getBoundingClientRect(), x = (e.clientX - r.left) * (W / r.width);
      var tq = t0 + (x - m.l) / iw * (t1 - t0), best = 0, bd = 1e9;
      for (var j = 0; j < P.length; j++) { var dd = Math.abs(P[j].t - tq); if (dd < bd) { bd = dd; best = j; } }
      var p = P[best], px = X(p.t).toFixed(1);
      cx.setAttribute('x1', px); cx.setAttribute('x2', px); cx.setAttribute('visibility', 'visible');
      hd.setAttribute('cx', px); hd.setAttribute('cy', Y(p.v).toFixed(1)); hd.setAttribute('visibility', 'visible');
      showTip('<b>' + dayName(p.d) + ' · ' + fmtHr(p.hr) + '</b><span>' + p.v.toFixed(1) + ' ' + o.unitFull + '</span>', e.clientX, e.clientY);
    }
    function out() { cx.setAttribute('visibility', 'hidden'); hd.setAttribute('visibility', 'hidden'); hideTip(); }
    svg.addEventListener('pointermove', move);
    svg.addEventListener('pointerdown', move);
    svg.addEventListener('pointerleave', out);
  }

  function ticksFor(W) {
    var a = [], t;
    if (range === '24') {
      var st = W < 520 ? 6 : 3;
      for (t = Math.ceil((M.last - 24) / st) * st; t <= M.last; t += st) a.push({ t: t, label: pad(t % 24) + ':00', sep: t % 24 === 0 });
      return a;
    }
    for (var d = 0; d < 7; d++) { a.push({ t: d * 24 + 12, label: M.days[d].n }); if (d) a.push({ t: d * 24, label: '', sep: true }); }
    return a;
  }
  var EMPTY = '<p class="empty">Esperando lecturas. El ESP32 guarda una cada 30 minutos.</p>';
  function renderClima() {
    var x0 = range === '24' ? M.last - 24 : 0;
    var win = M.pts.filter(function (p) { return p.t >= x0; });
    lastW = $('#chT').clientWidth;
    if (win.length < 2) { $('#chT').innerHTML = EMPTY; $('#chH').innerHTML = ''; return; }
    lineChart($('#chT'), { pts: win.map(function (p) { return { t: p.t, d: p.d, hr: p.hr, v: p.T }; }), x0: x0, x1: M.last, cv: '--c-temp', unit: '°', unitFull: '°C', dec: 1, step: 5, ticks: ticksFor, label: 'Temperatura' });
    lineChart($('#chH'), { pts: win.map(function (p) { return { t: p.t, d: p.d, hr: p.hr, v: p.H }; }), x0: x0, x1: M.last, cv: '--c-hum', unit: '%', unitFull: '% de humedad relativa', dec: 0, step: 10, ticks: ticksFor, label: 'Humedad relativa' });
  }
  function setRange(r) {
    range = r;
    $('#r24').setAttribute('aria-pressed', r === '24'); $('#r7').setAttribute('aria-pressed', r === '7');
    if (M) renderClima();
  }
  $('#r24').addEventListener('click', function () { setRange('24'); });
  $('#r7').addEventListener('click', function () { setRange('7'); });
  if (window.ResizeObserver) new ResizeObserver(function () { var w = $('#chT').clientWidth; if (M && w && Math.abs(w - lastW) > 1) renderClima(); }).observe($('#chT'));

  /* indicadores */
  function nearest(t, tol) {
    var best = null, bd = tol;
    M.pts.forEach(function (p) { var d = Math.abs(p.t - t); if (d <= bd) { bd = d; best = p; } });
    return best;
  }
  function delta(a, b, u) { var x = a - b; return (x >= 0 ? '▲ ' : '▼ ') + Math.abs(x).toFixed(1) + u + ' vs ayer a esta hora'; }
  function renderKpis() {
    var T = D.temp, H = D.hum, y = M ? nearest(M.last - 24, 1) : null;
    $('#kT').innerHTML = (T === null ? '—' : T.toFixed(1)) + '<small> °C</small>';
    $('#kTd').textContent = T !== null && y ? delta(T, y.T, '°') : 'Sin comparación con ayer todavía';
    $('#kH').innerHTML = (H === null ? '—' : Math.round(H)) + '<small> %</small>';
    $('#kHd').textContent = H !== null && y ? delta(H, y.H, ' pts') : 'Sin comparación con ayer todavía';
    $('#kL').textContent = D.luz ? 'Encendida' : 'Apagada';
    var c = D.cfg, txt;
    if (D.test) txt = 'Modo de prueba: horario simulado';
    else if (!D.ok) txt = 'Sin hora de internet';
    else {
      var ns = (D.t + D.off) % 86400, tgt = D.luz ? c.off : c.on, wait = tgt - ns; if (wait <= 0) wait += 86400;
      txt = (D.luz ? 'Apaga' : 'Enciende') + ' a las ' + hhmm(tgt) + ', en ' + fmtDur(wait);
    }
    $('#kLd').textContent = txt;
  }

  /* luz */
  function renderLuz() {
    var c = D.cfg, g = '';
    $('#lzB').textContent = Math.round((c.off - c.on) / 3600) + ' h';
    $('#lzS').textContent = 'al día, de ' + hhmm(c.on) + ' a ' + hhmm(c.off) + ' (según horario)';
    $('#rgS').textContent = 'Riego diario a las ' + hhmm(c.riego);
    for (var d = 0; d < 7; d++) {
      var evs = M.ev.filter(function (e) { return e.d === d; });
      var tp = '<b>' + dayName(d) + '</b><span>Luz ' + hhmm(c.on) + ' a ' + hhmm(c.off) + ' · ' + Math.round((c.off - c.on) / 3600) + ' h' +
        evs.map(function (e) { return '<br>Riego ' + hhmm(e.s) + ' a ' + hhmm(e.s + e.dur) + ' · ' + fmtDur(e.dur); }).join('') + '</span>';
      var tick = M.ini[d] === null ? '' : '<i class="gp" style="left:' + (M.ini[d] / 864).toFixed(2) + '%"></i>';
      g += '<div class="gr"><span class="gd">' + dayName(d) + '</span><div class="gt" data-tip="' + esc(tp) + '"><i class="gb" style="left:' + (c.on / 864).toFixed(2) + '%;width:' + ((c.off - c.on) / 864).toFixed(2) + '%"></i>' + tick + '</div><span class="gv">' + Math.round((c.off - c.on) / 3600) + ' h</span></div>';
    }
    $('#gantt').innerHTML = g;
  }

  /* mapa de temperatura */
  function renderHeat() {
    var sum = [], cnt = [], d, h;
    for (d = 0; d < 7; d++) { sum.push(new Array(24).fill(0)); cnt.push(new Array(24).fill(0)); }
    M.pts.forEach(function (p) { var hh = Math.floor(p.hr); if (p.d >= 0 && p.d < 7) { sum[p.d][hh] += p.T; cnt[p.d][hh]++; } });
    var mn = Infinity, mx = -Infinity, val = [];
    for (d = 0; d < 7; d++) { val.push([]); for (h = 0; h < 24; h++) { var v = cnt[d][h] ? sum[d][h] / cnt[d][h] : null; val[d].push(v); if (v !== null) { if (v < mn) mn = v; if (v > mx) mx = v; } } }
    var out = '<span></span>';
    for (h = 0; h < 24; h++) out += '<span class="hl' + (h % 6 !== 0 && h % 3 === 0 ? ' s3' : '') + '">' + (h % 3 === 0 ? pad(h) : '') + '</span>';
    for (d = 0; d < 7; d++) {
      out += '<span class="hl">' + M.days[d].n + '</span>';
      for (h = 0; h < 24; h++) {
        var vv = val[d][h];
        if (vv === null) out += '<div class="hc na"></div>';
        else {
          var p = mx > mn ? (vv - mn) / (mx - mn) : 0.5, tp = '<b>' + dayName(d) + ' · ' + pad(h) + ':00</b><span>' + vv.toFixed(1) + ' °C</span>';
          out += '<div class="hc" data-tip="' + esc(tp) + '" style="background:color-mix(in oklab,var(--heat-1) ' + (p * 100).toFixed(0) + '%,var(--heat-0))"></div>';
        }
      }
    }
    $('#heat').innerHTML = out;
    $('#hmin').textContent = mn === Infinity ? '—' : mn.toFixed(1) + ' °C';
    $('#hmax').textContent = mx === -Infinity ? '—' : mx.toFixed(1) + ' °C';
  }

  /* agua */
  var CHECK = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M5 12.500l4.500 4.500L19 7.500"/></svg>';
  function caudal() { var q = parseFloat($('#caudal').value); return q > 0 ? q : 0; }
  function renderWater() {
    var q = caudal(), L = M.min.map(function (m) { return m * q; });
    var tot = L.reduce(function (a, b) { return a + b; }, 0), mx = Math.max.apply(null, L.concat([1]));
    $('#wTot').textContent = num1(tot) + ' L';
    var bars = '', labs = '', d;
    for (d = 0; d < 7; d++) {
      var tp = '<b>' + dayName(d) + '</b><span>' + num1(M.min[d]) + ' min de bomba<br>≈ ' + num1(L[d]) + ' L</span>';
      bars += '<div class="bc" data-tip="' + esc(tp) + '"><span class="bv">' + num1(L[d]) + '</span><div class="bar" style="height:' + Math.max(2, L[d] / mx * 104).toFixed(0) + 'px"></div></div>';
      labs += '<span' + (d === 6 ? ' class="today"' : '') + '>' + M.days[d].n + '</span>';
    }
    $('#bars').innerHTML = bars; $('#barl').innerHTML = labs;
    var log = '';
    M.ev.slice(-3).reverse().forEach(function (e) {
      log += '<li><b>Riego automático</b><small>' + dayName(e.d) + ' · ' + hhmm(e.s) + ' a ' + hhmm(e.s + e.dur) + ' · ' + fmtDur(e.dur) + ' · ≈ ' + num1(e.dur / 60 * q) + ' L</small><span class="ok">' + CHECK + 'Completado</span></li>';
    });
    $('#log').innerHTML = log || '<li><small>Todavía no hay riegos registrados.</small></li>';
    var lastEv = M.ev[M.ev.length - 1];
    if (D.bomba) { $('#kW').textContent = 'En curso'; $('#kWd').textContent = 'La bomba está encendida'; }
    else if (lastEv) { $('#kW').innerHTML = num1(lastEv.dur / 60) + '<small> min</small>'; $('#kWd').textContent = dayName(lastEv.d) + ' ' + hhmm(lastEv.s) + ' · ≈ ' + num1(lastEv.dur / 60 * q) + ' L'; }
    else { $('#kW').innerHTML = '—'; $('#kWd').textContent = 'Aún sin riegos registrados'; }
    renderTable(L);
  }
  $('#caudal').addEventListener('input', function () {
    try { localStorage.setItem('caudal', $('#caudal').value); } catch (e) {}
    if (M) renderWater();
  });

  /* tabla */
  function renderTable(L) {
    var c = D.cfg, luzH = Math.round((c.off - c.on) / 3600);
    var h = '<thead><tr><th>Día</th><th>Temp. mín</th><th>Temp. máx</th><th>Humedad prom.</th><th>Luz</th><th>Riego</th><th>Agua estimada</th></tr></thead><tbody>';
    for (var d = 0; d < 7; d++) {
      var a = M.pts.filter(function (p) { return p.d === d; });
      var T = a.map(function (p) { return p.T; }), Hs = a.map(function (p) { return p.H; });
      var has = a.length > 0;
      h += '<tr><td>' + dayName(d) + '</td><td>' + (has ? Math.min.apply(null, T).toFixed(1) + ' °C' : '—') + '</td><td>' + (has ? Math.max.apply(null, T).toFixed(1) + ' °C' : '—') + '</td><td>' +
        (has ? Math.round(Hs.reduce(function (x, y) { return x + y; }, 0) / Hs.length) + ' %' : '—') + '</td><td>' + luzH + ' h</td><td>' + num1(M.min[d]) + ' min</td><td>' + num1(L[d]) + ' L</td></tr>';
    }
    $('#tbl').innerHTML = h + '</tbody>';
  }

  /* carga */
  function renderAll() {
    var live = $('#live'); live.className = 'chip';
    if (!D.ok) {
      M = null;
      live.innerHTML = '<i></i>Sin hora';
      $('#rng').textContent = 'El ESP32 todavía no tiene la hora de internet.';
      renderKpis();
      $('#chT').innerHTML = '<p class="empty">Esperando la hora de internet.</p>'; $('#chH').innerHTML = '';
      return;
    }
    M = build();
    var f = new Date(M.base * 1000), l = new Date((M.base + 6 * 86400) * 1000);
    live.innerHTML = '<i></i>En vivo · ' + hhmm((D.t + D.off) % 86400);
    $('#rng').textContent = 'Últimos 7 días · ' + DN[f.getUTCDay()] + ' ' + f.getUTCDate() + ' ' + MES[f.getUTCMonth()] + ' – ' + DN[l.getUTCDay()] + ' ' + l.getUTCDate() + ' ' + MES[l.getUTCMonth()] + ' ' + l.getUTCFullYear();
    renderKpis(); renderLuz(); renderHeat(); renderWater();
    var s = D.h.length + ':' + (D.h.length ? D.h[D.h.length - 1][0] : 0) + ':' + range + ':' + M.base;
    if (s !== sig) { sig = s; renderClima(); }
  }
  function load() {
    fetch('/datos', { cache: 'no-store' })
      .then(function (r) { if (!r.ok) throw new Error('http'); return r.json(); })
      .then(function (j) { D = j; renderAll(); })
      .catch(function () { var l = $('#live'); l.className = 'chip off'; l.innerHTML = '<i></i>Sin conexión'; });
  }
  try { var q0 = localStorage.getItem('caudal'); if (q0) $('#caudal').value = q0; } catch (e) {}

  function markNav() {
    var id = (location.hash || '#resumen').slice(1), map = { mapa: 'clima' };
    id = map[id] || id;
    document.querySelectorAll('.pill a').forEach(function (a) { a.classList.toggle('on', a.getAttribute('href') === '#' + id); });
  }
  window.addEventListener('hashchange', markNav);
  markNav();
  load();
  setInterval(load, 15000);
})();
</script>
</body></html>
)rawliteral";