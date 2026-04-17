#include <Arduino.h>
#include "kalibrasi_sensor.h"

// ──────────────────────────────────────
//  PIN
// ──────────────────────────────────────
#define SEL_A       25
#define SEL_B       26
#define BTN_KALIBR  27

// ──────────────────────────────────────
//  VARIABEL SENSOR
// ──────────────────────────────────────
int sensor[JUMLAH_SENSOR];
int sensorDigital[JUMLAH_SENSOR];  // 0=putih, 1=hitam


// ══════════════════════════════════════
//  FUNGSI: Baca semua sensor
// ══════════════════════════════════════
void bacaSensor() {
  digitalWrite(SEL_A, 0); digitalWrite(SEL_B, 0);
  delayMicroseconds(10);
  sensor[6]  = analogRead(32);
  sensor[8]  = analogRead(33);
  sensor[14] = analogRead(34);
  sensor[3]  = analogRead(35);

  digitalWrite(SEL_A, 1); digitalWrite(SEL_B, 0);
  delayMicroseconds(10);
  sensor[4]  = analogRead(32);
  sensor[9]  = analogRead(33);
  sensor[12] = analogRead(34);
  sensor[2]  = analogRead(35);

  digitalWrite(SEL_A, 0); digitalWrite(SEL_B, 1);
  delayMicroseconds(10);
  sensor[5]  = analogRead(32);
  sensor[11] = analogRead(33);
  sensor[13] = analogRead(34);
  sensor[0]  = analogRead(35);

  digitalWrite(SEL_A, 1); digitalWrite(SEL_B, 1);
  delayMicroseconds(10);
  sensor[7]  = analogRead(32);
  sensor[10] = analogRead(33);
  sensor[15] = analogRead(34);
  sensor[1]  = analogRead(35);
}


// ══════════════════════════════════════
//  FUNGSI: Ubah analog -> digital
// ══════════════════════════════════════
void ubahKeDigital() {
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    if (sensor[i] < threshold[i]) {
      sensorDigital[i] = 0;  // putih
    } else {
      sensorDigital[i] = 1;  // hitam
    }
  }
}


// ══════════════════════════════════════
//  SETUP
// ══════════════════════════════════════
void setup() {
  Serial.begin(115200);

  pinMode(SEL_A,      OUTPUT);
  pinMode(SEL_B,      OUTPUT);
  pinMode(BTN_KALIBR, INPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  initKalibrasi();

  Serial.println(">> Siap!");
  Serial.println(">> Tekan tombol D21 untuk mulai kalibrasi.");
}


// ══════════════════════════════════════
//  LOOP
// ══════════════════════════════════════
void loop() {

  // 1. Cek tombol (pull down: HIGH = ditekan)
  if (digitalRead(BTN_KALIBR) == HIGH) {

    if (tahap == 0) {
      // Tekan pertama → mulai kalibrasi HITAM
      mulaiKalibrasiHitam();
      delay(200);  // debounce

    } else if (tahap == 2) {
      // Tekan kedua → mulai kalibrasi PUTIH
      mulaiKalibrasiPutih();
      delay(300);  // debounce
    }
  }

  // 2. Proses kalibrasi (non-blocking)
  prosesKalibrasi();

  // 3. Baca dan tampilkan sensor (hanya saat idle)
  if (tahap == 0) {
    bacaSensor();

    if (sudahKalibrasi) {
      ubahKeDigital();
      Serial.print("DIG: ");
      for (int i = 0; i < JUMLAH_SENSOR; i++) {
        Serial.print(sensorDigital[i]);
        if (i < 15) Serial.print(" ");
      }
      Serial.println();

    } else {
      Serial.print("RAW: ");
      for (int i = 0; i < JUMLAH_SENSOR; i++) {
        Serial.print("S"); Serial.print(i);
        Serial.print(":"); Serial.print(sensor[i]);
        if (i < 15) Serial.print("  ");
      }
      Serial.println();
    }
  }

  delay(10);
}