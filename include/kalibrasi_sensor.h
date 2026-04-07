#ifndef KALIBRASI_SENSOR_H
#define KALIBRASI_SENSOR_H

#include <Arduino.h>

// ──────────────────────────────────────
//  KONSTANTA KALIBRASI
// ──────────────────────────────────────
#define JUMLAH_SENSOR 16
#define DURASI_KALIBR 10000  // 10 detik

// ──────────────────────────────────────
//  ALAMAT EEPROM
// ──────────────────────────────────────
#define ADDR_MIN_PUTIH  0
#define ADDR_MAX_PUTIH  32
#define ADDR_MIN_HITAM  64
#define ADDR_MAX_HITAM  96
#define ADDR_THRESHOLD  128
#define ADDR_FLAG       160

// ──────────────────────────────────────
//  VARIABEL GLOBAL KALIBRASI
// ──────────────────────────────────────
extern int minPutih[JUMLAH_SENSOR];
extern int maxPutih[JUMLAH_SENSOR];
extern int minHitam[JUMLAH_SENSOR];
extern int maxHitam[JUMLAH_SENSOR];
extern int threshold[JUMLAH_SENSOR];
extern int tahap;
extern bool sudahKalibrasi;
extern unsigned long waktuMulaiKalibr;

// ──────────────────────────────────────
//  DEKLARASI FUNGSI
// ──────────────────────────────────────
void hitungThreshold();
void simpanEEPROM();
void muatEEPROM();
void prosesKalibrasi();
void mulaiKalibrasiHitam();
void mulaiKalibrasiPutih();
void initKalibrasi();

#endif