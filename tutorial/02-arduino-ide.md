# 2. Preparar a Arduino IDE

[← Materiais](01-materiais.md) · [Índice](../README.md) · [Próximo: gravar o firmware →](03-gravar-firmware.md)

O firmware **não usa nenhuma biblioteca extra**. Só é preciso instalar o suporte à placa ESP32.

## 2.1 Instalar a IDE

Baixe a **Arduino IDE 2** em <https://www.arduino.cc/en/software> e instale.

## 2.2 Adicionar a placa ESP32 (só na primeira vez)

1. **Arquivo → Preferências**.
2. Em **"URLs adicionais para Gerenciadores de Placas"**, cole:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **OK**.
4. **Ferramentas → Placa → Gerenciador de Placas**, procure **esp32** e instale
   **"esp32 by Espressif Systems"**. O download é grande, pode demorar.

## 2.3 Driver USB (se a porta não aparecer)

Olhe o chip pequeno perto do conector USB do ESP32:

| Escrito no chip | Driver |
|---|---|
| CP2102 / CP2104 | **CP210x** (Silicon Labs) |
| CH340 / CH9102 | **CH340** ou **CH343** (WCH) |

Para conferir: **Windows + X → Gerenciador de Dispositivos**, ligue o ESP32 e veja.

| O que aparece | Significa |
|---|---|
| **Portas (COM e LPT)** → "CP210x (COM3)" ou "CH340 (COM4)" | Tudo certo |
| **Outros dispositivos** com triângulo amarelo | Falta o driver |
| Nada muda | O cabo só carrega, troque o cabo |

> Em computador de escola, instalar driver normalmente pede senha de administrador.
> Peça ao técnico, ou grave em outro computador: o programa fica salvo no ESP32.
