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
  Serial.print(F("Estado: "));
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
  if (!isfinite(ax) || !isfinite(ay) || !isfinite(az)) {
    Serial.println(F("Aviso: leitura invalida do MPU6050; amostra ignorada."));
    return;
  }
  float forca = sqrt(ax * ax + ay * ay + az * az);

  // Quanto o sensor mudou desde a última leitura.
  float delta = fabs(ax - axAnt) + fabs(ay - ayAnt) + fabs(az - azAnt);
  axAnt = ax;
  ayAnt = ay;
  azAnt = az;

  // Irritação sobe com balanço e desce sozinha com o tempo.
  unsigned long agora = millis();
  float dt = (agora - ultimaLeitura) / 1000.0f;
  ultimaLeitura = agora;
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
    if (prazoVencido(fimEstado)) {
      agitacao = 0;
      embalo = 0;
      mudarEstado(ST_NORMAL);
    }
    return;
  }

  // Queda livre.
  if (quedaContagem >= 4) {
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
    } else if (prazoVencido(proxConfuso)) {
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
  if (estado == ST_TONTO && prazoVencido(proxConfuso)) {
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
    if (extraAtivo && prazoVencido(fimExtra)) limparExtra();

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
    if (!extraAtivo && prazoVencido(proxAleatorio)) expressaoAleatoria();
  }
}

