import type { Metadata, Viewport } from "next";
import type { ReactNode } from "react";
import "./globals.css";

export const metadata: Metadata = {
  title: "Hydra | Controle do guindaste",
  description: "Painel de controle local para o guindaste hidráulico com ESP32.",
  applicationName: "Hydra",
};

export const viewport: Viewport = {
  themeColor: "#111512",
  width: "device-width",
  initialScale: 1,
  maximumScale: 1,
  userScalable: false,
};

export default function RootLayout({
  children,
}: Readonly<{
  children: ReactNode;
}>) {
  return (
    <html lang="pt-BR">
      <body>{children}</body>
    </html>
  );
}
