#include <ESP32_NOW.h>
#include <WiFi.h>

#define PIN1_M1 23
#define PIN2_M1 27

#define PIN1_M2 19
#define PIN2_M2 18

#define PIN1_M3 32
#define PIN2_M3 33

#define PIN1_M4 25
#define PIN2_M4 26


unsigned long ultimoPacchetto = 0;
const unsigned long TIMEOUT = 500; // ms

typedef struct {
  int16_t M1;
  int16_t M2;
  int16_t M3;
  int16_t M4;
} Payload;

Payload datiRicevuti;  // qui vengono salvati i dati in arrivo

void onDataReceived(const esp_now_recv_info_t *info,
                    const uint8_t *dati, int len) {
  memcpy(&datiRicevuti, dati, sizeof(datiRicevuti));
  // qui usi datiRicevuti.M1, .M2, .M3, .M4
  ultimoPacchetto = millis(); // resetta il timer ad ogni pacchetto ricevuto
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // ← questa riga è la chiave
  
  delay(500);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  esp_now_init();
  esp_now_register_recv_cb(onDataReceived);
}

void scriviMotori(uint8_t pinA, uint8_t pinB, uint8_t val_pinA, uint8_t val_pinB){
  analogWrite(pinA, val_pinA);
  analogWrite(pinB, val_pinB);
}

void loop() {
  if (millis() - ultimoPacchetto > TIMEOUT) {
    // nessun pacchetto da 500ms → ferma i motori
    datiRicevuti.M1 = 0;
    datiRicevuti.M2 = 0;
    datiRicevuti.M3 = 0;
    datiRicevuti.M4 = 0;
    // chiama qui la tua funzione che scrive 0 ai driver motori
  }

  if(datiRicevuti.M1>=0){
    scriviMotori(PIN1_M1, PIN2_M1, abs(datiRicevuti.M1), 0);
  }else{
    scriviMotori(PIN1_M1, PIN2_M1, 0, abs(datiRicevuti.M1));
  }
  if(datiRicevuti.M2>=0){
    scriviMotori(PIN1_M2, PIN2_M2, abs(datiRicevuti.M2), 0);
  }else{
    scriviMotori(PIN1_M2, PIN2_M2, 0, abs(datiRicevuti.M2));
  }
  if(datiRicevuti.M3>=0){
    scriviMotori(PIN1_M3, PIN2_M3, abs(datiRicevuti.M3), 0);
  }else{
    scriviMotori(PIN1_M3, PIN2_M3, 0, abs(datiRicevuti.M3));
  }
  if(datiRicevuti.M4>=0){
    scriviMotori(PIN1_M4, PIN2_M4, abs(datiRicevuti.M4), 0);
  }else{
    scriviMotori(PIN1_M4, PIN2_M4, 0, abs(datiRicevuti.M4));
  }
  Serial.print(datiRicevuti.M1);Serial.print(" ");Serial.print(datiRicevuti.M2);Serial.print(" ");Serial.print(datiRicevuti.M3);Serial.print(" ");Serial.println(datiRicevuti.M4);
delay(50);
}