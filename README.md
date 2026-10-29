# Pip0ca

Um robozinho de mesa com olhos animados que reage ao movimento. Feito com ESP32, tela OLED e sensor MPU6050.

<!-- Coloque aqui uma foto ou GIF do robô: ![RoboZinho](docs/foto.jpg) -->

## O que ele faz

Usa só o acelerômetro do MPU6050 para reagir:

| Situação | Reação |
|---|---|
| Parado em pé | Olhos piscam, passeiam e acompanham a inclinação |
| Balanço leve | Fica tonto (estrelinhas girando) |
| Balanço forte | Fica bravo (símbolo de raiva) e se acalma aos poucos |
| Pancada isolada | "Ai!" (olhos apertados) |
| Queda livre | Susto (olhos grandes, "!" e suor) |
| De cabeça para baixo | Enjoado |
| Embalo suave | Feliz (corações subindo) |
| Parado por 15 s | Entediado |
| Parado por 30 s | Dorme (Zzz) e acorda rindo |
| De vez em quando | Expressões aleatórias (riso, um olho só, piscadinha...) |

Cada emoção tem um som próprio no buzzer.

## Hardware

- ESP32 DevKit
- Display OLED SSD1306 128x64 (I2C)
- Módulo MPU6050 (GY-521)
- Buzzer passivo
- Protoboard e jumpers

## Ligações

| Peça | Pino do módulo | GPIO da ESP32 |
|---|---|---|
| OLED | SDA / SCL | 21 / 22 |
| MPU6050 | SDA / SCL | 26 / 27 |
| Buzzer | (+) | 23 |
| Todos | VCC / GND | 3V3 / GND |

Os pinos podem ser trocados no começo do arquivo `RoboZinho/RoboZinho.ino`.

## Bibliotecas

- Adafruit GFX Library
- Adafruit SSD1306
- MPU6050_tockn
- FluxGarage RoboEyes

Core da placa: `esp32:esp32` versão 3.x.

## Como instalar (arduino-cli)

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" "MPU6050_tockn" "FluxGarage RoboEyes"

arduino-cli compile --fqbn esp32:esp32:esp32 RoboZinho
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 RoboZinho
```

No Linux, seu usuário precisa estar no grupo da porta serial (`uucp` no Arch, `dialout` no Debian/Ubuntu).

## Testar o sensor

A pasta `TesteMPU` tem um sketch que mostra no Serial se o MPU6050 foi encontrado e a força em g.

## Ajustes

No bloco `AJUSTES` do `pipoca.ino` dá para mudar a sensibilidade (`RUIDO`, `GANHO`, `LIMITE_TONTO`, `LIMITE_BRAVO`), os tempos de tédio e sono e a direção do olhar.

## Licença

Distribuído sob a **GNU GPL v3** (veja o arquivo `LICENSE`), porque usa a biblioteca FluxGarage RoboEyes, que é GPL v3.

## Créditos

- [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes), de Dennis Hoelscher
- Adafruit GFX e Adafruit SSD1306
- MPU6050_tockn
