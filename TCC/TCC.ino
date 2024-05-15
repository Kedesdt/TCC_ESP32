#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <HTTPClient.h>
#include <Wifi.h>
#include <Temperature_LM75_Derived.h>
#include "I2Cdev.h"
#include "MPU6050.h"

#define REPORTING_PERIOD_MS     10000
#define LED_PIN 2

//const char* ssid = "KJAA";
//const char* password =  "kjaa2017";
const char* ssid = "renato";
const char* password =  "lablab01";
const char* serverName = "http://45.93.100.30:8080/data";

int lastValidOxi, lastValidFreq, oxi, freq, lastValidTemp, temp, axi, ayi, azi, gxi, gyi, gzi;
int16_t ax, ay, az;
int16_t gx, gy, gz;

PulseOximeter pox;
Generic_LM75 temperature;
MPU6050 accelgyro;

uint32_t tsLastReport = 0;

// Callback (registered below) fired when a pulse is detected
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

    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
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
  //if ((WiFi.status() == WL_CONNECTED)){
      // Make sure to call update as fast as possible
    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

      freq = pox.getHeartRate();
      Serial.print("Heart rate:");
      Serial.print(freq);
      lastValidFreq = freq > 0 ? freq : lastValidFreq;
      oxi = pox.getSpO2();
      Serial.print("bpm / SpO2:");
      Serial.print(oxi);
      lastValidOxi = oxi > 0 ? oxi : lastValidOxi;
      Serial.println("%");
      temp = temperature.readTemperatureC();
      lastValidTemp = temp ? temp : lastValidTemp;
      Serial.print("Temperature = ");
      Serial.print(lastValidTemp);
      Serial.println(" C");

      accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      axi = ax;
      ayi = ay;
      azi = az;
      gxi = gx;
      gyi = gy;
      gzi = gz;

      Serial.print("Ax: " + String(ax)  + " ");
      Serial.print("Ay: " + String(ay)  + " ");
      Serial.println("Az: " + String(az)  + " ");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
      }
      Serial.println("Connected");
      
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");

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
        "\"pulseiraId\": 2"
      "}";

      Serial.println(send);

      int httpResponseCode = http.POST(send);

      if (httpResponseCode>0) {
        String response = http.getString(); //Get the response to the request
        Serial.println(httpResponseCode); //Print return code
        Serial.println(response); //Print request answer
      } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }

      http.end(); //Free resources
      
      tsLastReport = millis();
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      Serial.println("Wifi Disconnected");
      //delay(1000);
      if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
      } else {
        Serial.println("SUCCESS");
      }
    }
  //}
}
