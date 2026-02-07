#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

#define PIN_SLOW 25 
#define PIN_FAST 26 

float visualFreq = 20.5; 
float freqSlow = 40.0;   

double phaseShift = 0.0;

bool laserActiv = true;

typedef struct struct_message {
  int frecventa;
} struct_message;

struct_message myData;

void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  
  int freqAudio = myData.frecventa;

  if (freqAudio == 0) {
    laserActiv = false;
  } else {
    laserActiv = true;
    float mappedVal = map(freqAudio, 130, 1000, 10, 90);
    visualFreq = mappedVal + 0.33; 
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Eroare ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Laser Receiver cu ROTATIE Activat!");
}

void loop() {
  double t = micros() / 1000000.0;

  int valX = 128; 
  int valY = 128;

  if (laserActiv) {
    phaseShift += 0.03; 
    if (phaseShift > 2 * PI) phaseShift = 0; 

    valX = 128 + 80 * sin(2 * PI * freqSlow * t);
    valY = 128 + 80 * sin(2 * PI * visualFreq * t + phaseShift); 
  } else {
    valX = 128 + 60 * sin(2 * PI * 2 * t);
    valY = 128 + 60 * cos(2 * PI * 2 * t);
  }

  dacWrite(PIN_SLOW, valX);
  dacWrite(PIN_FAST, valY);
}