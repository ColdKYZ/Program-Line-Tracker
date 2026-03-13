#include <Arduino.h>
#include "sensor_config.cpp"

// Pin selector MUX
#define SEL_A  5
#define SEL_B  18

// Hasil baca 16 sensor
int sensor[16];

void bacaSensor() {
  // Diurutkan dari kiri ke kanan sesuai posisi fisik sensor, bukan urutan pembacaan MUX
  // ──── ch = 0 : SEL_A=0, SEL_B=0 ────
  digitalWrite(SEL_A, 0);
  digitalWrite(SEL_B, 0);
  delayMicroseconds(10);
  sensor[6]  = analogRead(32);  // (physical sensor 0)
  sensor[8]  = analogRead(33);  // (physical sensor 1)
  sensor[14] = analogRead(34);  // (physical sensor 2)
  sensor[3]  = analogRead(35);  // (physical sensor 3)

  // ──── ch = 1 : SEL_A=1, SEL_B=0 ────
  digitalWrite(SEL_A, 1);
  digitalWrite(SEL_B, 0);
  delayMicroseconds(10);
  sensor[4]  = analogRead(32);  // (physical sensor 4)
  sensor[9]  = analogRead(33);  // (physical sensor 5)
  sensor[12] = analogRead(34);  // (physical sensor 6)
  sensor[2]  = analogRead(35);  // (physical sensor 7) ← paling KIRI

  // ──── ch = 2 : SEL_A=0, SEL_B=1 ────
  digitalWrite(SEL_A, 0);
  digitalWrite(SEL_B, 1);
  delayMicroseconds(10);
  sensor[5]  = analogRead(32);  // (physical sensor 8)
  sensor[11]  = analogRead(33);  // (physical sensor 9)
  sensor[13] = analogRead(34);  // (physical sensor 10)
  sensor[0]  = analogRead(35);  // (physical sensor 11)

  // ──── ch = 3 : SEL_A=1, SEL_B=1 ────
  digitalWrite(SEL_A, 1);
  digitalWrite(SEL_B, 1);
  delayMicroseconds(10);
  sensor[7]  = analogRead(32);  // (physical sensor 12)
  sensor[10] = analogRead(33);  // (physical sensor 13)
  sensor[15] = analogRead(34);  // (physical sensor 14) ← paling KANAN
  sensor[1]  = analogRead(35);  // (physical sensor 15) 
}

void setup() {
  Serial.begin(115200);

  pinMode(SEL_A, OUTPUT);
  pinMode(SEL_B, OUTPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  bacaSensor();

  // Tampilkan sensor sesuai urutan fisik (kiri ke kanan)
  for (int i = 0; i < 16; i++) {
    Serial.print("S"); Serial.print(i);
    Serial.print(":"); Serial.print(sensor[i]);
    if (i < 15) Serial.print("  ");
  }
  Serial.println();

  delay(200);
}