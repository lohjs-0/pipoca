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

bool prazoVencido(unsigned long prazo) {
  return (int32_t)(millis() - prazo) >= 0;
}

void atualizarSom() {
  if (!melAtual || !prazoVencido(melProx)) return;
  if (melIdx >= melTam) {
    melAtual = nullptr;
    return;
  }
  tone(BUZZER_PIN, melAtual[melIdx].freq, melAtual[melIdx].dur);
  melProx = millis() + melAtual[melIdx].dur + 25;
  melIdx++;
}

