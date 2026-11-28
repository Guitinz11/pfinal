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

type TractorState = {
  v: number;
  j: MotorState[];
};

const MOTORS = [
  { index: 0, name: "Base", side: "BASE" },
  { index: 1, name: "Ombro", side: "OMBRO" },
  { index: 2, name: "Cotovelo", side: "COTOVELO" },
  { index: 3, name: "Garra", side: "GARRA" },
] as const;

function Arrow({ direction }: { direction: "back" | "forward" }) {
  return (
    <svg
      className={`drive-arrow drive-arrow-${direction}`}
      viewBox="0 0 24 24"
      aria-hidden="true"
    >
      <path d="M5 12h14m-6-6 6 6-6 6" />
    </svg>
  );
}

export default function Home() {
  const [tractor, setTractor] = useState<TractorState | null>(null);
  const [connection, setConnection] = useState<"connecting" | "online" | "offline">(
    "connecting",
  );
  const [speed, setSpeed] = useState(70);
  const [activeMotors, setActiveMotors] = useState<Record<number, number>>({});
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
          const state = (await response.json()) as TractorState;
          if (!Array.isArray(state.j) || state.j.length < 2 || typeof state.v !== "number") {
            throw new Error("Resposta inválida do ESP32");
          }
          setTractor(state);
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

  const releaseMotor = useCallback(
    (motor: number, direction?: number) => {
      const heldDirection = heldRef.current[motor];
      if (heldDirection === undefined || (direction !== undefined && heldDirection !== direction)) {
        return;
      }
      delete heldRef.current[motor];
      setActiveMotors({ ...heldRef.current });
      transmit(`m,${motor},0`, true);
    },
    [transmit],
  );

  const stopAll = useCallback(() => {
    heldRef.current = {};
    setActiveMotors({});
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

  const beginMotor = (motor: number, direction: number) => {
    if (heldRef.current[motor] !== undefined || connection !== "online") return;
    heldRef.current[motor] = direction;
    setActiveMotors({ ...heldRef.current });
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
      beginMotor(motor, direction);
    }
  };

  const onControlKeyUp = (
    event: KeyboardEvent<HTMLButtonElement>,
    motor: number,
    direction: number,
  ) => {
    if (event.key === " " || event.key === "Enter") {
      event.preventDefault();
      releaseMotor(motor, direction);
    }
  };

  return (
    <main className="dashboard">
      <header className="topbar">
        <a className="brand" href="#" aria-label="Garra hidráulica, início">
          <span className="brand-icon" aria-hidden="true">
            <svg viewBox="0 0 32 32" fill="none">
              <circle cx="10" cy="22" r="5" />
              <circle cx="23" cy="23" r="3.5" />
              <path d="M7 17h15l4 4h-3M13 17l2-8h7l3 8M16 9h6M5 27h22" />
            </svg>
          </span>
          <span className="brand-name">GARRA<span>CTRL</span></span>
          <span className="brand-divider" />
          <span className="brand-caption">PAINEL DE PILOTAGEM</span>
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
          <span className="connection-network">WI-FI</span>
        </div>
      </header>

      <div className="page-heading">
        <div>
          <span className="section-kicker">PAINEL DE PILOTAGEM</span>
          <h1>Controle da garra hidráulica</h1>
          <p>Comande cada movimento separadamente. Segure para mover e solte para parar.</p>
        </div>
        <div className="page-facts" aria-label="Informações do sistema">
          <span><strong>04</strong> MOTORES</span>
          <i />
          <span><strong>ESP32</strong> WI-FI LOCAL</span>
        </div>
      </div>

      <section className="drive-panel" aria-label="Controles dos motores da garra hidráulica">
        <div className="section-heading">
          <div>
            <span className="section-kicker">CONTROLE INDEPENDENTE</span>
            <h2>Movimentação</h2>
          </div>
          <p><span className="hold-indicator" /> SEGURE PARA MOVER</p>
        </div>

        <div className="motor-list">
          {MOTORS.map((motor) => {
            const state = tractor?.j[motor.index];
            const direction = activeMotors[motor.index];
            const active = direction !== undefined;
            const locked = state?.t ?? false;
            const status = locked
              ? "AGUARDANDO"
              : active
                ? direction > 0
                  ? "AVANÇANDO"
                  : "RECUANDO"
                : "PRONTO";

            return (
              <article className={`motor-row${active ? " motor-active" : ""}`} key={motor.index}>
                <div className="motor-info">
                  <span className="motor-indicator" aria-hidden="true">
                    <svg viewBox="0 0 32 32" fill="none">
                      <circle cx="16" cy="16" r="11" />
                      <circle cx="16" cy="16" r="4" />
                      <path d="M16 5v7m0 8v7M5 16h7m8 0h7M8 8l5 5m6 6 5 5m0-16-5 5m-6 6-5 5" />
                    </svg>
                  </span>
                  <div>
                    <span className="motor-side">{motor.side}</span>
                    <h3>{state?.n ?? motor.name}</h3>
                  </div>
                  <span className={`motor-status${active ? " motor-status-active" : ""}`}>
                    <i />{status}
                  </span>
                </div>
                <div className="motor-controls">
                  {([-1, 1] as const).map((driveDirection) => {
                    const isForward = driveDirection === 1;
                    const label = isForward
                      ? (state?.p ?? "Avançar")
                      : (state?.m ?? "Recuar");
                    return (
                      <button
                        key={driveDirection}
                        type="button"
                        className={`drive-button${active && direction === driveDirection ? " drive-button-active" : ""}`}
                        aria-label={`${motor.name}: ${label.toLowerCase()}`}
                        disabled={connection !== "online"}
                        onPointerDown={(event) => {
                          event.preventDefault();
                          event.currentTarget.setPointerCapture(event.pointerId);
                          beginMotor(motor.index, driveDirection);
                        }}
                        onPointerUp={() => releaseMotor(motor.index, driveDirection)}
                        onPointerCancel={() => releaseMotor(motor.index, driveDirection)}
                        onLostPointerCapture={() => releaseMotor(motor.index, driveDirection)}
                        onKeyDown={(event) => onControlKeyDown(event, motor.index, driveDirection)}
                        onKeyUp={(event) => onControlKeyUp(event, motor.index, driveDirection)}
                        onContextMenu={(event) => event.preventDefault()}
                      >
                        <Arrow direction={isForward ? "forward" : "back"} />
                        <span>{label}</span>
                      </button>
                    );
                  })}
                </div>
              </article>
            );
          })}
        </div>

        <div className="drive-tools">
          <div className="speed-control">
            <div className="speed-heading">
              <div>
                <span className="section-kicker">VELOCIDADE</span>
                <strong>{speed}<small>%</small></strong>
              </div>
              <span className="speed-hint">Ajuste dos motores</span>
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
          </div>
          <button type="button" className="stop-button" onClick={stopAll}>
            <svg viewBox="0 0 24 24" aria-hidden="true"><rect x="5" y="5" width="14" height="14" rx="2" /></svg>
            <span>PARAR TUDO</span>
            <kbd>ESC</kbd>
          </button>
        </div>
      </section>

      <p className="drive-tip">
        Segure um movimento para operar o eixo correspondente.
        Solte o botão ou use PARAR TUDO para interromper todos os motores.
      </p>

      <footer className="footer">
        <span>GARRACTRL <i>·</i> ESP32</span>
        <span className="footer-safety"><i /> SOLTE O BOTÃO PARA PARAR</span>
      </footer>
    </main>
  );
}
