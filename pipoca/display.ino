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

