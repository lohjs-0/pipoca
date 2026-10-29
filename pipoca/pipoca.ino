/*
  RoboZinho v3 - ESP32 + OLED SSD1306 + MPU6050 + FluxGarage_RoboEyes

  Só precisa do MPU6050 como sensor. Expressões:
    NORMAL      olhos passeiam, piscam e acompanham a inclinação
    TONTO       balanço leve (estrelinhas girando)
    BRAVO       balanço forte ou prolongado (símbolo de raiva)
    AI          pancada isolada (olhos apertados, estrela de impacto)
    SUSTO       queda livre (olhos grandes, "!" e suor)
    INVERTIDO   de cabeça para baixo (enjoado)
    FELIZ       embalo/carinho suave (ri, corações subindo)
    ENTEDIADO   parado por um tempo (olhar cansado, "...")
    SONO        parado por mais tempo (dorme com Zzz, acorda rindo)
    ALEATORIAS  de vez em quando: riso, confusão, um olho só,
                olhos felizes, olhos arregalados, piscadinha

  Bibliotecas:
    - Adafruit GFX Library
    - Adafruit SSD1306
    - MPU6050_tockn
    - FluxGarage_RoboEyes

  Ligações (ESP32 DevKit):
    OLED SDA = GPIO 21   |  OLED SCL = GPIO 22
    MPU  SDA = GPIO 26   |  MPU  SCL = GPIO 27
    Buzzer (+) = GPIO 23, (-) = GND
    VCC -> 3V3, GND -> GND

  Requer o core esp32 3.x (tone() disponível).
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050_tockn.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define MPU_SDA_PIN 26
#define MPU_SCL_PIN 27
#define BUZZER_PIN 23

// ---------- Estados (precisa ficar antes das funções) ----------
enum Estado {
  ST_NORMAL, ST_TONTO, ST_BRAVO, ST_SUSTO, ST_SONO,
  ST_ACORDANDO, ST_INVERTIDO, ST_FELIZ, ST_AI, ST_ENTEDIADO
};
const char *NOMES[] = {
  "NORMAL", "TONTO", "BRAVO", "SUSTO", "SONO",
  "ACORDANDO", "INVERTIDO", "FELIZ", "AI", "ENTEDIADO"
};

// OLED que deixa segurar o envio do quadro, para desenhar extras sem piscar.
class OLEDCustom : public Adafruit_SSD1306 {
 public:
  bool enviar = true;
  OLEDCustom(uint8_t w, uint8_t h, TwoWire *twi, int8_t rst)
    : Adafruit_SSD1306(w, h, twi, rst) {}
  void display() {
    if (enviar) Adafruit_SSD1306::display();
  }
};

OLEDCustom display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TwoWire I2C_MPU = TwoWire(1);
MPU6050 mpu6050(I2C_MPU);
RoboEyes<OLEDCustom> roboEyes(display);

// ================== AJUSTES ==================
const unsigned long INTERVALO_LEITURA = 50;   // ms entre leituras do MPU
const float RUIDO = 0.50;             // movimento abaixo disso é ignorado
const float GANHO = 1.0;              // quanto cada balanço aumenta a irritação
const float DECAIMENTO = 2.5;         // quanto a irritação cai por segundo
const float LIMITE_TONTO = 4.0;       // nível para ficar tonto
const float LIMITE_BRAVO = 9.0;       // nível para ficar bravo
const float LIMITE_PANCADA = 2.5;     // movimento brusco único = "ai!"
const float LIMITE_QUEDA = 0.40;      // força (g) abaixo disso = queda livre
const float LIMITE_INVERTIDO = -0.6;  // az abaixo disso = de cabeça para baixo
const float LIMITE_INCLINACAO = 0.35; // inclinação (g) para o olhar acompanhar
const unsigned long TEMPO_ENTEDIADO = 15000;  // parado = fica entediado
const unsigned long TEMPO_SONO = 30000;       // parado = dorme
const unsigned long TEMPO_SUSTO = 1500;
const unsigned long TEMPO_ACORDAR = 1800;
const unsigned long TEMPO_AI = 900;
const unsigned long TEMPO_FELIZ = 2500;
// Se o olhar for para o lado errado, troque para true:
const bool INVERTER_X = false;
const bool INVERTER_Y = false;
// =============================================

// ---------- Variáveis de controle ----------
Estado estado = ST_NORMAL;
float agitacao = 0;
float axAnt = 0, ayAnt = 0, azAnt = 1;
int quedaContagem = 0;
int invertidoContagem = 0;
int embalo = 0;
int posAtual = -1;
int extraAtivo = 0;   // 0 = nenhuma, 1 = um olho, 2 = feliz, 3 = arregalado
unsigned long fimEstado = 0;
unsigned long fimExtra = 0;
unsigned long proxConfuso = 0;
unsigned long proxAleatorio = 0;
unsigned long ultimoMovimento = 0;
unsigned long ultimaLeitura = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  pinMode(BUZZER_PIN, OUTPUT);

  // OLED no barramento I2C principal.
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  // MPU-6050 em um segundo barramento I2C.
  I2C_MPU.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("Falha no OLED SSD1306"));
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.display();

  // Verifica o endereço I2C do MPU6050 antes de iniciar a biblioteca.
  I2C_MPU.beginTransmission(0x68);
  uint8_t erroMPU = I2C_MPU.endTransmission();
  if (erroMPU != 0) {
    I2C_MPU.beginTransmission(0x69);
    erroMPU = I2C_MPU.endTransmission();
  }
  if (erroMPU != 0) {
    Serial.println(F("Falha: MPU6050 nao encontrado nos enderecos I2C 0x68/0x69."));
    while (true) delay(1000);
  }

  mpu6050.begin();
  delay(300);
  mpu6050.update();
  axAnt = mpu6050.getAccX();
  ayAnt = mpu6050.getAccY();
  azAnt = mpu6050.getAccZ();
  if (!isfinite(axAnt) || !isfinite(ayAnt) || !isfinite(azAnt)) {
    Serial.println(F("Falha: leituras invalidas do MPU6050."));
    while (true) delay(1000);
  }
  Serial.println(F("MPU6050 encontrado e inicializado."));

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setCuriosity(ON);
  ultimaLeitura = millis();
  mudarEstado(ST_NORMAL);

  TOCAR(MEL_FELIZ);
}

void loop() {
  if (estado == ST_SONO) desenharSono();
  else desenharFrame();
  atualizarSom();

  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    lerEAtualizar();
  }

  delay(5);
}
