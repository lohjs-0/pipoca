/*
  TesteMPU - ESP32 + MPU6050
  Mostra no Serial se o MPU foi achado e a força em g.

  Ligações: MPU SDA = GPIO 26, SCL = GPIO 27, VCC -> 3V3, GND -> GND
  Biblioteca: MPU6050_tockn
*/

#include <Wire.h>
#include <MPU6050_tockn.h>

#define MPU_SDA_PIN 26
#define MPU_SCL_PIN 27

TwoWire I2C_MPU = TwoWire(1);
MPU6050 mpu6050(I2C_MPU);

unsigned long ultimoPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  I2C_MPU.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  // Procura o MPU no barramento (endereço esperado: 0x68 ou 0x69).
  I2C_MPU.beginTransmission(0x68);
  bool achou68 = (I2C_MPU.endTransmission() == 0);
  I2C_MPU.beginTransmission(0x69);
  bool achou69 = (I2C_MPU.endTransmission() == 0);

  if (achou68) Serial.println("MPU6050 ENCONTRADO em 0x68 - OK");
  else if (achou69) Serial.println("MPU6050 ENCONTRADO em 0x69 (mude o endereco no codigo)");
  else {
    Serial.println("MPU6050 NAO ENCONTRADO - confira SDA/SCL, VCC e GND");
    while (true) delay(1000);
  }

  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  Serial.println("Pronto. Parado deve marcar perto de 1.00 g. Balance para ver subir.");
}

void loop() {
  mpu6050.update();

  if (millis() - ultimoPrint > 150) {
    ultimoPrint = millis();
    float ax = mpu6050.getAccX();
    float ay = mpu6050.getAccY();
    float az = mpu6050.getAccZ();
    float forca = sqrt(ax * ax + ay * ay + az * az);

    Serial.print("forca (g): ");
    Serial.print(forca, 2);
    if (forca > 2.8) Serial.print("  <-- IRRITADO!");
    Serial.println();
  }
}
