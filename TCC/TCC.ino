#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <HTTPClient.h>
#include <Wifi.h>
#include <Temperature_LM75_Derived.h>
#include "I2Cdev.h"
#include "MPU6050.h"

#define REPORTING_PERIOD_MS     10000
#define LED_PIN 2
#define PULSEIRA_ID 2

//const char* ssid = "KJAA";
//const char* password =  "kjaa2017";
const char* ssid = "renato";
const char* password =  "lablab01";
const char* serverName = "http://45.93.100.30:8080/data";


// variaveis
int lastValidOxi, lastValidFreq, oxi, freq, lastValidTemp, temp, axi, ayi, azi, gxi, gyi, gzi;
int16_t ax, ay, az;
int16_t gx, gy, gz;

PulseOximeter pox; // objeto que obtem pulsação e oxigenaçao
Generic_LM75 temperature; // objeto que obtem temperatura
MPU6050 accelgyro; //objeto que obtem aceleração e giroscopio

uint32_t tsLastReport = 0;


// Função callback que roda sempre que um batimento é detectado
void onBeatDetected()
{
  Serial.println("Beat");
  digitalWrite(LED_PIN, HIGH); // acende o LED
  delay(100); // espera 500ms
  digitalWrite(LED_PIN, LOW); // apaga o LED
  
}

void setup()
{
  Serial.begin(115200);
  
  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin()) {
    Serial.println("FAILED");
    for(;;);
  } else {
    Serial.println("SUCCESS");
  }

  pinMode(LED_PIN, OUTPUT);

  pox.setOnBeatDetectedCallback(onBeatDetected);
  pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);

  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
  #endif

  accelgyro.initialize();

  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

}

void loop()
{
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

    freq = pox.getHeartRate(); // obtendo pulsação
    Serial.print("Heart rate:");
    Serial.print(freq);
    lastValidFreq = freq > 0 ? freq : lastValidFreq; // verificando se é uma pulsação valida senão mantem a ultima
    
    oxi = pox.getSpO2(); // obtendo oxigenaçao
    Serial.print("bpm / SpO2:");
    Serial.print(oxi);
    lastValidOxi = oxi > 0 ? oxi : lastValidOxi; // verificando se é uma oxigenaçao valida senão mantem a ultima
    Serial.println("%");
    
    temp = temperature.readTemperatureC(); // obtendo temperatura
    lastValidTemp = temp ? temp : lastValidTemp; // verificando se é uma temperatura valida senão mantem a ultima
    Serial.print("Temperature = ");
    Serial.print(lastValidTemp);
    Serial.println(" C");

    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // obtendo aceleração e giroscopio
    axi = ax;
    ayi = ay;
    azi = az;
    gxi = gx;
    gyi = gy;
    gzi = gz;

    Serial.print  ("Ax: " + String(ax)  + " ");
    Serial.print  ("Ay: " + String(ay)  + " ");
    Serial.println("Az: " + String(az)  + " ");
    Serial.print  ("Gx: " + String(gx)  + " ");
    Serial.print  ("Gy: " + String(gy)  + " ");
    Serial.println("Gz: " + String(gz)  + " ");

    WiFi.begin(ssid, password);  // conectando ao wifi somente agora pois não sei o porque mas conectado ao wifi o medidos de oxigenação e pulsação não funciona.
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected");
    
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // formando a string no formato json para envio do POST
    String send = String() + 
    "{"
      "\"ax\": " + String(axi) + ", "
      "\"ay\": " + String(ayi) + ", "
      "\"az\": " + String(azi) + ", "
      "\"gx\": " + String(gxi) + ", "
      "\"gy\": " + String(gyi) + ", "
      "\"gz\": " + String(gzi) + ", "
      "\"freq\": " + String(lastValidFreq) + ", " 
      "\"oxi\": "  + String(lastValidOxi)  + ", " 
      "\"temp\" :" + String(lastValidTemp) + ", "
      "\"pulseiraId\":" + String(PULSEIRA_ID) +
    "}";

    Serial.println(send);

    int httpResponseCode = http.POST(send); // enviando post e pegando o codigo da reposta

    if (httpResponseCode>0) {
      String response = http.getString(); // pegando resposta
      Serial.println(httpResponseCode); //Print return code
      Serial.println(response); //Print request answer
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end(); //Free resources
    
    tsLastReport = millis();
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF); // deconectar e desligar o wifi pois não funciona wifi r sensor de oxigenação e pulsação ao mesmo tempo
    Serial.println("Wifi Disconnected");

    if (!pox.begin()) {    // reiniciar o sensor de pulsação e oxigenação por causa do problema com o wifi
      Serial.println("FAILED");
      for(;;);
    } else {
      Serial.println("SUCCESS");
    }
  }
}
