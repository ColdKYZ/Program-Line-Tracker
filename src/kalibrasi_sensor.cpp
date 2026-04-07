#include "kalibrasi_sensor.h"
#include <EEPROM.h>

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

// Deklarasi fungsi bacaSensor dari main.cpp
extern void bacaSensor();
extern int sensor[JUMLAH_SENSOR];

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
//  FUNGSI: Mulai kalibrasi hitam
// ══════════════════════════════════════
void mulaiKalibrasiHitam() {
  Serial.println("\n>> [1/2] Letakkan sensor di atas HITAM...");
  for (int i = 0; i < JUMLAH_SENSOR; i++) { 
    minHitam[i] = 4095; 
    maxHitam[i] = 0; 
  }
  tahap = 1;
  waktuMulaiKalibr = millis();
}

// ══════════════════════════════════════
//  FUNGSI: Mulai kalibrasi putih
// ══════════════════════════════════════
void mulaiKalibrasiPutih() {
  Serial.println("\n>> [2/2] Letakkan sensor di atas PUTIH...");
  for (int i = 0; i < JUMLAH_SENSOR; i++) { 
    minPutih[i] = 4095; 
    maxPutih[i] = 0; 
  }
  tahap = 3;
  waktuMulaiKalibr = millis();
}

// ══════════════════════════════════════
//  FUNGSI: Inisialisasi kalibrasi
// ══════════════════════════════════════
void initKalibrasi() {
  #define EEPROM_SIZE 256
  EEPROM.begin(EEPROM_SIZE);
  muatEEPROM();
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