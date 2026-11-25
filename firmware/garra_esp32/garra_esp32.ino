// ============================================================
//  Garra hidráulica (seringas + água) com MOTORES DC + PONTE H
//  controlada pelo celular.
//
//  NÃO PRECISA DE NENHUMA BIBLIOTECA EXTRA: usa só o que já vem
//  com o suporte ao ESP32 na Arduino IDE (WiFi, WebServer e LEDC).
//  Tudo está neste arquivo único: a configuração fica logo abaixo.
//
//  Funciona com ponte H L298N, L9110S, TB6612 ou parecidas
//  (2 entradas por motor). No L298N, deixe os jumpers ENA/ENB.
//
//  Comandos em http://192.168.4.1/cmd?c=<comando>
//    m,<motor>,<dir>   dir = 1 (botão +), -1 (botão −), 0 (parar)
//                      o celular repete enquanto o botão está apertado
//    v,<0-100>         velocidade em %
//    x                 PARAR tudo
//    p                 ping
//  Vários comandos podem ir juntos, separados por ';'  ex: m,0,1;m,1,-1
//  Toda resposta é um JSON com o estado atual.
// ============================================================
#include <WiFi.h>
#include <WebServer.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

// ============================================================
//  CONFIGURAÇÃO  (edite aqui: pinos, nomes, rede, velocidade)
// ============================================================

// ---------- Rede Wi-Fi criada pelo ESP32 (modo Access Point) ----------
// O celular conecta nesta rede e abre http://192.168.4.1
#define WIFI_SSID   "Garra-ESP"
#define WIFI_SENHA  "garra1234"   // mínimo 8 caracteres

// ---------- Motores (cada um = 1 motor DC + 1 par de seringas) ----------
// 8 seringas = 4 pares = 4 motores = 2 módulos de ponte H (2 motores cada).
//
// pinoA / pinoB : vão nas entradas da ponte H (IN1/IN2 ou IN3/IN4).
// botaoMenos / botaoMais : texto dos botões no celular.
// invertido     : true se o motor gira ao contrário do botão.
//
//  Ponte H 1:  IN1 = 32  IN2 = 33  (Base)    |  IN3 = 25  IN4 = 26  (Ombro)
//  Ponte H 2:  IN1 = 23  IN2 = 22  (Cotovelo)|  IN3 = 19  IN4 = 18  (Garra)
//  (Ponte H 1 usa pinos do lado esquerdo da placa, a Ponte H 2 do lado direito.)
struct Motor {
  const char* nome;
  const char* botaoMenos;
  const char* botaoMais;
  uint8_t     pinoA;
  uint8_t     pinoB;
  bool        invertido;
};

#define NUM_MOTORES 4

static const Motor MOTORES[NUM_MOTORES] = {
  // nome        botão −      botão +     pinoA pinoB invertido
  { "Base",     "Esquerda",  "Direita",    32,   33,   false },
  { "Ombro",    "Desce",     "Sobe",       25,   26,   false },
  { "Cotovelo", "Recolhe",   "Estica",     23,   22,   false },
  { "Garra",    "Abre",      "Fecha",      19,   18,   false },
};

// ---------- Velocidade (PWM nas entradas da ponte H) ----------
#define VELOCIDADE_PADRAO  70    // % (ajustável pelo celular)
#define VELOCIDADE_MIN     30    // abaixo disso o motor costuma nem girar
#define PWM_FREQ         1000    // Hz
#define PWM_BITS            8    // 0–255
#define RAMPA_MS          150    // partida suave (evita tranco na hidráulica)

// ---------- Segurança ----------
// O celular manda o comando repetidamente enquanto o botão está apertado.
// Se parar de chegar por este tempo, o motor para (botão solto / sinal caiu).
#define TEMPO_SEM_COMANDO_MS  400
// Tempo máximo girando sem soltar o botão (protege o fim de curso da seringa).
#define TEMPO_MAX_MS         6000

// ============================================================
//  PÁGINA DO CELULAR
// ============================================================
// Página web servida pelo ESP32 (abre em qualquer navegador de celular).
// Motores DC: SEGURE o botão para mover, SOLTE para parar.

const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<meta name="theme-color" content="#101820">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Guindaste">
<link rel="manifest" href="/manifest.json">
<link rel="icon" href="/icon.svg" type="image/svg+xml">
<link rel="apple-touch-icon" href="/icon.svg">
<title>Guindaste Hidráulico</title>
<style>
  :root { color-scheme:dark; --bg:#101820; --panel:#18232d; --panel2:#202e39; --line:#2b3b47; --text:#edf3f5; --muted:#91a1ac; --amber:#f6b83f; --amber-ink:#211707; --red:#ee554f; --green:#51d39b; --blue:#79bdf2; }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; -webkit-user-select:none; user-select:none; -webkit-touch-callout:none; }
  html { min-height:100%; background:var(--bg); }
  body { min-height:100vh; margin:0; padding:max(16px,env(safe-area-inset-top)) max(16px,env(safe-area-inset-right)) max(22px,env(safe-area-inset-bottom)) max(16px,env(safe-area-inset-left)); color:var(--text); background:radial-gradient(ellipse at 82% 0%,#1c2a34 0,transparent 38%),var(--bg); font-family:Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif; }
  button { font:inherit; }
  .shell { max-width:1100px; margin:0 auto; }
  .topbar { display:flex; align-items:center; gap:12px; min-height:48px; }
  .brand-mark { width:42px; height:42px; display:grid; place-items:center; flex:none; border:1px solid #69572e; border-radius:13px; color:var(--amber); background:#30291c; }
  .brand-mark svg { width:25px; height:25px; }
  .brand-title { margin:0; font-size:15px; font-weight:750; letter-spacing:.01em; }
  .brand-sub { margin:3px 0 0; color:var(--muted); font-size:11px; letter-spacing:.1em; text-transform:uppercase; }
  .connection { display:flex; align-items:center; gap:8px; margin-left:auto; padding:9px 12px; color:var(--muted); background:#18232d; border:1px solid var(--line); border-radius:99px; font-size:12px; }
  .connection i { width:8px; height:8px; flex:none; border-radius:50%; background:var(--red); box-shadow:0 0 0 3px #ee554f22; }
  .connection.on { color:#c9f8e4; border-color:#285443; background:#14271f; }
  .connection.on i { background:var(--green); box-shadow:0 0 0 3px #51d39b22; }
  .overview { display:grid; grid-template-columns:1fr minmax(210px, .75fr); align-items:center; gap:18px; min-height:226px; margin:20px 0 16px; padding:25px 28px; overflow:hidden; border:1px solid var(--line); border-radius:22px; background:linear-gradient(115deg,#1b2832 0%,#17232d 68%,#1d2b34 100%); }
  .eyebrow { color:var(--amber); font-size:11px; font-weight:800; letter-spacing:.16em; text-transform:uppercase; }
  .overview h2 { max-width:510px; margin:9px 0 9px; font-size:clamp(25px,4vw,36px); line-height:1.08; letter-spacing:-.04em; }
  .overview p { max-width:430px; margin:0; color:var(--muted); font-size:14px; line-height:1.6; }
  .specs { display:flex; gap:20px; margin-top:19px; }
  .spec b { display:block; font-size:15px; }
  .spec span { display:block; margin-top:3px; color:var(--muted); font-size:11px; }
  .crane-art { width:100%; max-height:190px; color:var(--amber); filter:drop-shadow(0 10px 24px #0005); }
  .section-heading { display:flex; align-items:flex-end; justify-content:space-between; gap:12px; margin:24px 2px 11px; }
  .section-heading h2 { margin:0; font-size:17px; letter-spacing:-.02em; }
  .section-heading p { margin:0; color:var(--muted); font-size:12px; }
  #grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:12px; }
  .card { min-width:0; padding:15px; border:1px solid var(--line); border-radius:17px; background:linear-gradient(145deg,#1b2731,#17222b); }
  .card-head { display:flex; align-items:center; gap:10px; min-height:38px; margin-bottom:13px; }
  .axis-number { display:grid; width:34px; height:34px; place-items:center; flex:none; border:1px solid #344753; border-radius:11px; color:var(--blue); background:#1b2b35; font-size:12px; font-weight:800; }
  .axis-copy { min-width:0; }
  .axis-copy strong { display:block; overflow:hidden; font-size:14px; text-overflow:ellipsis; white-space:nowrap; }
  .axis-copy small { display:block; margin-top:3px; color:var(--muted); font-size:11px; }
  .tag { margin-left:auto; padding:5px 8px; border-radius:99px; color:var(--muted); background:#23313b; font-size:10px; font-weight:750; letter-spacing:.04em; white-space:nowrap; }
  .tag.mov { color:var(--amber-ink); background:var(--amber); }
  .tag.lim { color:#271d08; background:#f4c55f; }
  .row { display:grid; grid-template-columns:1fr 1fr; gap:9px; }
  .hold { display:flex; align-items:center; justify-content:center; gap:7px; min-width:0; min-height:58px; padding:8px 7px; border:1px solid #344550; border-radius:12px; color:var(--text); background:#24323c; font-size:13px; font-weight:700; touch-action:none; transition:background .12s,border-color .12s,transform .12s; }
  .hold svg { width:17px; height:17px; flex:none; }
  .hold:focus-visible,#stop:focus-visible,input:focus-visible { outline:2px solid var(--amber); outline-offset:3px; }
  .hold.on { transform:scale(.98); border-color:var(--amber); color:var(--amber-ink); background:var(--amber); }
  .hold:active { transform:scale(.98); }
  .stop-panel { display:flex; align-items:center; gap:14px; margin-top:16px; padding:13px 15px; border:1px solid #663a3a; border-radius:16px; background:linear-gradient(100deg,#321f22,#281d22); }
  .stop-copy { flex:1; min-width:0; }
  .stop-copy strong { display:block; font-size:13px; }
  .stop-copy span { display:block; margin-top:3px; color:#c6a4a4; font-size:11px; }
  #stop { display:flex; align-items:center; justify-content:center; gap:8px; min-width:170px; min-height:48px; padding:0 16px; border:0; border-radius:11px; color:#fff; background:var(--red); font-size:13px; font-weight:850; letter-spacing:.06em; touch-action:manipulation; }
  #stop:active { transform:scale(.98); filter:brightness(.9); }
  .speed-panel { display:grid; grid-template-columns:auto minmax(100px,1fr) auto; align-items:center; gap:14px; margin-top:12px; padding:14px 16px; border:1px solid var(--line); border-radius:16px; background:var(--panel); }
  .speed-title strong { display:block; font-size:13px; }
  .speed-title span { display:block; margin-top:3px; color:var(--muted); font-size:11px; }
  input[type=range] { width:100%; height:30px; margin:0; accent-color:var(--amber); }
  .speed-value { min-width:52px; text-align:right; font-size:17px; font-variant-numeric:tabular-nums; font-weight:800; }
  .hint { margin:13px 2px 0; color:var(--muted); font-size:11px; line-height:1.55; text-align:center; }
  @media (max-width:560px) {
    body { padding-left:max(12px,env(safe-area-inset-left)); padding-right:max(12px,env(safe-area-inset-right)); }
    .overview { grid-template-columns:1fr minmax(100px,.55fr); gap:8px; min-height:190px; margin-top:14px; padding:19px 16px; }
    .overview h2 { font-size:27px; }
    .overview p { font-size:12px; }
    .specs { gap:13px; margin-top:14px; }
    .crane-art { max-height:145px; }
    #grid { gap:9px; }
    .card { padding:11px; }
    .card-head { gap:7px; margin-bottom:10px; }
    .axis-number { width:29px; height:29px; }
    .axis-copy strong { font-size:12px; }
    .axis-copy small { font-size:10px; }
    .tag { padding:4px 6px; font-size:9px; }
    .hold { min-height:54px; gap:4px; font-size:11px; }
    .hold svg { width:15px; height:15px; }
    .stop-panel { gap:9px; padding:10px; }
    .stop-copy span { font-size:10px; }
    #stop { min-width:132px; min-height:46px; padding:0 10px; font-size:11px; }
    .speed-panel { gap:9px; padding:12px; }
    .speed-title span { font-size:10px; }
    .speed-value { min-width:44px; font-size:15px; }
  }
  @media (max-width:360px) {
    #grid { grid-template-columns:1fr; }
    .overview { grid-template-columns:1fr 100px; }
    .axis-copy strong { font-size:13px; }
    .hold { font-size:13px; }
  }
  @media (orientation:landscape) and (max-height:520px) {
    body { padding-top:10px; }
    .overview { min-height:150px; margin:10px 0; padding:15px 20px; }
    .overview h2 { font-size:25px; }
    .crane-art { max-height:125px; }
    .section-heading { margin-top:14px; }
    .card { padding:10px; }
    .hold { min-height:46px; }
    .stop-panel { margin-top:10px; padding:8px 12px; }
    #stop { min-height:40px; }
    .hint { margin-top:8px; }
  }
</style>
</head>
<body>
<div class="shell">
  <header class="topbar">
    <div class="brand-mark"><svg viewBox="0 0 32 32" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M4 27h24M8 27V9h8M16 9l9 5M16 9v9l8 5M24 14v10M21 24a3 3 0 0 0 6 0"/><path d="M8 13h8M8 18h5"/></svg></div>
    <div><p class="brand-title">Controle hidráulico</p><p class="brand-sub">Console de operação · ESP32</p></div>
    <div class="connection" id="st" role="status" aria-live="polite"><i></i><span>Conectando</span></div>
  </header>

  <main>
    <section class="overview" aria-labelledby="page-title">
      <div>
        <div class="eyebrow">Operação manual</div>
        <h2 id="page-title">Guindaste<br>hidráulico</h2>
        <p>Controle os movimentos do braço pelo celular. O ESP32 mantém a conexão local, sem precisar de internet.</p>
        <div class="specs"><div class="spec"><b>04</b><span>movimentos</span></div><div class="spec"><b>Wi-Fi</b><span>conexão direta</span></div></div>
      </div>
      <svg class="crane-art" viewBox="0 0 260 190" fill="none" role="img" aria-label="Ilustração de um guindaste hidráulico">
        <path d="M24 164h212M52 157h156l-12 13H64z" stroke="#71818a" stroke-width="5" stroke-linejoin="round"/>
        <path d="M78 154l12-20h76l16 20" fill="#2c3b43" stroke="#8c9ca3" stroke-width="3" stroke-linejoin="round"/>
        <circle cx="127" cy="113" r="12" fill="#f6b83f" stroke="#101820" stroke-width="5"/>
        <path d="M127 113 98 54l8-5 36 57M132 108 194 43l8 7-64 70" fill="#d9982d" stroke="#f6b83f" stroke-width="3" stroke-linejoin="round"/>
        <path d="m99 56 33 53M139 108l59-58" stroke="#f8d68e" stroke-width="3" stroke-linecap="round"/>
        <path d="M197 45l13-14 17 2-17 7M225 33l10 3-8 9" stroke="#f6b83f" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
        <path d="M194 49 201 36M77 155V96M77 96h22" stroke="#71818a" stroke-width="4" stroke-linecap="round"/>
        <circle cx="127" cy="113" r="23" stroke="#f6b83f" stroke-opacity=".22" stroke-width="2"/>
        <path d="M65 174h126" stroke="#364650" stroke-width="2" stroke-dasharray="4 6"/>
      </svg>
    </section>

    <div class="section-heading"><h2>Movimentos do guindaste</h2><p>Pressione e mantenha para mover</p></div>
    <div id="grid" aria-live="polite"></div>

    <section class="stop-panel" aria-label="Parada geral">
      <div class="stop-copy"><strong>Precisa interromper?</strong><span>Envia parada para todos os motores.</span></div>
      <button id="stop" type="button"><svg width="17" height="17" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><rect x="5" y="5" width="14" height="14" rx="2"/></svg>PARAR TUDO</button>
    </section>

    <section class="speed-panel" aria-label="Ajuste de velocidade">
      <div class="speed-title"><strong>Velocidade</strong><span>Ajuste antes de movimentar</span></div>
      <input type="range" id="vel" min="30" max="100" step="5" aria-label="Velocidade dos motores">
      <div class="speed-value" id="vv">--%</div>
    </section>
    <p class="hint">Solte o controle para parar. Por segurança, cada movimento para automaticamente após 6 segundos; evite forçar o fim de curso das seringas.</p>
  </main>
</div>

<script>
const $ = id => document.getElementById(id);
const ICON = {
  l: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M15 5l-7 7 7 7"/></svg>',
  r: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 5l7 7-7 7"/></svg>'
};
const TITULOS = ['Giro da base', 'Elevação do braço', 'Extensão do braço', 'Abertura da garra'];
let estado = null, segurando = {}, ocupado = false, fila = null, falhas = 0;

function online(ok) {
  falhas = ok ? 0 : falhas + 1;
  const on = ok || (falhas < 3 && $('st').classList.contains('on'));
  $('st').classList.toggle('on', on);
  $('st').querySelector('span').textContent = on ? 'ESP32 conectado' : (falhas < 3 ? 'Conectando' : 'Sem conexão');
}
function vibrar(ms) { if (navigator.vibrate) navigator.vibrate(ms); }

function enviar(m) {
  if (ocupado) { fila = fila ? fila + ';' + m : m; return; }
  ocupado = true;
  const ctl = new AbortController(); const to = setTimeout(() => ctl.abort(), 1500);
  fetch('/cmd?c=' + encodeURIComponent(m), { cache: 'no-store', signal: ctl.signal })
    .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); return r.json(); })
    .then(s => { online(true); atualizar(s); })
    .catch(() => online(false))
    .finally(() => { clearTimeout(to); ocupado = false; if (fila) { const f = fila; fila = null; enviar(f); } });
}

function montar(j) {
  $('grid').innerHTML = j.map((x, i) => `
    <div class="card">
      <div class="card-head"><span class="axis-number">0${i + 1}</span><div class="axis-copy"><strong>${TITULOS[i] || x.n}</strong><small>${x.n}</small></div><span class="tag" id="t${i}">PRONTO</span></div>
      <div class="row">
        <button class="hold" type="button" aria-label="${x.n}: ${x.m}" data-i="${i}" data-d="-1">${ICON.l}<span>${x.m}</span></button>
        <button class="hold" type="button" aria-label="${x.n}: ${x.p}" data-i="${i}" data-d="1"><span>${x.p}</span>${ICON.r}</button>
      </div>
    </div>`).join('');
  document.querySelectorAll('.hold').forEach(b => {
    const i = +b.dataset.i, d = +b.dataset.d;
    const iniciar = () => {
      if (Object.prototype.hasOwnProperty.call(segurando, i)) return;
      segurando[i] = d; b.classList.add('on'); vibrar(12); enviar(`m,${i},${d}`);
    };
    const fim = () => {
      if (segurando[i] !== d) return;
      delete segurando[i]; b.classList.remove('on'); enviar(`m,${i},0`);
    };
    b.addEventListener('pointerdown', e => {
      e.preventDefault();
      if (b.setPointerCapture) try { b.setPointerCapture(e.pointerId); } catch (_) {}
      iniciar();
    });
    ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(ev => b.addEventListener(ev, fim));
    b.addEventListener('keydown', e => {
      if (e.key === ' ' || e.key === 'Enter') { e.preventDefault(); iniciar(); }
    });
    b.addEventListener('keyup', e => {
      if (e.key === ' ' || e.key === 'Enter') { e.preventDefault(); fim(); }
    });
    b.addEventListener('contextmenu', e => e.preventDefault());
  });
}

function atualizar(s) {
  if (!estado || estado.j.length !== s.j.length) montar(s.j);
  estado = s;
  s.j.forEach((x, i) => {
    const t = $('t' + i);
    if (x.t) { t.textContent = 'SOLTE BOTÃO'; t.className = 'tag lim'; }
    else if (x.d) { t.textContent = x.d > 0 ? x.p.toUpperCase() : x.m.toUpperCase(); t.className = 'tag mov'; }
    else { t.textContent = 'PRONTO'; t.className = 'tag'; }
  });
  if (document.activeElement !== $('vel')) $('vel').value = s.v;
  $('vv').textContent = s.v + '%';
}

function pararTudo() {
  Object.keys(segurando).forEach(k => delete segurando[k]);
  document.querySelectorAll('.hold.on').forEach(b => b.classList.remove('on'));
  fila = null; enviar('x');
}
$('stop').onclick = () => { vibrar([30, 40, 30]); pararTudo(); };
$('vel').oninput = () => { $('vv').textContent = $('vel').value + '%'; enviar('v,' + $('vel').value); };

setInterval(() => {
  const ks = Object.keys(segurando);
  enviar(ks.length ? ks.map(i => `m,${i},${segurando[i]}`).join(';') : 'p');
}, 150);
document.addEventListener('visibilitychange', () => { if (document.hidden) pararTudo(); });
window.addEventListener('blur', () => { if (Object.keys(segurando).length) pararTudo(); });
enviar('p');
</script>
</body>
</html>
)rawliteral";

// Ícone e manifesto: permitem "Adicionar à tela inicial" com cara de app
const char ICONE_SVG[] PROGMEM = R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><rect width="512" height="512" rx="112" fill="#f6b83f"/><g fill="none" stroke="#211707" stroke-width="28" stroke-linecap="round" stroke-linejoin="round"><path d="M72 424h368M122 408V104h126M248 104l152 94M248 104v145l116 79M364 194v112M338 306a26 26 0 0 0 52 0M122 180h126M122 252h90"/></g><circle cx="248" cy="249" r="28" fill="#211707"/></svg>)rawliteral";

const char MANIFESTO_JSON[] PROGMEM = R"rawliteral({"name":"Guindaste Hidráulico","short_name":"Guindaste","start_url":"/","display":"standalone","orientation":"any","background_color":"#101820","theme_color":"#101820","icons":[{"src":"/icon.svg","sizes":"any","type":"image/svg+xml","purpose":"any"}]})rawliteral";

// ============================================================
//  PROGRAMA
// ============================================================

int   direcao[NUM_MOTORES];          // -1, 0, 1
float duty[NUM_MOTORES];             // força atual (0..1), sobe na rampa
unsigned long ultimoCmd[NUM_MOTORES];
unsigned long inicioGiro[NUM_MOTORES];
bool  travado[NUM_MOTORES];          // parou por tempo máximo: espera soltar o botão
int   velocidade = VELOCIDADE_PADRAO;

WebServer server(80);

// ------------------------------------------------------------
//  PWM via LEDC (nativo do ESP32), compatível com core 2.x e 3.x
// ------------------------------------------------------------
#define PWM_MAX ((1 << PWM_BITS) - 1)

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  void pwmIniciar(uint8_t pino, uint8_t) { ledcAttach(pino, PWM_FREQ, PWM_BITS); }
  void pwmEscrever(uint8_t pino, uint8_t, uint32_t v) { ledcWrite(pino, v); }
#else
  void pwmIniciar(uint8_t pino, uint8_t canal) { ledcSetup(canal, PWM_FREQ, PWM_BITS); ledcAttachPin(pino, canal); }
  void pwmEscrever(uint8_t, uint8_t canal, uint32_t v) { ledcWrite(canal, v); }
#endif

void aplicarMotor(int i) {
  int d = direcao[i];
  if (MOTORES[i].invertido) d = -d;
  uint32_t v = (uint32_t)(duty[i] * PWM_MAX);
  uint32_t a = (d > 0) ? v : 0;
  uint32_t b = (d < 0) ? v : 0;
  pwmEscrever(MOTORES[i].pinoA, i * 2,     a);
  pwmEscrever(MOTORES[i].pinoB, i * 2 + 1, b);
}

void pararMotor(int i) {
  direcao[i] = 0;
  duty[i] = 0;
  aplicarMotor(i);
}

void pararTudo() {
  for (int i = 0; i < NUM_MOTORES; i++) pararMotor(i);
}

// ------------------------------------------------------------
String estadoJson() {
  String s = "{\"v\":" + String(velocidade) + ",\"j\":[";
  for (int i = 0; i < NUM_MOTORES; i++) {
    if (i) s += ",";
    s += "{\"n\":\"" + String(MOTORES[i].nome) + "\"" +
         ",\"m\":\"" + String(MOTORES[i].botaoMenos) + "\"" +
         ",\"p\":\"" + String(MOTORES[i].botaoMais) + "\"" +
         ",\"d\":"   + String(direcao[i]) +
         ",\"t\":"   + (travado[i] ? "true" : "false") + "}";
  }
  return s + "]}";
}

void tratarMensagem(const char* msg) {
  int j, v;
  unsigned long agora = millis();
  switch (msg[0]) {
    case 'm':
      if (sscanf(msg, "m,%d,%d", &j, &v) == 2 && j >= 0 && j < NUM_MOTORES) {
        v = constrain(v, -1, 1);
        if (v == 0) { pararMotor(j); travado[j] = false; break; }
        if (travado[j]) { ultimoCmd[j] = agora; break; }   // ainda segurando: ignora
        if (v != direcao[j]) {               // começou a girar (ou trocou o sentido)
          duty[j] = 0;
          inicioGiro[j] = agora;
          direcao[j] = v;
        }
        ultimoCmd[j] = agora;
      }
      break;
    case 'v':
      if (sscanf(msg, "v,%d", &v) == 1) velocidade = constrain(v, VELOCIDADE_MIN, 100);
      break;
    case 'x':
      pararTudo();
      Serial.println("PARAR acionado");
      break;
    default:   // 'p' (ping)
      break;
  }
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Garra hidraulica ESP32 (motores DC) ===");

  for (int i = 0; i < NUM_MOTORES; i++) {
    pwmIniciar(MOTORES[i].pinoA, i * 2);
    pwmIniciar(MOTORES[i].pinoB, i * 2 + 1);
    pararMotor(i);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_SENHA);
  Serial.printf("Rede: %s  |  Senha: %s\n", WIFI_SSID, WIFI_SENHA);
  Serial.print("Abra no celular: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", PAGINA_HTML); });
  server.on("/icon.svg", []() {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send(200, "image/svg+xml", ICONE_SVG);
  });
  server.on("/manifest.json", []() { server.send(200, "application/manifest+json", MANIFESTO_JSON); });
  server.on("/cmd", []() {
    if (server.hasArg("c")) {            // vários comandos separados por ';'
      String c = server.arg("c");
      int ini = 0;
      while (ini < (int)c.length() && ini < 200) {
        int fim = c.indexOf(';', ini);
        if (fim < 0) fim = c.length();
        tratarMensagem(c.substring(ini, fim).c_str());
        ini = fim + 1;
      }
    }
    server.send(200, "application/json", estadoJson());
  });
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long t = 0;
  unsigned long agora = millis();
  if (agora - t < 10) return;
  t = agora;

  float alvo = velocidade / 100.0f;
  float passo = alvo * 10.0f / RAMPA_MS;

  for (int i = 0; i < NUM_MOTORES; i++) {
    // Destrava quando o botão foi solto (comandos pararam de chegar)
    if (travado[i] && agora - ultimoCmd[i] > TEMPO_SEM_COMANDO_MS) travado[i] = false;
    if (direcao[i] == 0) continue;

    // Segurança: botão solto / sinal perdido
    if (agora - ultimoCmd[i] > TEMPO_SEM_COMANDO_MS) {
      pararMotor(i);
      continue;
    }
    // Segurança: girando tempo demais sem soltar
    if (agora - inicioGiro[i] > TEMPO_MAX_MS) {
      Serial.printf("%s: tempo maximo atingido, parado\n", MOTORES[i].nome);
      pararMotor(i);
      travado[i] = true;
      continue;
    }
    // Partida suave
    duty[i] = min(alvo, duty[i] + passo);
    aplicarMotor(i);
  }
}
