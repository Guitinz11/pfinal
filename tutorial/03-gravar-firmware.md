# 3. Gravar o firmware

[← Arduino IDE](02-arduino-ide.md) · [Índice](../README.md) · [Próximo: ligação elétrica →](04-ligacao-eletrica.md)

O firmware é **um arquivo só**: [`firmware/garra_esp32/garra_esp32.ino`](../firmware/garra_esp32/garra_esp32.ino).
Ele contém a configuração, a página do celular e o programa.

## 3.1 Baixar

No GitHub: **Code → Download ZIP**, e **descompacte** o ZIP. Não abra o arquivo de dentro do ZIP.

## 3.2 Abrir

- Abra `firmware/garra_esp32/garra_esp32.ino` na Arduino IDE.
- A pasta precisa se chamar `garra_esp32`, igual ao arquivo. Se a IDE oferecer para mover o
  arquivo para uma pasta, pode aceitar.

> **Pasta de rede (computador de escola):** se a pasta estiver num caminho como
> `\\servidor\pastas alunos$\...`, a IDE mostra *"Não há suporte para caminhos UNC"* e pode
> falhar. Copie a pasta para o disco `C:`, **ou** abra o `.ino` no Bloco de Notas, copie tudo
> (Ctrl+A, Ctrl+C), crie um **Arquivo → Novo Sketch** na IDE e cole por cima.

## 3.3 Configurar e gravar

1. **Ferramentas → Placa → esp32 → ESP32 Dev Module**.
2. **Ferramentas → Porta** → a porta COM do ESP32. Se o menu estiver cinza, veja o
   [driver USB](02-arduino-ide.md#23-driver-usb-se-a-porta-não-aparecer).
3. **✓ Verificar**: deve terminar com algo como `Sketch uses 1023507 bytes (78%)`.
4. **→ Carregar**. Se travar em `Connecting....___`, segure o botão **BOOT** da placa até
   aparecer a porcentagem.

## 3.4 Conferir

1. **Ferramentas → Monitor Serial**, velocidade **115200**.
2. Aperte o botão **EN/RST** da placa. Deve aparecer:
   ```
   === Garra hidraulica ESP32 (motores DC) ===
   Rede: Garra-ESP  |  Senha: garra1234
   Abra no celular: http://192.168.4.1
   ```

## 3.5 O que dá para mudar

Tudo fica no começo do `.ino`, na seção **CONFIGURAÇÃO**:

| O quê | Onde |
|---|---|
| Nome e senha da rede | `WIFI_SSID`, `WIFI_SENHA` (senha com 8+ caracteres) |
| Pinos de cada motor | `pinoA`, `pinoB` na tabela `MOTORES` |
| Texto dos botões | `botaoMenos`, `botaoMais` |
| Motor girando ao contrário | `invertido` = `true` |
| Velocidade inicial | `VELOCIDADE_PADRAO` (%) |
| Tempo máximo segurando o botão | `TEMPO_MAX_MS` (padrão 6000 = 6 s) |
