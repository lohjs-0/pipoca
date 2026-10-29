/*
  TesteMPU - ESP32 + MPU6050
  Detecta o MPU6050 em 0x68/0x69 e mostra a aceleração resultante em g.

  Ligações: MPU SDA = GPIO 26, SCL = GPIO 27, VCC -> 3V3, GND -> GND
  Biblioteca: MPU6050_tockn
*/

#include <Wire.h>
#include <MPU6050_tockn.h>
#include <math.h>

const uint8_t MPU_SDA_PIN = 26;
const uint8_t MPU_SCL_PIN = 27;
const uint8_t MPU_ADDR_68 = 0x68;
const uint8_t MPU_ADDR_69 = 0x69;
const unsigned long INTERVALO_PRINT_MS = 150;

TwoWire I2C_MPU = TwoWire(1);
MPU6050 *mpu6050 = nullptr;
uint8_t enderecoMPU = 0;
unsigned long ultimoPrint = 0;

bool responderNoEndereco(uint8_t endereco) {
  I2C_MPU.beginTransmission(endereco);
  return I2C_MPU.endTransmission() == 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  I2C_MPU.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  if (responderNoEndereco(MPU_ADDR_68)) {
    enderecoMPU = MPU_ADDR_68;
  } else if (responderNoEndereco(MPU_ADDR_69)) {
    enderecoMPU = MPU_ADDR_69;
  } else {
    Serial.println(F("MPU6050 nao encontrado em 0x68 nem 0x69."));
    Serial.println(F("Confira SDA/SCL, alimentacao 3V3, GND e as conexoes."));
    while (true) delay(1000);
  }

  Serial.print(F("MPU6050 encontrado em 0x"));
  Serial.println(enderecoMPU, HEX);

  // Passa o endereço detectado à biblioteca (útil quando AD0 seleciona 0x69).
  mpu6050 = new MPU6050(I2C_MPU, enderecoMPU);
  mpu6050->begin();
  Serial.println(F("Calibrando giroscopio: mantenha o sensor parado."));
  mpu6050->calcGyroOffsets(true);
  Serial.println(F("Pronto. Parado, a forca deve ficar perto de 1.00 g."));
  Serial.println(F("Balance o sensor para observar a variacao."));
}

void loop() {
  mpu6050->update();

  const unsigned long agora = millis();
  if (agora - ultimoPrint >= INTERVALO_PRINT_MS) {
    ultimoPrint = agora;

    const float ax = mpu6050->getAccX();
    const float ay = mpu6050->getAccY();
    const float az = mpu6050->getAccZ();

    if (!isfinite(ax) || !isfinite(ay) || !isfinite(az)) {
      Serial.println(F("Leitura invalida do MPU6050."));
      return;
    }

    const float forca = sqrtf(ax * ax + ay * ay + az * az);
    Serial.print(F("Forca (g): "));
    Serial.print(forca, 2);
    if (forca > 2.8f) Serial.print(F("  <-- movimento forte"));
    Serial.println();
  }
}
