#include <Arduino.h>
#include <EEPROM.h>

// ──────────────────────────────────────
//  PIN
// ──────────────────────────────────────
#define SEL_A       5
#define SEL_B       18
#define BTN_KALIBR  21

#define EEPROM_SIZE   256
#define JUMLAH_SENSOR 16
#define DURASI_KALIBR 10000  // 10 detik

// ──────────────────────────────────────
//  VARIABEL SENSOR
// ──────────────────────────────────────
int sensor[JUMLAH_SENSOR];
int sensorDigital[JUMLAH_SENSOR];  // 0=putih, 1=hitam

// ──────────────────────────────────────
//  VARIABEL KALIBRASI
// ──────────────────────────────────────
int minPutih[JUMLAH_SENSOR];
int maxPutih[JUMLAH_SENSOR];
int minHitam[JUMLAH_SENSOR];
int maxHitam[JUMLAH_SENSOR];
int threshold[JUMLAH_SENSOR];

// Tahap kalibrasi:
// 0 = idle
// 1 = sedang kalibrasi hitam
// 2 = menunggu tombol untuk kalibrasi putih
// 3 = sedang kalibrasi putih
int tahap = 0;

bool sudahKalibrasi = false;
unsigned long waktuMulaiKalibr = 0;

// ──────────────────────────────────────
//  ALAMAT EEPROM
// ──────────────────────────────────────
#define ADDR_MIN_PUTIH  0
#define ADDR_MAX_PUTIH  32
#define ADDR_MIN_HITAM  64
#define ADDR_MAX_HITAM  96
#define ADDR_THRESHOLD  128
#define ADDR_FLAG       160


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
//  FUNGSI: Hitung threshold
//  threshold = (rata putih + rata hitam) / 2
// ══════════════════════════════════════
void hitungThreshold() {
  Serial.println("\n>> Threshold tiap sensor:");
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    int rataPutih = (minPutih[i] + maxPutih[i]) / 2;
    int rataHitam = (minHitam[i] + maxHitam[i]) / 2;
    threshold[i]  = (rataPutih + rataHitam) / 2;

    Serial.print("  S"); Serial.print(i);
    Serial.print(": putih="); Serial.print(rataPutih);
    Serial.print("  hitam="); Serial.print(rataHitam);
    Serial.print("  threshold="); Serial.println(threshold[i]);
  }
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
//  FUNGSI: Simpan ke EEPROM
// ══════════════════════════════════════
void simpanEEPROM() {
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    EEPROM.write(ADDR_MIN_PUTIH + (i*2),     minPutih[i] >> 8);
    EEPROM.write(ADDR_MIN_PUTIH + (i*2) + 1, minPutih[i] & 0xFF);
    EEPROM.write(ADDR_MAX_PUTIH + (i*2),     maxPutih[i] >> 8);
    EEPROM.write(ADDR_MAX_PUTIH + (i*2) + 1, maxPutih[i] & 0xFF);
    EEPROM.write(ADDR_MIN_HITAM + (i*2),     minHitam[i] >> 8);
    EEPROM.write(ADDR_MIN_HITAM + (i*2) + 1, minHitam[i] & 0xFF);
    EEPROM.write(ADDR_MAX_HITAM + (i*2),     maxHitam[i] >> 8);
    EEPROM.write(ADDR_MAX_HITAM + (i*2) + 1, maxHitam[i] & 0xFF);
    EEPROM.write(ADDR_THRESHOLD + (i*2),     threshold[i] >> 8);
    EEPROM.write(ADDR_THRESHOLD + (i*2) + 1, threshold[i] & 0xFF);
  }
  EEPROM.write(ADDR_FLAG, 0xAB);
  EEPROM.commit();
  Serial.println(">> Tersimpan di EEPROM.");
}


// ══════════════════════════════════════
//  FUNGSI: Muat dari EEPROM
// ══════════════════════════════════════
void muatEEPROM() {
  if (EEPROM.read(ADDR_FLAG) != 0xAB) {
    Serial.println(">> EEPROM kosong, belum pernah kalibrasi.");
    return;
  }
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    threshold[i] = (EEPROM.read(ADDR_THRESHOLD + (i*2)) << 8)
                 |  EEPROM.read(ADDR_THRESHOLD + (i*2) + 1);
  }
  sudahKalibrasi = true;
  Serial.println(">> Kalibrasi dimuat dari EEPROM.");
}


// ══════════════════════════════════════
//  FUNGSI: Proses kalibrasi (non-blocking)
// ══════════════════════════════════════
void prosesKalibrasi() {
  if (tahap != 1 && tahap != 3) return;

  unsigned long sekarang = millis();

  // Masih dalam 10 detik → catat min & max
  if (sekarang - waktuMulaiKalibr < DURASI_KALIBR) {
    bacaSensor();

    for (int i = 0; i < JUMLAH_SENSOR; i++) {
      if (tahap == 1) {
        // Kalibrasi hitam
        if (sensor[i] < minHitam[i]) minHitam[i] = sensor[i];
        if (sensor[i] > maxHitam[i]) maxHitam[i] = sensor[i];
      } else {
        // Kalibrasi putih
        if (sensor[i] < minPutih[i]) minPutih[i] = sensor[i];
        if (sensor[i] > maxPutih[i]) maxPutih[i] = sensor[i];
      }
    }

    // Progress tiap 1 detik
    static unsigned long waktuPrint = 0;
    if (sekarang - waktuPrint >= 1000) {
      int sisa = (DURASI_KALIBR - (sekarang - waktuMulaiKalibr)) / 1000;
      Serial.print(">> Kalibrasi ");
      Serial.print(tahap == 1 ? "HITAM" : "PUTIH");
      Serial.print("... sisa "); Serial.print(sisa); Serial.println(" detik");
      waktuPrint = sekarang;
    }

  // 10 detik selesai
  } else {

    if (tahap == 1) {
      // ── Hitam selesai ──
      Serial.println("\n>> Kalibrasi HITAM selesai:");
      for (int i = 0; i < JUMLAH_SENSOR; i++) {
        Serial.print("  S"); Serial.print(i);
        Serial.print(": min="); Serial.print(minHitam[i]);
        Serial.print("  max="); Serial.println(maxHitam[i]);
      }
      tahap = 2;  // tunggu tombol untuk kalibrasi putih
      Serial.println("\n>> Tekan tombol D19 untuk mulai kalibrasi PUTIH.");

    } else {
      // ── Putih selesai → hitung threshold & simpan ──
      Serial.println("\n>> Kalibrasi PUTIH selesai:");
      for (int i = 0; i < JUMLAH_SENSOR; i++) {
        Serial.print("  S"); Serial.print(i);
        Serial.print(": min="); Serial.print(minPutih[i]);
        Serial.print("  max="); Serial.println(maxPutih[i]);
      }
      hitungThreshold();
      simpanEEPROM();
      sudahKalibrasi = true;
      tahap = 0;
      Serial.println(">> Kalibrasi lengkap! Robot siap jalan.\n");
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

  EEPROM.begin(EEPROM_SIZE);
  muatEEPROM();

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
      Serial.println("\n>> [1/2] Letakkan sensor di atas HITAM...");
      for (int i = 0; i < JUMLAH_SENSOR; i++) { minHitam[i] = 4095; maxHitam[i] = 0; }
      tahap = 1;
      waktuMulaiKalibr = millis();
      delay(200);  // debounce

    } else if (tahap == 2) {
      // Tekan kedua → mulai kalibrasi PUTIH
      Serial.println("\n>> [2/2] Letakkan sensor di atas PUTIH...");
      for (int i = 0; i < JUMLAH_SENSOR; i++) { minPutih[i] = 4095; maxPutih[i] = 0; }
      tahap = 3;
      waktuMulaiKalibr = millis();
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