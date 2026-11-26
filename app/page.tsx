"use client";

import {
  type CSSProperties,
  type KeyboardEvent,
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

type MotorState = {
  n: string;
  m: string;
  p: string;
  d: number;
  t: boolean;
};

type CraneState = {
  v: number;
  j: MotorState[];
};

type Axis = {
  name: string;
  subtitle: string;
  icon: "base" | "arm" | "reach" | "claw";
};

const AXES: Axis[] = [
  { name: "Giro da base", subtitle: "BASE", icon: "base" },
  { name: "Elevação do braço", subtitle: "OMBRO", icon: "arm" },
  { name: "Extensão do braço", subtitle: "COTOVELO", icon: "reach" },
  { name: "Abertura da garra", subtitle: "GARRA", icon: "claw" },
];

function CraneIllustration() {
  return (
    <svg
      className="crane-illustration"
      viewBox="0 0 360 245"
      fill="none"
      role="img"
      aria-label="Ilustração de um guindaste hidráulico com seringas"
    >
      <defs>
        <linearGradient id="arm" x1="120" y1="110" x2="290" y2="55">
          <stop stopColor="#f4a83a" />
          <stop offset="1" stopColor="#ffd27b" />
        </linearGradient>
        <linearGradient id="cylinder" x1="145" y1="160" x2="238" y2="95">
          <stop stopColor="#b6d5bd" stopOpacity=".25" />
          <stop offset="1" stopColor="#d9efe0" stopOpacity=".8" />
        </linearGradient>
      </defs>
      <path d="M37 213h286" stroke="#58645a" strokeWidth="5" strokeLinecap="round" />
      <path d="M81 202h203l-15 17H96z" fill="#222923" stroke="#637066" strokeWidth="2" />
      <path d="M120 200v-49l34-29 43 29v49" fill="#26302a" stroke="#778276" strokeWidth="3" />
      <path d="M154 151 115 73l11-8 56 77" fill="url(#arm)" stroke="#ffce76" strokeWidth="3" strokeLinejoin="round" />
      <path d="m166 145 99-92 10 10-97 105" fill="url(#arm)" stroke="#ffce76" strokeWidth="3" strokeLinejoin="round" />
      <path d="m134 116 33 31-16 12-29-35z" fill="url(#cylinder)" stroke="#d4e4d1" strokeWidth="2" />
      <path d="m195 119 48-44 10 10-46 46z" fill="url(#cylinder)" stroke="#d4e4d1" strokeWidth="2" />
      <path d="m131 124 25 22M200 128l44-40" stroke="#91be9e" strokeWidth="4" strokeLinecap="round" />
      <circle cx="166" cy="148" r="13" fill="#f3ae47" stroke="#151a16" strokeWidth="5" />
      <circle cx="166" cy="148" r="22" stroke="#f3ae47" strokeOpacity=".24" strokeWidth="2" />
      <path d="m264 54 18-21 25 2-17 11 15 7-16 18" stroke="#f4b64d" strokeWidth="4" strokeLinecap="round" strokeLinejoin="round" />
      <path d="M113 72h13M263 53l10 11" stroke="#fff0d3" strokeWidth="3" strokeLinecap="round" />
      <path d="M91 202v-57M91 145h20" stroke="#718076" strokeWidth="4" strokeLinecap="round" />
      <circle cx="121" cy="201" r="5" fill="#91a092" />
      <circle cx="262" cy="201" r="5" fill="#91a092" />
      <path d="M61 230h238" stroke="#354039" strokeWidth="2" strokeDasharray="4 8" />
    </svg>
  );
}

function AxisIcon({ type }: { type: Axis["icon"] }) {
  if (type === "base") {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M4 19h16M7 16h10l-2-3H9l-2 3Zm5-3V8m0 0-3 3m3-3 3 3M5 6a9 9 0 0 1 14 0" />
      </svg>
    );
  }
  if (type === "arm" || type === "reach") {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M4 19h16M6 17l5-8 5 3 4-6M9 12l5 4m-3-7 2-3m5 0 2 1" />
        <circle cx="11" cy="9" r="1.5" />
      </svg>
    );
  }
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <path d="M8 4v7l4 4 4-4V4M8 11l-3 4m11-4 3 4M9 19h6" />
    </svg>
  );
}

function Arrow({ direction }: { direction: "left" | "right" }) {
  return (
    <svg
      viewBox="0 0 24 24"
      aria-hidden="true"
      className={direction === "left" ? "arrow arrow-left" : "arrow"}
    >
      <path d="M5 12h14m-6-6 6 6-6 6" />
    </svg>
  );
}

export default function Home() {
  const [crane, setCrane] = useState<CraneState | null>(null);
  const [connection, setConnection] = useState<"connecting" | "online" | "offline">(
    "connecting",
  );
  const [speed, setSpeed] = useState(70);
  const [activeAxes, setActiveAxes] = useState<Record<number, number>>({});
  const heldRef = useRef<Record<number, number>>({});
  const requestRef = useRef<AbortController | null>(null);
  const busyRef = useRef(false);
  const pendingRef = useRef<string | null>(null);
  const lastPingRef = useRef(0);

  const transmit = useCallback((commands: string, urgent = false) => {
    if (urgent) {
      requestRef.current?.abort();
      pendingRef.current = commands;
    } else if (busyRef.current) {
      pendingRef.current = commands;
      return;
    }

    if (busyRef.current) return;
    busyRef.current = true;

    const sendNext = (current: string) => {
      const controller = new AbortController();
      requestRef.current = controller;
      fetch("/api/control", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ commands: current }),
        cache: "no-store",
        signal: controller.signal,
      })
        .then(async (response) => {
          if (!response.ok) throw new Error("ESP32 indisponível");
          const state = (await response.json()) as CraneState;
          if (!Array.isArray(state.j) || typeof state.v !== "number") {
            throw new Error("Resposta inválida do ESP32");
          }
          setCrane(state);
          setSpeed(state.v);
          setConnection("online");
        })
        .catch(() => {
          if (!controller.signal.aborted) setConnection("offline");
        })
        .finally(() => {
          if (requestRef.current === controller) requestRef.current = null;
          const pending = pendingRef.current;
          pendingRef.current = null;
          if (pending) {
            sendNext(pending);
          } else {
            busyRef.current = false;
          }
        });
    };

    sendNext(commands);
  }, []);

  const releaseAxis = useCallback(
    (motor: number, direction?: number) => {
      const heldDirection = heldRef.current[motor];
      if (heldDirection === undefined || (direction !== undefined && heldDirection !== direction)) {
        return;
      }
      delete heldRef.current[motor];
      setActiveAxes({ ...heldRef.current });
      transmit(`m,${motor},0`, true);
    },
    [transmit],
  );

  const stopAll = useCallback(() => {
    heldRef.current = {};
    setActiveAxes({});
    transmit("x", true);
  }, [transmit]);

  useEffect(() => {
    setConnection("connecting");
    transmit("p");
    const timer = window.setInterval(() => {
      const active = Object.entries(heldRef.current);
      if (active.length) {
        transmit(active.map(([motor, direction]) => `m,${motor},${direction}`).join(";"));
      } else if (Date.now() - lastPingRef.current > 900) {
        lastPingRef.current = Date.now();
        transmit("p");
      }
    }, 150);

    const stopWhenLeaving = () => {
      if (Object.keys(heldRef.current).length) {
        heldRef.current = {};
        navigator.sendBeacon(
          "/api/control",
          new Blob([JSON.stringify({ commands: "x" })], {
            type: "application/json",
          }),
        );
      }
    };
    const stopWhenHidden = () => {
      if (document.hidden) stopAll();
    };
    const stopOnEscape = (event: globalThis.KeyboardEvent) => {
      if (event.key === "Escape") stopAll();
    };

    window.addEventListener("pagehide", stopWhenLeaving);
    document.addEventListener("visibilitychange", stopWhenHidden);
    window.addEventListener("blur", stopWhenLeaving);
    window.addEventListener("keydown", stopOnEscape);
    return () => {
      window.clearInterval(timer);
      window.removeEventListener("pagehide", stopWhenLeaving);
      document.removeEventListener("visibilitychange", stopWhenHidden);
      window.removeEventListener("blur", stopWhenLeaving);
      window.removeEventListener("keydown", stopOnEscape);
      if (Object.keys(heldRef.current).length) stopAll();
    };
  }, [stopAll, transmit]);

  const beginAxis = (motor: number, direction: number) => {
    if (heldRef.current[motor] !== undefined || connection !== "online") return;
    heldRef.current[motor] = direction;
    setActiveAxes({ ...heldRef.current });
    if (navigator.vibrate) navigator.vibrate(12);
    transmit(`m,${motor},${direction}`, true);
  };

  const changeSpeed = (value: number) => {
    setSpeed(value);
    transmit(`v,${value}`);
  };

  const onControlKeyDown = (
    event: KeyboardEvent<HTMLButtonElement>,
    motor: number,
    direction: number,
  ) => {
    if (event.key === " " || event.key === "Enter") {
      event.preventDefault();
      beginAxis(motor, direction);
    }
  };

  const onControlKeyUp = (
    event: KeyboardEvent<HTMLButtonElement>,
    motor: number,
    direction: number,
  ) => {
    if (event.key === " " || event.key === "Enter") {
      event.preventDefault();
      releaseAxis(motor, direction);
    }
  };

  return (
    <main className="shell">
      <header className="topbar">
        <a className="brand" href="#" aria-label="Hydra, início">
          <span className="brand-symbol">
            <svg viewBox="0 0 32 32" fill="none" aria-hidden="true">
              <path d="M6 25h20M10 22v-7l5-4 5 4v7m-5-11V6m0 0-3 3m3-3 3 3m2 3 5-5m-2 0 2 0 0 2" />
              <circle cx="15" cy="15" r="2" />
            </svg>
          </span>
          <span className="brand-name">hydra<span>.</span></span>
          <span className="brand-divider" />
          <span className="brand-caption">PAINEL DE CONTROLE</span>
        </a>
        <div className={`connection connection-${connection}`} role="status" aria-live="polite">
          <span className="connection-dot" />
          <span>
            {connection === "online"
              ? "ESP32 conectado"
              : connection === "offline"
                ? "Sem conexão"
                : "Conectando"}
          </span>
          <span className="connection-wifi">LOCAL</span>
        </div>
      </header>

      <section className="hero">
        <div className="hero-copy">
          <div className="eyebrow"><span /> CONTROLE HIDRÁULICO</div>
          <h1>Precisão em<br />cada movimento.</h1>
          <p>
            Controle as seringas e mova seu guindaste com suavidade.
            Pressione para mover, solte para parar.
          </p>
          <div className="hero-meta">
            <div className="meta-item">
              <span className="meta-icon">01</span>
              <span><strong>4 eixos</strong><small>controle independente</small></span>
            </div>
            <span className="meta-separator" />
            <div className="meta-item">
              <span className="meta-icon meta-icon-wifi">⌁</span>
              <span><strong>Conexão local</strong><small>sem internet</small></span>
            </div>
          </div>
        </div>
        <div className="hero-art-wrap">
          <span className="art-glow" />
          <div className="art-label art-label-top"><span /> CIRCUITO HIDRÁULICO</div>
          <CraneIllustration />
          <div className="art-label art-label-bottom"><span className="art-line" /> ESP32 · SERINGAS · MOVIMENTO</div>
          <span className="art-coordinate">24° 18&apos; 06.2&quot; S</span>
        </div>
        <div className="hero-index">H / 01</div>
      </section>

      <div className="section-title">
        <div>
          <span className="section-kicker">ESTAÇÃO DE COMANDO</span>
          <h2>Movimentos</h2>
        </div>
        <p><span className="hold-indicator" /> MANTENHA PRESSIONADO PARA MOVER</p>
      </div>

      <section className="controls-grid" aria-label="Controles do guindaste">
        {AXES.map((axis, index) => {
          const state = crane?.j[index];
          const direction = activeAxes[index];
          const active = direction !== undefined;
          const locked = state?.t ?? false;
          const status = locked
            ? "SOLTE O BOTÃO"
            : active
              ? direction > 0
                ? state?.p.toUpperCase() ?? "MOVENDO"
                : state?.m.toUpperCase() ?? "MOVENDO"
              : "PRONTO";

          return (
            <article className={`axis-card${active ? " axis-active" : ""}`} key={axis.subtitle}>
              <div className="axis-card-head">
                <span className="axis-icon"><AxisIcon type={axis.icon} /></span>
                <div className="axis-heading">
                  <span>{axis.subtitle} <i> / 0{index + 1}</i></span>
                  <h3>{axis.name}</h3>
                </div>
                <span className={`axis-status${active ? " status-active" : ""}${locked ? " status-locked" : ""}`}>
                  <i />{status}
                </span>
              </div>
              <div className="axis-controls">
                <button
                  type="button"
                  className={`direction-button${active && direction < 0 ? " direction-active" : ""}`}
                  aria-label={`${axis.name}: ${state?.m ?? "direção negativa"}`}
                  disabled={connection !== "online"}
                  onPointerDown={(event) => {
                    event.preventDefault();
                    event.currentTarget.setPointerCapture(event.pointerId);
                    beginAxis(index, -1);
                  }}
                  onPointerUp={() => releaseAxis(index, -1)}
                  onPointerCancel={() => releaseAxis(index, -1)}
                  onLostPointerCapture={() => releaseAxis(index, -1)}
                  onKeyDown={(event) => onControlKeyDown(event, index, -1)}
                  onKeyUp={(event) => onControlKeyUp(event, index, -1)}
                  onContextMenu={(event) => event.preventDefault()}
                >
                  <Arrow direction="left" />
                  <span>{state?.m ?? "Voltar"}</span>
                </button>
                <button
                  type="button"
                  className={`direction-button${active && direction > 0 ? " direction-active" : ""}`}
                  aria-label={`${axis.name}: ${state?.p ?? "direção positiva"}`}
                  disabled={connection !== "online"}
                  onPointerDown={(event) => {
                    event.preventDefault();
                    event.currentTarget.setPointerCapture(event.pointerId);
                    beginAxis(index, 1);
                  }}
                  onPointerUp={() => releaseAxis(index, 1)}
                  onPointerCancel={() => releaseAxis(index, 1)}
                  onLostPointerCapture={() => releaseAxis(index, 1)}
                  onKeyDown={(event) => onControlKeyDown(event, index, 1)}
                  onKeyUp={(event) => onControlKeyUp(event, index, 1)}
                  onContextMenu={(event) => event.preventDefault()}
                >
                  <span>{state?.p ?? "Avançar"}</span>
                  <Arrow direction="right" />
                </button>
              </div>
              <div className="axis-card-footer">
                <span>ACIONAMENTO POR SERINGA</span>
                <span className={active ? "footer-active" : ""}>
                  <i /> {active ? "ATIVO" : "EM ESPERA"}
                </span>
              </div>
            </article>
          );
        })}
      </section>

      <section className="bottom-grid">
        <div className="speed-card">
          <div className="bottom-card-heading">
            <span className="bottom-icon">
              <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 17a8 8 0 1 1 16 0M12 13l4-4M7 17h10" /><circle cx="12" cy="17" r="1" /></svg>
            </span>
            <div><span className="section-kicker">AJUSTE DO SISTEMA</span><h3>Velocidade</h3></div>
            <strong className="speed-number">{speed}<small>%</small></strong>
          </div>
          <label className="sr-only" htmlFor="speed">Velocidade dos motores</label>
          <input
            id="speed"
            className="speed-slider"
            type="range"
            min="30"
            max="100"
            step="5"
            value={speed}
            disabled={connection !== "online"}
            style={{ "--range-progress": `${((speed - 30) / 70) * 100}%` } as CSSProperties}
            onChange={(event) => changeSpeed(Number(event.target.value))}
          />
          <div className="speed-range"><span>30% · SUAVE</span><span>100% · MÁXIMA</span></div>
        </div>

        <div className="safety-card">
          <div className="safety-copy">
            <span className="safety-overline"><span /> CONTROLE DE SEGURANÇA</span>
            <h3>Interromper<br />movimentos</h3>
            <p>Envia um comando de parada imediata para os quatro motores.</p>
          </div>
          <button type="button" className="stop-button" onClick={stopAll}>
            <svg viewBox="0 0 24 24" aria-hidden="true"><rect x="5" y="5" width="14" height="14" rx="2" /></svg>
            <span>PARAR TUDO</span>
            <kbd>ESC</kbd>
          </button>
        </div>
      </section>

      <footer className="footer">
        <span><span className="footer-mark">H</span> HYDRA <i>·</i> GUINDASTE HIDRÁULICO</span>
        <span className="footer-safety"><span /> SOLTE O CONTROLE PARA PARAR · LIMITE DE 6 SEGUNDOS POR MOVIMENTO</span>
      </footer>
    </main>
  );
}
