// Página web servida pelo ESP32 (abre em qualquer navegador de celular).
#pragma once
#include <Arduino.h>

const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>Garra Hidráulica</title>
<style>
  :root { --bg:#10141a; --card:#1b222c; --txt:#e8edf3; --sub:#8a96a6; --acc:#2f9bff; --red:#e5484d; --ok:#30a46c; }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--txt); padding:16px; }
  header { display:flex; justify-content:space-between; align-items:center; margin-bottom:14px; }
  h1 { font-size:20px; margin:0; }
  #st { font-size:13px; padding:4px 10px; border-radius:99px; background:var(--red); }
  #st.on { background:var(--ok); }
  .card { background:var(--card); border-radius:14px; padding:14px; margin-bottom:12px; }
  .top { display:flex; justify-content:space-between; font-size:15px; margin-bottom:8px; }
  .top b { color:var(--acc); font-variant-numeric:tabular-nums; }
  .row { display:flex; gap:10px; align-items:center; }
  input[type=range] { flex:1; height:36px; accent-color:var(--acc); }
  .mini { width:44px; height:44px; border-radius:10px; border:0; background:#2a3442; color:var(--txt); font-size:22px; }
  .btns { display:flex; gap:10px; margin-bottom:12px; }
  .btn { flex:1; height:56px; border:0; border-radius:14px; font-size:18px; font-weight:700; color:#fff; }
  #stop { background:var(--red); flex:2; }
  #stop.on { animation:pisca 1s infinite; }
  #home { background:#3a4656; }
  @keyframes pisca { 50% { opacity:.6; } }
  small { color:var(--sub); }
</style>
</head>
<body>
<header><h1>🦾 Garra Hidráulica</h1><span id="st">desconectado</span></header>

<div class="btns">
  <button class="btn" id="stop">PARAR</button>
  <button class="btn" id="home">Home</button>
</div>

<div id="juntas"></div>

<div class="card">
  <div class="top"><span>Velocidade</span><b id="vv">--</b></div>
  <div class="row"><input type="range" id="vel" min="5" max="360" step="5"></div>
  <small>graus por segundo — mais devagar = mais força e menos tranco</small>
</div>

<script>
let estado = null, arrastando = {}, pend = {}, timer = null;
const $ = id => document.getElementById(id);

function online(ok) {
  $('st').textContent = ok ? 'conectado' : 'desconectado';
  $('st').classList.toggle('on', ok);
}
function enviar(m) {
  return fetch('/cmd?c=' + encodeURIComponent(m), { cache: 'no-store' })
    .then(r => r.json()).then(s => { online(true); atualizar(s); })
    .catch(() => online(false));
}

// Agrupa envios (no máx. a cada 50 ms) para não afogar o ESP32
function agendar(chave, msg) {
  pend[chave] = msg;
  if (!timer) timer = setTimeout(() => { Object.values(pend).forEach(enviar); pend = {}; timer = null; }, 50);
}

function montar(j) {
  $('juntas').innerHTML = j.map((x, i) => `
    <div class="card">
      <div class="top"><span>${x.n}</span><b id="b${i}">${x.a}°</b></div>
      <div class="row">
        <button class="mini" data-i="${i}" data-d="-5">−</button>
        <input type="range" id="r${i}" min="${x.min}" max="${x.max}" value="${x.a}">
        <button class="mini" data-i="${i}" data-d="5">+</button>
      </div>
    </div>`).join('');
  j.forEach((x, i) => {
    const r = $('r' + i);
    r.oninput = () => { $('b' + i).textContent = r.value + '°'; agendar('s' + i, `s,${i},${r.value}`); };
    r.onpointerdown = () => arrastando[i] = true;
    r.onpointerup = r.onpointercancel = () => arrastando[i] = false;
  });
  document.querySelectorAll('.mini').forEach(b => b.onclick = () => {
    const i = +b.dataset.i, r = $('r' + i);
    r.value = +r.value + +b.dataset.d; r.oninput();
  });
}

function atualizar(s) {
  if (!estado || estado.j.length !== s.j.length) montar(s.j);
  estado = s;
  s.j.forEach((x, i) => {
    if (!arrastando[i]) { $('r' + i).value = x.a; $('b' + i).textContent = x.a + '°'; }
  });
  if (document.activeElement !== $('vel')) $('vel').value = s.v;
  $('vv').textContent = s.v + '°/s';
  $('stop').classList.toggle('on', s.p);
}

$('stop').onclick = () => enviar('x');
$('home').onclick = () => enviar('h');
$('vel').oninput  = () => { $('vv').textContent = $('vel').value + '°/s'; agendar('v', 'v,' + $('vel').value); };
setInterval(() => enviar('p'), 1000);   // "estou vivo" + sincroniza com outros celulares
enviar('p');
</script>
</body>
</html>
)rawliteral";
