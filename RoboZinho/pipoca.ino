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

// ---------- Sons (frequência Hz, duração ms) ----------
struct Nota { uint16_t freq; uint16_t dur; };
const Nota MEL_FELIZ[] = {{1000, 80}, {1300, 80}, {1700, 140}};
const Nota MEL_TONTO[] = {{700, 90}, {500, 90}, {700, 90}};
const Nota MEL_BRAVO[] = {{300, 130}, {250, 130}, {300, 180}};
const Nota MEL_SUSTO[] = {{1500, 80}, {1000, 80}, {500, 220}};
const Nota MEL_SONO[]  = {{600, 160}, {450, 200}, {300, 300}};
const Nota MEL_AI[]    = {{1800, 60}, {1200, 90}};
const Nota MEL_TEDIO[] = {{500, 200}, {420, 260}};

#define TOCAR(m) tocar(m, sizeof(m) / sizeof(m[0]))

const Nota *melAtual = nullptr;
byte melTam = 0;
byte melIdx = 0;
unsigned long melProx = 0;

void tocar(const Nota *m, byte n) {
  melAtual = m;
  melTam = n;
  melIdx = 0;
  melProx = 0;
}

void atualizarSom() {
  if (!melAtual || millis() < melProx) return;
  if (melIdx >= melTam) {
    melAtual = nullptr;
    return;
  }
  tone(BUZZER_PIN, melAtual[melIdx].freq, melAtual[melIdx].dur);
  melProx = millis() + melAtual[melIdx].dur + 25;
  melIdx++;
}

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

void limparExtra() {
  roboEyes.setCyclops(OFF);
  roboEyes.setMood(DEFAULT);
  roboEyes.setWidth(36, 36);
  roboEyes.setHeight(36, 36);
  extraAtivo = 0;
}

void mudarEstado(Estado novo) {
  estado = novo;
  posAtual = -1;
  Serial.print("Estado: ");
  Serial.println(NOMES[novo]);

  // Limpa efeitos do estado anterior.
  roboEyes.setHFlicker(OFF);
  roboEyes.setVFlicker(OFF);
  roboEyes.setSweat(OFF);
  roboEyes.setCyclops(OFF);
  roboEyes.setWidth(36, 36);
  roboEyes.setHeight(36, 36);
  roboEyes.setIdleMode(OFF);
  roboEyes.setPosition(DEFAULT);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.open();
  extraAtivo = 0;
  proxAleatorio = millis() + random(12000, 25000);

  switch (novo) {
    case ST_NORMAL:
      roboEyes.setMood(DEFAULT);
      roboEyes.setIdleMode(ON, 2, 2);
      ultimoMovimento = millis();
      break;

    case ST_TONTO:
      roboEyes.setMood(DEFAULT);
      roboEyes.anim_confused();
      proxConfuso = millis() + 1500;
      TOCAR(MEL_TONTO);
      break;

    case ST_BRAVO:
      roboEyes.setMood(ANGRY);
      roboEyes.setHFlicker(ON, 2);
      TOCAR(MEL_BRAVO);
      break;

    case ST_AI:
      roboEyes.setMood(ANGRY);
      roboEyes.setWidth(40, 40);
      roboEyes.setHeight(22, 22);
      roboEyes.setHFlicker(ON, 3);
      fimEstado = millis() + TEMPO_AI;
      TOCAR(MEL_AI);
      break;

    case ST_SUSTO:
      roboEyes.setMood(DEFAULT);
      roboEyes.setSweat(ON);
      roboEyes.setWidth(46, 46);
      roboEyes.setHeight(46, 46);
      roboEyes.anim_confused();
      fimEstado = millis() + TEMPO_SUSTO;
      TOCAR(MEL_SUSTO);
      break;

    case ST_INVERTIDO:
      roboEyes.setMood(TIRED);
      roboEyes.setVFlicker(ON, 2);
      roboEyes.anim_confused();
      proxConfuso = millis() + 1500;
      TOCAR(MEL_TONTO);
      break;

    case ST_FELIZ:
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();
      fimEstado = millis() + TEMPO_FELIZ;
      TOCAR(MEL_FELIZ);
      break;

    case ST_ENTEDIADO:
      roboEyes.setMood(TIRED);
      roboEyes.setIdleMode(ON, 4, 4);
      TOCAR(MEL_TEDIO);
      break;

    case ST_SONO:
      roboEyes.setMood(TIRED);
      roboEyes.setAutoblinker(OFF);
      roboEyes.close();
      TOCAR(MEL_SONO);
      break;

    case ST_ACORDANDO:
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();
      fimEstado = millis() + TEMPO_ACORDAR;
      TOCAR(MEL_FELIZ);
      break;
  }
}

// Expressão aleatória de vez em quando, só no modo normal.
void expressaoAleatoria() {
  switch (random(6)) {
    case 0:
      roboEyes.anim_laugh();
      break;
    case 1:
      roboEyes.anim_confused();
      break;
    case 2:
      roboEyes.setCyclops(ON);
      extraAtivo = 1;
      fimExtra = millis() + 2500;
      break;
    case 3:
      roboEyes.setMood(HAPPY);
      extraAtivo = 2;
      fimExtra = millis() + 1800;
      break;
    case 4:
      roboEyes.setWidth(44, 44);
      roboEyes.setHeight(44, 44);
      extraAtivo = 3;
      fimExtra = millis() + 1500;
      break;
    default:
      roboEyes.blink();
      break;
  }
  proxAleatorio = millis() + random(12000, 25000);
}

// Converte a inclinação (em g) para a direção do olhar.
int direcaoOlhar(float gx, float gy) {
  int h = 0, v = 0;
  if (gy > LIMITE_INCLINACAO) h = 1;
  else if (gy < -LIMITE_INCLINACAO) h = -1;
  if (gx > LIMITE_INCLINACAO) v = 1;
  else if (gx < -LIMITE_INCLINACAO) v = -1;

  if (v == -1) return (h == -1) ? NW : (h == 1) ? NE : N;
  if (v == 1) return (h == -1) ? SW : (h == 1) ? SE : S;
  return (h == -1) ? W : (h == 1) ? E : DEFAULT;
}

void lerEAtualizar() {
  mpu6050.update();

  float ax = mpu6050.getAccX();
  float ay = mpu6050.getAccY();
  float az = mpu6050.getAccZ();
  float forca = sqrt(ax * ax + ay * ay + az * az);

  // Quanto o sensor mudou desde a última leitura.
  float delta = fabs(ax - axAnt) + fabs(ay - ayAnt) + fabs(az - azAnt);
  axAnt = ax;
  ayAnt = ay;
  azAnt = az;

  // Irritação sobe com balanço e desce sozinha com o tempo.
  float dt = INTERVALO_LEITURA / 1000.0;
  float agitAntes = agitacao;
  if (delta > RUIDO) agitacao += (delta - RUIDO) * GANHO;
  agitacao -= DECAIMENTO * dt;
  if (agitacao < 0) agitacao = 0;
  if (agitacao > 12) agitacao = 12;

  if (delta > 0.12) ultimoMovimento = millis();
  quedaContagem = (forca < LIMITE_QUEDA) ? quedaContagem + 1 : 0;
  invertidoContagem = (az < LIMITE_INVERTIDO) ? invertidoContagem + 1 : 0;

  // Embalo suave (carinho): movimento pequeno e constante.
  if (delta >= 0.10 && delta < 0.25 && agitacao < 1.0) embalo++;
  else if (embalo > 0) embalo--;

  // Estados com duração fixa.
  if (estado == ST_SUSTO || estado == ST_ACORDANDO ||
      estado == ST_AI || estado == ST_FELIZ) {
    if (millis() > fimEstado) {
      agitacao = 0;
      embalo = 0;
      mudarEstado(ST_NORMAL);
    }
    return;
  }

  // Queda livre.
  if (quedaContagem >= 2) {
    quedaContagem = 0;
    mudarEstado(ST_SUSTO);
    return;
  }

  // Dormindo: acorda se mexerem.
  if (estado == ST_SONO) {
    if (delta > 0.15) mudarEstado(ST_ACORDANDO);
    return;
  }

  // De cabeça para baixo.
  if (estado == ST_INVERTIDO) {
    if (az > -0.3) {
      mudarEstado(ST_NORMAL);
    } else if (millis() > proxConfuso) {
      roboEyes.anim_confused();
      proxConfuso = millis() + 1500;
    }
    return;
  }
  if (invertidoContagem >= 6) {
    invertidoContagem = 0;
    mudarEstado(ST_INVERTIDO);
    return;
  }

  // Pancada isolada: movimento brusco com o robô calmo.
  if (delta > LIMITE_PANCADA && agitAntes < 1.0) {
    agitacao = agitAntes;
    mudarEstado(ST_AI);
    return;
  }

  // Carinho / embalo.
  if (embalo >= 20 && (estado == ST_NORMAL || estado == ST_ENTEDIADO)) {
    embalo = 0;
    mudarEstado(ST_FELIZ);
    return;
  }

  // Escalada da irritação (com histerese para não ficar trocando de estado).
  Estado novo;
  if (agitacao >= LIMITE_BRAVO) novo = ST_BRAVO;
  else if (estado == ST_BRAVO && agitacao >= LIMITE_BRAVO / 2) novo = ST_BRAVO;
  else if (agitacao >= LIMITE_TONTO) novo = ST_TONTO;
  else if (estado == ST_TONTO && agitacao >= LIMITE_TONTO / 2) novo = ST_TONTO;
  else novo = (estado == ST_ENTEDIADO) ? ST_ENTEDIADO : ST_NORMAL;

  if (novo != estado) {
    mudarEstado(novo);
    return;
  }

  // Tonto: repete a animação enquanto continua balançando.
  if (estado == ST_TONTO && millis() > proxConfuso) {
    roboEyes.anim_confused();
    proxConfuso = millis() + 1500;
  }

  // Entediado volta ao normal se mexerem.
  if (estado == ST_ENTEDIADO && millis() - ultimoMovimento < TEMPO_ENTEDIADO) {
    mudarEstado(ST_NORMAL);
    return;
  }

  // Parado por muito tempo: entedia e depois dorme.
  if (estado == ST_NORMAL || estado == ST_ENTEDIADO) {
    unsigned long parado = millis() - ultimoMovimento;
    if (parado > TEMPO_SONO) {
      mudarEstado(ST_SONO);
      return;
    }
    if (estado == ST_NORMAL && parado > TEMPO_ENTEDIADO) {
      mudarEstado(ST_ENTEDIADO);
      return;
    }
  }

  if (estado == ST_NORMAL) {
    // Termina a expressão aleatória em andamento.
    if (extraAtivo && millis() > fimExtra) limparExtra();

    // Olhos acompanham a inclinação.
    float gx = INVERTER_X ? -ax : ax;
    float gy = INVERTER_Y ? -ay : ay;
    int dir = direcaoOlhar(gx, gy);
    if (dir != posAtual) {
      posAtual = dir;
      if (dir == DEFAULT) {
        roboEyes.setIdleMode(ON, 2, 2);
        roboEyes.setPosition(DEFAULT);
      } else {
        roboEyes.setIdleMode(OFF);
        roboEyes.setPosition(dir);
      }
    }

    // De vez em quando, uma expressão aleatória.
    if (!extraAtivo && millis() > proxAleatorio) expressaoAleatoria();
  }
}

// ---------- Extras desenhados por cima dos olhos ----------
void coracao(int x, int y) {
  display.fillCircle(x - 2, y, 2, SSD1306_WHITE);
  display.fillCircle(x + 2, y, 2, SSD1306_WHITE);
  display.fillTriangle(x - 4, y + 1, x + 4, y + 1, x, y + 6, SSD1306_WHITE);
}

void desenharExtras() {
  unsigned long t = millis();

  switch (estado) {
    case ST_BRAVO: {
      // Símbolo de raiva pulsando no canto superior direito.
      int x = 112, y = 9, g = (t / 200) % 2;
      display.drawFastHLine(x - 6 - g, y - 6 - g, 4, SSD1306_WHITE);
      display.drawFastVLine(x - 6 - g, y - 6 - g, 4, SSD1306_WHITE);
      display.drawFastHLine(x + 3 + g, y - 6 - g, 4, SSD1306_WHITE);
      display.drawFastVLine(x + 6 + g, y - 6 - g, 4, SSD1306_WHITE);
      display.drawFastHLine(x - 6 - g, y + 6 + g, 4, SSD1306_WHITE);
      display.drawFastVLine(x - 6 - g, y + 3 + g, 4, SSD1306_WHITE);
      display.drawFastHLine(x + 3 + g, y + 6 + g, 4, SSD1306_WHITE);
      display.drawFastVLine(x + 6 + g, y + 3 + g, 4, SSD1306_WHITE);
      break;
    }

    case ST_TONTO:
    case ST_INVERTIDO: {
      // Estrelinhas girando em volta da cabeça.
      float ang = t / 120.0;
      for (int i = 0; i < 3; i++) {
        float a = ang + i * 2.094;
        display.fillCircle(64 + (int)(26 * cos(a)), 7 + (int)(4 * sin(a)), 1, SSD1306_WHITE);
      }
      break;
    }

    case ST_SUSTO:
      // Exclamação (o suor já vem da biblioteca).
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(2, 2);
      display.print("!");
      break;

    case ST_AI: {
      // Estrela de impacto.
      const int raios[8][2] = {{6, 0}, {-6, 0}, {0, 6}, {0, -6}, {4, 4}, {-4, -4}, {4, -4}, {-4, 4}};
      for (int i = 0; i < 8; i++)
        display.drawLine(110, 10, 110 + raios[i][0], 10 + raios[i][1], SSD1306_WHITE);
      break;
    }

    case ST_FELIZ:
    case ST_ACORDANDO: {
      // Corações subindo pelas laterais.
      unsigned long f = t / 35;
      coracao(10, 58 - (int)(f % 54));
      coracao(118, 58 - (int)((f + 27) % 54));
      break;
    }

    case ST_ENTEDIADO: {
      // Reticências "..." aparecendo.
      int n = (t / 400) % 4;
      for (int i = 0; i < n; i++) display.fillCircle(96 + i * 8, 6, 2, SSD1306_WHITE);
      break;
    }

    default:
      break;
  }
}

// Desenha um quadro: olhos + extras, enviando ao OLED uma única vez.
unsigned long ultimoFrame = 0;

void desenharFrame() {
  if (millis() - ultimoFrame < 15) return;
  ultimoFrame = millis();

  display.enviar = false;     // a biblioteca desenha, mas ainda não envia
  roboEyes.drawEyes();
  display.enviar = true;

  desenharExtras();
  display.display();          // envia olhos + extras juntos
}

// Desenha o robô dormindo: olhos fechados e "Zzz" animado.
unsigned long ultimoZzz = 0;

void desenharSono() {
  if (millis() - ultimoZzz < 300) return;
  ultimoZzz = millis();

  int fase = (millis() / 600) % 4;  // 0 a 3 letras

  display.clearDisplay();
  display.fillRoundRect(23, 32, 36, 4, 2, SSD1306_WHITE);  // olho esquerdo fechado
  display.fillRoundRect(69, 32, 36, 4, 2, SSD1306_WHITE);  // olho direito fechado
  display.setTextColor(SSD1306_WHITE);

  if (fase >= 1) {
    display.setTextSize(1);
    display.setCursor(108, 46);
    display.print("z");
  }
  if (fase >= 2) {
    display.setTextSize(2);
    display.setCursor(108, 26);
    display.print("Z");
  }
  if (fase >= 3) {
    display.setTextSize(3);
    display.setCursor(108, 0);
    display.print("Z");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  // OLED no barramento I2C principal.
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  // MPU-6050 em um segundo barramento I2C.
  I2C_MPU.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Falha no OLED SSD1306");
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.display();

  mpu6050.begin();
  delay(300);
  mpu6050.update();
  axAnt = mpu6050.getAccX();
  ayAnt = mpu6050.getAccY();
  azAnt = mpu6050.getAccZ();

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setCuriosity(ON);
  mudarEstado(ST_NORMAL);

  TOCAR(MEL_FELIZ);
}

void loop() {
  if (estado == ST_SONO) desenharSono();
  else desenharFrame();
  atualizarSom();

  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = millis();
    lerEAtualizar();
  }

  delay(5);
}
