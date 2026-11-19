// Página web servida pelo ESP32 (abre em qualquer navegador de celular).
// Motores DC: SEGURE o botão para mover, SOLTE para parar.
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
  :root { --bg:#10141a; --card:#1b222c; --txt:#e8edf3; --sub:#8a96a6; --acc:#2f9bff; --red:#e5484d; --ok:#30a46c; --warn:#f5a524; }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; -webkit-user-select:none; user-select:none; -webkit-touch-callout:none; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--txt); padding:16px; }
  header { display:flex; justify-content:space-between; align-items:center; margin-bottom:14px; }
  h1 { font-size:20px; margin:0; }
  #st { font-size:13px; padding:4px 10px; border-radius:99px; background:var(--red); }
  #st.on { background:var(--ok); }
  .card { background:var(--card); border-radius:14px; padding:14px; margin-bottom:12px; }
  .top { display:flex; justify-content:space-between; align-items:center; font-size:16px; margin-bottom:10px; }
  .tag { font-size:12px; color:var(--sub); }
  .tag.mov { color:var(--acc); } .tag.lim { color:var(--warn); }
  .row { display:flex; gap:10px; }
  .hold { flex:1; height:64px; border:0; border-radius:12px; background:#2a3442; color:var(--txt); font-size:17px; font-weight:600; touch-action:none; }
  .hold.on { background:var(--acc); color:#fff; }
  #stop { width:100%; height:60px; border:0; border-radius:14px; background:var(--red); color:#fff; font-size:20px; font-weight:800; margin-bottom:12px; }
  input[type=range] { width:100%; height:36px; accent-color:var(--acc); }
  small { color:var(--sub); }
</style>
</head>
<body>
<header><h1>🦾 Garra Hidráulica</h1><span id="st">desconectado</span></header>
<button id="stop">PARAR TUDO</button>
<div id="motores"></div>
<div class="card">
  <div class="top"><span>Velocidade</span><b id="vv">--</b></div>
  <input type="range" id="vel" min="30" max="100" step="5">
  <small>Segure o botão para mover e solte para parar.</small>
</div>

<script>
const $ = id => document.getElementById(id);
let estado = null, segurando = {}, ocupado = false, fila = null;

function online(ok) { $('st').textContent = ok ? 'conectado' : 'desconectado'; $('st').classList.toggle('on', ok); }

// Uma requisição por vez; o comando mais novo substitui o que estava esperando
function enviar(m) {
  if (ocupado) { fila = fila ? fila + ';' + m : m; return; }
  ocupado = true;
  fetch('/cmd?c=' + encodeURIComponent(m), { cache: 'no-store' })
    .then(r => r.json()).then(s => { online(true); atualizar(s); })
    .catch(() => online(false))
    .finally(() => { ocupado = false; if (fila) { const f = fila; fila = null; enviar(f); } });
}

function montar(j) {
  $('motores').innerHTML = j.map((x, i) => `
    <div class="card">
      <div class="top"><span>${x.n}</span><span class="tag" id="t${i}">parado</span></div>
      <div class="row">
        <button class="hold" data-i="${i}" data-d="-1">◀ ${x.m}</button>
        <button class="hold" data-i="${i}" data-d="1">${x.p} ▶</button>
      </div>
    </div>`).join('');
  document.querySelectorAll('.hold').forEach(b => {
    const i = +b.dataset.i, d = +b.dataset.d;
    const ini = e => { e.preventDefault(); b.setPointerCapture && b.setPointerCapture(e.pointerId);
                       segurando[i] = d; b.classList.add('on'); enviar(`m,${i},${d}`); };
    const fim = () => { if (segurando[i] !== d) return; delete segurando[i]; b.classList.remove('on'); enviar(`m,${i},0`); };
    b.addEventListener('pointerdown', ini);
    ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(ev => b.addEventListener(ev, fim));
    b.addEventListener('contextmenu', e => e.preventDefault());
  });
}

function atualizar(s) {
  if (!estado || estado.j.length !== s.j.length) montar(s.j);
  estado = s;
  s.j.forEach((x, i) => {
    const t = $('t' + i);
    if (x.t) { t.textContent = 'tempo máximo: solte o botão'; t.className = 'tag lim'; }
    else if (x.d) { t.textContent = 'movendo: ' + (x.d > 0 ? x.p : x.m); t.className = 'tag mov'; }
    else { t.textContent = 'parado'; t.className = 'tag'; }
  });
  if (document.activeElement !== $('vel')) $('vel').value = s.v;
  $('vv').textContent = s.v + '%';
}

$('stop').onclick = () => {
  Object.keys(segurando).forEach(k => delete segurando[k]);
  document.querySelectorAll('.hold.on').forEach(b => b.classList.remove('on'));
  fila = null; enviar('x');
};
$('vel').oninput = () => { $('vv').textContent = $('vel').value + '%'; enviar('v,' + $('vel').value); };

// Enquanto um botão está apertado, repete o comando (se parar de chegar, o motor para)
setInterval(() => {
  const ks = Object.keys(segurando);
  enviar(ks.length ? ks.map(i => `m,${i},${segurando[i]}`).join(';') : 'p');
}, 150);
enviar('p');
document.addEventListener('visibilitychange', () => { if (document.hidden) $('stop').onclick(); });
</script>
</body>
</html>
)rawliteral";
