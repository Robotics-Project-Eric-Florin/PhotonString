#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>
#include <esp_now.h>
#include <WiFi.h>

Adafruit_MPU6050 mpu;

uint8_t broadcastAddress[] = {0x88, 0x13, 0xBF, 0x0D, 0xB7, 0x54}; 

const int BUZZER_PIN = 18;
const int BUTTON_PIN = 13; 

const int NUMAR_NOTE = 5;

int noteMinor[] = {262, 311, 349, 392, 466}; 
int noteMajor[] = {262, 294, 330, 392, 440};  

int* noteCurente = noteMinor;
bool esteMajor = false; 

int lastButtonState = HIGH; 
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

float pitchSmooth = 0;
float rollSmooth = 0;
const float ALPHA = 0.15; 
int ultimaFrecventa = 0; 
bool esteLiniste = false;

typedef struct struct_message {
  int frecventa; 
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Eroare ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Eroare adaugare peer");
    return;
  }

  if (!mpu.begin()) {
    Serial.println("Senzor MPU6050 lipsa!");
    while (1) { 
      tone(BUZZER_PIN, 1000, 100); delay(200);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setFilterBandwidth(MPU6050_BAND_10_HZ);
  
  Serial.println("=== THEREMIN DUAL-MODE ===");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
       schimbaGama();
       delay(300); 
    }
  }
  lastButtonState = reading;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z)) * 180.0 / PI;
  float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;

  pitchSmooth = ALPHA * pitch + (1.0 - ALPHA) * pitchSmooth;
  rollSmooth = ALPHA * roll + (1.0 - ALPHA) * rollSmooth;

  if (abs(pitchSmooth) < 4.0 && abs(rollSmooth) < 8.0) {
    if (!esteLiniste) { 
      noTone(BUZZER_PIN);
      esteLiniste = true;
      ultimaFrecventa = 0;
      
      myData.frecventa = 0;
      esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    }
    return; 
  }
  esteLiniste = false;

  float multiplicator = 1.0;
  if (rollSmooth < -25) multiplicator = 0.5; 
  else if (rollSmooth > 25) multiplicator = 2.0; 

  int unghiLimitat = constrain((int)pitchSmooth, -45, 45);
  int indexNota = map(unghiLimitat, -45, 45, 0, NUMAR_NOTE - 1);
  indexNota = constrain(indexNota, 0, NUMAR_NOTE - 1);

  int frecventaNoua = noteCurente[indexNota] * multiplicator;

  if (frecventaNoua != ultimaFrecventa) {
    tone(BUZZER_PIN, frecventaNoua);
    ultimaFrecventa = frecventaNoua;
    
    myData.frecventa = frecventaNoua;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    Serial.print("Gama: "); Serial.print(esteMajor ? "MAJOR" : "MINOR");
    Serial.print(" | Freq: "); Serial.println(frecventaNoua);
  }
  
  delay(15); 
}

void schimbaGama() {
  esteMajor = !esteMajor; 

  if (esteMajor) {
    noteCurente = noteMajor;
    Serial.println(">>> SCHIMBAT PE GAMA MAJORA (Vesel)");
    tone(BUZZER_PIN, 1000, 100); delay(150);
    tone(BUZZER_PIN, 1500, 100); delay(150);
  } else {
    noteCurente = noteMinor;
    Serial.println(">>> SCHIMBAT PE GAMA MINORA (Misterios)");
    tone(BUZZER_PIN, 500, 100); delay(150);
    tone(BUZZER_PIN, 300, 100); delay(150);
  }
  ultimaFrecventa = 0; 
}