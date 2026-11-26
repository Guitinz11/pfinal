import { NextResponse } from "next/server";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const ESP32_URL = (process.env.ESP32_URL ?? "http://192.168.4.1").replace(
  /\/+$/,
  "",
);
const COMMAND_PATTERN =
  /^(?:p|x|v,(?:[3-9]\d|100)|m,[0-3],(?:-1|0|1))(?:;(?:p|x|v,(?:[3-9]\d|100)|m,[0-3],(?:-1|0|1)))*$/;

export async function POST(request: Request) {
  let body: { commands?: unknown };

  try {
    body = await request.json();
  } catch {
    return NextResponse.json({ error: "Corpo JSON inválido." }, { status: 400 });
  }

  const { commands } = body;
  if (
    typeof commands !== "string" ||
    commands.length > 200 ||
    !COMMAND_PATTERN.test(commands)
  ) {
    return NextResponse.json(
      { error: "Comando de controle inválido." },
      { status: 400 },
    );
  }

  try {
    const upstream = await fetch(
      `${ESP32_URL}/cmd?c=${encodeURIComponent(commands)}`,
      {
        cache: "no-store",
        signal: AbortSignal.timeout(1200),
      },
    );
    const payload = await upstream.json();

    return NextResponse.json(payload, { status: upstream.status });
  } catch {
    return NextResponse.json(
      { error: "Não foi possível alcançar o ESP32 na rede local." },
      { status: 502 },
    );
  }
}
