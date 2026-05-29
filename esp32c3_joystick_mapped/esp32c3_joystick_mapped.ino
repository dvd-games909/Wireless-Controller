#include <ESP32_NOW.h>
#include <WiFi.h>

// ============================================================
//  ESP32-C3 SuperMini  –  4 assi joystick + 2 pulsanti
//  Output: -255 / +255 con zona morta e calibrazione centro
// ============================================================

#define PIN_JOY1_X 5
#define PIN_JOY1_Y 4
#define PIN_JOY2_X 7
#define PIN_JOY2_Y 6
#define PIN_BUT1 2
#define PIN_BUT2 1

#define OVERSAMPLING 16  // campioni per media ADC
#define PRINT_MS 100     // intervallo stampa (ms)
#define BAUD_RATE 115200

#define DEADZONE 10        // zona morta attorno al centro (unità -255/+255)
#define CALIB_SAMPLES 100  // campioni per calibrazione centro al boot

typedef struct {
  int16_t M1;
  int16_t M2;
  int16_t M3;
  int16_t M4;
} Payload;

Payload datiDaInviare;

// Centri calibrati per ogni asse (calcolati al boot)
int centro[4];

uint8_t receiverMac[] = { 0x30, 0xAE, 0xA4, 0x0D, 0xBA, 0xD8 };  // il MAC  Address del dispositivo ricevente

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) { delay(10); }

  WiFi.mode(WIFI_STA);
  esp_now_init();

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  pinMode(PIN_BUT1, INPUT);
  pinMode(PIN_BUT2, INPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("# ESP32-C3 SuperMini – Joystick -255/+255");
  calibra();
}

void loop() {
  int j1x = mappaAsse(leggiADC(PIN_JOY1_X), centro[0]);
  int j1y = -mappaAsse(leggiADC(PIN_JOY1_Y), centro[1]);
  int j2x = mappaAsse(leggiADC(PIN_JOY2_X), centro[2]);
  int j2y = -mappaAsse(leggiADC(PIN_JOY2_Y), centro[3]);

  bool but1 = !digitalRead(PIN_BUT1);
  bool but2 = !digitalRead(PIN_BUT2);

  // Output Serial Plotter
  Serial.print("J1X:");
  Serial.print(j1x);
  Serial.print("\tJ1Y:");
  Serial.print(j1y);
  Serial.print("\tJ2X:");
  Serial.print(j2x);
  Serial.print("\tJ2Y:");
  Serial.print(j2y);
  Serial.print("\tBUT1:");
  Serial.print(but1);
  Serial.print("\tBUT2:");
  Serial.println(but2);

  calcolaMotori(j2y, j2x, j1x, but2, but1);

  Serial.print("\tM1: ");
  Serial.print(datiDaInviare.M1);
  Serial.print("\tM2: ");
  Serial.print(datiDaInviare.M2);
  Serial.print("\tM3: ");
  Serial.print(datiDaInviare.M3);
  Serial.print("\tM4: ");
  Serial.println(datiDaInviare.M4);

  esp_now_send(receiverMac, (uint8_t *)&datiDaInviare, sizeof(datiDaInviare));

  delay(PRINT_MS);
}
void calcolaMotori(int16_t avantIndietro, int16_t rotazione,
                   int16_t slide, bool btnDriftDx, bool btnDriftSx) {


  // Cinematica ruote Mecanum configurazione X
  float m1 = avantIndietro + rotazione + slide;
  float m2 = avantIndietro - rotazione - slide;
  float m3 = avantIndietro - rotazione + slide;
  float m4 = avantIndietro + rotazione - slide;

  if (btnDriftDx) {
    m3 += 200;
    m4 -= 200;
  }
  else if (btnDriftSx) {
    m3 -= 200;
    m4 += 200;
  }

  // Normalizzazione proporzionale (mantiene i rapporti tra i motori)
  float massimo = max({ abs(m1), abs(m2), abs(m3), abs(m4) });
  if (massimo > 255) {
    float scala = 255.0 / massimo;
    m1 *= scala;
    m2 *= scala;
    m3 *= scala;
    m4 *= scala;
  }

  // Assegna alla struct
  datiDaInviare.M1 = (int16_t)m1;
  datiDaInviare.M2 = (int16_t)m2;
  datiDaInviare.M3 = (int16_t)m3;
  datiDaInviare.M4 = (int16_t)m4;
}

// ---- Lettura ADC con media ----
int leggiADC(int pin) {
  long somma = 0;
  for (int i = 0; i < OVERSAMPLING; i++) {
    somma += analogRead(pin);
    delayMicroseconds(100);
  }
  return (int)(somma / OVERSAMPLING);
}

// ---- Calibrazione centro al boot ----
// Legge CALIB_SAMPLES volte a riposo e calcola la media per ogni asse
void calibra() {
  Serial.println("# Calibrazione in corso, non toccare i joystick...");
  long sum[4] = { 0, 0, 0, 0 };
  int pins[4] = { PIN_JOY1_X, PIN_JOY1_Y, PIN_JOY2_X, PIN_JOY2_Y };

  for (int i = 0; i < CALIB_SAMPLES; i++) {
    for (int j = 0; j < 4; j++) {
      sum[j] += leggiADC(pins[j]);
    }
    delay(20);
  }
  for (int j = 0; j < 4; j++) {
    centro[j] = (int)(sum[j] / CALIB_SAMPLES);
  }

  Serial.printf("# Centri calibrati: J1X=%d  J1Y=%d  J2X=%d  J2Y=%d\n",
                centro[0], centro[1], centro[2], centro[3]);
  Serial.println("# Calibrazione completata, inizio lettura.");
  Serial.println("# ------------------------------------------------");
}

// ---- Mappa raw ADC → -255/+255 con zona morta ----
// Usa il centro calibrato e mappa separatamente
// il lato negativo (0→centro) e positivo (centro→4095)
int mappaAsse(int raw, int center) {
  int val;

  if (raw < center) {
    // Lato negativo: da 0 (min) a center (zero) → -255 a 0
    val = map(raw, 0, center, -255, 0);
  } else {
    // Lato positivo: da center (zero) a 4095 (max) → 0 a +255
    val = map(raw, center, 4095, 0, 255);
  }

  // Zona morta: valori vicini allo zero vengono azzerati
  if (val > -DEADZONE && val < DEADZONE) val = 0;

  return val;
}