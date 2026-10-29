# Pip0ca — robozinho de mesa

Um robozinho de mesa que reage ao movimento com expressões, animações e sons. O projeto usa uma placa ESP32, um display OLED SSD1306 e um sensor MPU6050.

> **Status:** projeto maker em desenvolvimento. Os comandos e conexões abaixo refletem a configuração do sketch; a compilação e o funcionamento devem ser validados na placa e nas versões de bibliotecas usadas por você.

<!-- Adicione uma foto ou GIF real do robô quando estiver disponível. Exemplo: ![Pipoca em funcionamento](docs/foto.jpg) -->

## O que ele faz

O sketch usa as leituras do acelerômetro do MPU6050 para classificar movimentos e controlar os olhos do robô.

| Situação | Reação |
| --- | --- |
| Parado em pé | Olhos piscam, passeiam e acompanham a inclinação |
| Balanço leve | Fica tonto, com estrelinhas girando |
| Balanço forte | Fica bravo e se acalma gradualmente |
| Pancada isolada | Reação de “Ai!” com olhos apertados |
| Queda livre | Susto com olhos grandes, exclamação e suor |
| De cabeça para baixo | Expressão de enjoo |
| Embalo suave | Fica feliz, com corações subindo |
| Parado por cerca de 15 s | Fica entediado |
| Parado por cerca de 30 s | Dorme com “Zzz” e acorda ao ser movimentado |
| De vez em quando | Expressões aleatórias |

As melodias são reproduzidas pelo buzzer. Os tempos e limiares podem ser ajustados no bloco `AJUSTES` de `pipoca.ino`.

## Estrutura do projeto

```
pipoca/
├── pipoca.ino       # setup(), loop(), configuração e variáveis compartilhadas
├── Estados.ino      # transições emocionais e interpretação das leituras do MPU6050
├── Sons.ino         # melodias e controle do buzzer
├── Display.ino      # renderização dos olhos, animações e elementos gráficos
├── TesteMPU/
│   └── TesteMPU.ino # sketch independente para verificar o MPU6050
├── README.md
└── README-ESTRUTURA.txt
```

Os arquivos `.ino` da pasta principal são abas do mesmo sketch e devem permanecer juntos na pasta `pipoca`. Na Arduino IDE, abra `pipoca.ino`. O sketch de `TesteMPU` é independente: abra `TesteMPU/TesteMPU.ino` separadamente.

## Hardware e ligações

A configuração do código é compatível com um **ESP32 DevKit**, mas o modelo/revisão exatos da placa usada no teste ainda precisam ser registrados.

| Componente | Pino do módulo | GPIO da ESP32 |
| --- | --- | --- |
| OLED SSD1306 | SDA | 21 |
| OLED SSD1306 | SCL | 22 |
| MPU6050 (GY-521) | SDA | 26 |
| MPU6050 (GY-521) | SCL | 27 |
| Buzzer passivo | sinal / positivo | 23 |
| Módulos | VCC | 3V3 |
| Módulos | GND | GND comum |

O OLED e o MPU6050 usam **barramentos I2C separados** no sketch: o OLED utiliza `Wire` nos GPIOs 21/22 e o MPU6050 utiliza `I2C_MPU` nos GPIOs 26/27. Não conecte SDA/SCL dos dois módulos como se fossem um único barramento sem também alterar a configuração do código.

Endereços I2C esperados:

- OLED SSD1306: `0x3C` (configurado em `OLED_ADDR`; alguns módulos podem usar outro endereço).

- MPU6050: `0x68` ou `0x69`, conforme o pino AD0 do módulo.

### Aviso elétrico do buzzer

O sketch gera o sinal no GPIO 23. Confirme que o buzzer é **passivo** e que sua corrente está dentro do limite elétrico permitido pelo ESP32. Não alimente um buzzer de maior corrente diretamente pelo GPIO: use um transistor/driver apropriado e alimentação compatível, mantendo o GND comum. Não exceda a tensão suportada pelos módulos; a configuração documentada usa 3,3 V.

## Requisitos de software

- Arduino IDE ou `arduino-cli`.

- Core Arduino para ESP32 (`esp32:esp32`), versão 3.x conforme indicado no sketch.

- Bibliotecas: Adafruit GFX Library, Adafruit SSD1306, MPU6050_tockn e FluxGarage RoboEyes.

As versões exatas das bibliotecas ainda não foram registradas como testadas. Para tornar a instalação reproduzível, anote as versões que funcionarem na sua placa e ambiente.

### Instalação e compilação com `arduino-cli`

Instale o core ESP32 e as bibliotecas necessárias. A biblioteca RoboEyes pode não estar disponível com exatamente esse nome no catálogo do `arduino-cli`; se o comando de instalação não a encontrar, instale-a pela Arduino IDE ou siga as instruções do repositório FluxGarage RoboEyes.

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SSD1306"
arduino-cli lib install "MPU6050_tockn"
```

Compile o sketch principal a partir do diretório que contém a pasta `pipoca`:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 pipoca
```

Compile o teste do sensor separadamente:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 pipoca/TesteMPU
```

Para enviar o firmware, substitua `/dev/ttyUSB0` pela porta serial correta:

```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 pipoca
```

No Linux, pode ser necessário conceder ao usuário acesso à porta serial, por exemplo adicionando-o ao grupo `dialout` em Debian/Ubuntu. O nome do grupo varia conforme a distribuição. Depois de alterar grupos, pode ser necessário encerrar e iniciar a sessão novamente.

## Testar o MPU6050

Abra `TesteMPU/TesteMPU.ino` como sketch independente, compile e envie para a placa. Abra o Serial Monitor em **115200 baud**. O teste procura o sensor em `0x68` e `0x69`, calibra o giroscópio e imprime a força resultante em g.

Durante a calibração, mantenha o sensor parado. Parado, a força resultante deve ficar próxima de `1.00 g`; ao movimentá-lo, o valor varia. O limite de `2.8 g` é apenas uma indicação no monitor serial do sketch de teste, não uma calibração universal de pancada.

Exemplo ilustrativo de saída:

```
MPU6050 encontrado em 0x68
Calibrando giroscopio: mantenha o sensor parado.
Pronto. Parado, a forca deve ficar perto de 1.00 g.
Forca (g): 1.01
Forca (g): 1.04
Forca (g): 3.12  <-- movimento forte
```

## Ajustes e calibração

No bloco `AJUSTES` de `pipoca.ino` você pode alterar, entre outros:

- `RUIDO`: ignora pequenas variações abaixo do limiar definido.

- `GANHO` e `DECAIMENTO`: controlam o aumento e a redução da agitação.

- `LIMITE_TONTO`, `LIMITE_BRAVO` e `LIMITE_PANCADA`: limiares de reação.

- `LIMITE_QUEDA` e `LIMITE_INVERTIDO`: critérios de queda livre e inversão.

- `TEMPO_ENTEDIADO`, `TEMPO_SONO` e demais durações: temporização das emoções.

- `INVERTER_X` e `INVERTER_Y`: sentido do acompanhamento do olhar.

Para calibrar, fixe o sensor na posição definitiva, verifique os eixos no teste serial, observe a leitura em repouso e faça movimentos controlados. Ajuste um limiar por vez e registre os valores testados. Os limiares atuais são iniciais e podem exigir ajuste de acordo com a montagem, vibração e orientação do sensor.

## Problemas comuns

| Sintoma | O que verificar |
| --- | --- |
| OLED não inicia | Alimentação, GND comum, SDA/SCL nos GPIOs 21/22 e endereço `0x3C` |
| MPU6050 não encontrado | Alimentação 3,3 V, GND, SDA/SCL nos GPIOs 26/27, conexão do barramento e endereço `0x68`/`0x69` |
| Leituras instáveis | Fixação mecânica, fios curtos, alimentação e orientação do sensor; rode o teste com o MPU imóvel |
| Força distante de 1 g em repouso | Confira orientação/unidade das leituras, calibração e se o sensor está realmente parado |
| Sem som ou ESP32 reinicia | Tipo e corrente do buzzer, driver/transistor, GND comum e conexão do GPIO 23 |
| Não compila | Confirme o core ESP32 3.x, instale as quatro bibliotecas e mantenha todas as abas `.ino` na mesma pasta `pipoca` |
| Falha ao enviar firmware | Verifique a porta serial, cabo USB de dados, permissões do sistema e placa selecionada |

## Licença

O projeto informa uso da licença **GNU GPL v3**. Consulte o arquivo `LICENSE` distribuído com o repositório para o texto completo e confirme que ele está presente antes de publicar ou redistribuir. A biblioteca FluxGarage RoboEyes também é licenciada sob GPL v3.

## Créditos

- [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes), de Dennis Hoelscher.

- Adafruit GFX e Adafruit SSD1306.

- MPU6050_tockn.

Feito por **[SEU NOME]** — substitua pelo nome ou identificação dos autores do projeto.