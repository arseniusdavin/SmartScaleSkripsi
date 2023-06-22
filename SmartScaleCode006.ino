//auto connect di setup 2 kali yg pertama untuk lgsg connect ke 2 adalah AP mode selama 60 detik.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#include "HX711.h"
#include <WiFiManager.h>

#if defined(ESP32)
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif

//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#include "time.h"//library for get time

#define LOADCELL_DOUT_PIN  21 //esp32
#define LOADCELL_SCK_PIN  22 //esp32


//OLED parameter
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//MAX30100 parameter
#define REPORTING_PERIOD_MS 1000 //report every 1 second 

//MAX30100 variables
bool status;
PulseOximeter pox;
MAX30100 sensor;
float BPM, SpO2;
float BPM_before;
float BPM_after;
float BPM_result;
bool oximeterStart;
bool oximeterEnd;
bool oximeterStatus;

//HX711 variables
HX711 scale;
long zero_factor;
float calibration_factor = -25000; //works at -25 000 to -26 000
float weight_value;
float weight_before;
float weight_result;
float weight_after;

//wifi variables
WiFiManager wm;
#define ssid "cariparkir2"
#define Password "LockeyV1P"
bool wifiStatus;
bool APmode;
uint32_t portalMil;

//firebase variables
#define API_KEY "AIzaSyATVCbORXfiDjw-WViAJl_IqUg07nm_hD4" //punya nico
#define DATABASE_URL "https://viewpageuser-default-rtdb.asia-southeast1.firebasedatabase.app/"

//#define API_KEY "AIzaSyBRRbDrVdoEKQRtkVU04CLLliKPMMxxCe0" //davin
//#define DATABASE_URL "https://skripsi-smart-scale-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define USER_EMAIL "davin.arsenius123@binus.ac.id"
#define USER_PASSWORD "binus123"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool fireState;

//NTP server variables (time.h)
struct tm timeinfo;
int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year;
String DateNow;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 7;
const int   daylightOffset_sec = 0;

//millis variables
uint32_t wifiMil = 0;
uint32_t weightMil = 0;
uint32_t verifMil = 0;
uint32_t verifTimeOut = 0;
uint32_t weightTimeOut = 0;
uint32_t weightTime = 0;
uint32_t tareTime = 0;
uint32_t oxiTimeOut = 0;
uint32_t oxiTime = 0;
uint32_t oxiInterval = 0;
uint32_t oxiCheck = 0;
uint32_t stateMil = 0;

//other variables
int verification_code;
int verification;
int state;


void displayOpening() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Smart");
  display.setCursor(0, 32);
  display.println("Scale");
  display.display();
}

void displayConnecting() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Connecting");
  display.setCursor(0, 16);
  display.println("to");
  display.setTextSize(4);
  display.setCursor(0, 32);
  display.println("WiFi");
  display.display();
}

void displayConfigure() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Configure");
  display.setCursor(0, 16);
  display.println("the");
  display.setTextSize(4);
  display.setCursor(0, 32);
  display.println("WiFi");
  display.display();
}

void displayWifiConnected() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("WiFi");
  display.setTextSize(2);
  display.setCursor(0, 32);
  display.println("Connected");
  display.display();
}

void displayNoWifi() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("NO");
  display.setTextSize(4);
  display.setCursor(0, 32);
  display.println("WiFi");
  display.display();
}

void displayNoDB() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("NO");
  display.setTextSize(4);
  display.setCursor(0, 32);
  display.println("DB");
  display.display();
}

void displayStepToScale() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Step to");
  display.setTextSize(4);
  display.setCursor(0, 32);
  display.println("Scale");
  display.display();
}

void displayWeight() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Weight:");
  display.setTextSize(4);
  display.setCursor(0, 16);
  display.println(weight_value);
  display.setTextSize(2);
  display.setCursor(90, 48);
  display.println("kg");
  display.display();
}

void displayBPM() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("BPM:");
  display.setTextSize(4);
  display.setCursor(0, 16);
  display.println(BPM);
  display.setTextSize(2);
  display.setCursor(26, 48);
  display.println("beat/min");
  display.display();
}

void displaySpO2() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("SpO2:");
  display.setTextSize(4);
  display.setCursor(0, 16);
  display.println(SpO2);
  display.setTextSize(2);
  display.setCursor(106, 48);
  display.println("%");
  display.display();
}

void displayVerifCode() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15, 0);
  display.println("verification code:");
  display.setTextSize(7);
  display.setCursor(30, 10);
  display.println(verification_code);
  display.display();
}

void displayUseOxi() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Use the");
  display.setTextSize(3);
  display.setCursor(0, 32);
  display.print("Oximete");
  display.display();
}

void displayFailVerify() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Fail to");
  display.setTextSize(3);
  display.setCursor(10, 32);
  display.print("Verify");
  display.display();
}

void displayOxiError() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("OXIMETE");
  display.setTextSize(3);
  display.setCursor(27, 32);
  display.print("ERROR");
  display.display();
}

void displayDone() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(25, 16);
  display.print("DONE");
  display.display();
}

void displayWeightResult() {
  displayWeight();
  delay(400);
  display.clearDisplay();
  display.display();
  delay(300);
  displayWeight();
  delay(400);
  display.clearDisplay();
  display.display();
  delay(300);
  displayWeight();
  delay(400);
  display.clearDisplay();
  display.display();
  delay(300);
  displayWeight();
  delay(3000);
}

void printLocalTime() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  DateNow = (String)timeinfo.tm_mday + "-" + (String)timeinfo.tm_mon + "-" + (String)timeinfo.tm_year + ", " + (String)timeinfo.tm_hour + ":" + (String)timeinfo.tm_min + ":" + (String)timeinfo.tm_sec;
}

void onBeatDetected()
{
  Serial.println("Beat Detected!");
}


void weighing() {
  if (millis() - weightMil > 1000) {
    if (scale.is_ready()) {
      weight_result = scale.get_units();
      Serial.print("weight difference: ");
      Serial.println(weight_result);
      if (weight_result > 0.02 && weight_result < 2 ) {
        scale.tare();  //Reset the scale to 0
        zero_factor = scale.read_average(); //Get a baseline reading
        Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
        Serial.println(zero_factor);
        tareTime = millis();
      }
      else if (weight_result > -180 && weight_result < -0.02) {
        scale.tare();  //Reset the scale to 0
        zero_factor = scale.read_average(); //Get a baseline reading
        Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
        Serial.println(zero_factor);
        tareTime = millis();
      }
      tareTime = millis();
      while (weight_result > 2) {
        weight_before = scale.get_units(10);
        delay(300);
        weight_after = scale.get_units(10);
        weight_value = weight_after;
        displayWeight();
        if (weight_after - weight_before < 0.01 && millis() - tareTime > 5000) {
          weight_value = weight_after;
          Serial.print("HX711 reading: ");
          Serial.print(weight_value , 1);
          Serial.println(" kgs");
          displayWeightResult();
          break;
        }
      }
    }
    else {
      Serial.println("HX711 not found.");
    }
    weightMil = millis();
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(18, INPUT_PULLUP);
  Wire.begin(16, 17); //i2c pin for OLED and MAX30100
  //initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  }
  displayOpening();

  //initialize MAX30100
  Serial.print("Initializing Pulse Oximeter..");
  status = pox.begin();
  if (!status)
  {
    Serial.println("FAILED");
  }
  else
  {
    Serial.println("SUCCESS");
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_20_8MA);
  pox.shutdown();

  //initialize HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();  //Reset the scale to 0
  zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

  wm.setConfigPortalBlocking(true);
  wm.setConfigPortalTimeout(15);

  displayConnecting();
  bool res;
  res = wm.autoConnect("SmartScaleAP");
  if (!res) {
    Serial.println("Failed to connect");
    wifiStatus = 0;
    displayNoWifi();
  }
  else {
    Serial.println("WIFI CONNECTED");
    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    long rssi = WiFi.RSSI();
    Serial.print("RSSI:");
    Serial.println(rssi);
    wifiStatus = 1;
    displayWifiConnected();
  }

  APmode = digitalRead(18);
  Serial.print("APmode : ");
  Serial.println(APmode);
  wm.setConfigPortalTimeout(60);

  if (APmode == 0) {
    displayConfigure();
    wm.resetSettings();
    bool res;
    res = wm.autoConnect("SmartScaleAP");

    if (!res) {
      Serial.println("Failed to connect");
      wifiStatus = 0;
      displayNoWifi();
    }
    else {
      Serial.println("WIFI CONNECTED");
      Serial.println();
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      long rssi = WiFi.RSSI();
      Serial.print("RSSI:");
      Serial.println(rssi);
      wifiStatus = 1;
      displayWifiConnected();
    }
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);

  fireState = Firebase.ready();
  Serial.print("fireState: ");
  Serial.println(fireState);

}

void loop() {
  if ( wifiStatus == 0) {
    weighing();
    displayNoWifi();
    delay(1000);
  }
  else if (wifiStatus == 1 && fireState == 0) {
    weighing();
    displayNoDB();
    delay(1000);
  }
  else if (wifiStatus == 1 && fireState == 1 && millis() - stateMil > 1000) {
    Firebase.getInt(fbdo, "device1/state");
    state = fbdo.to<int>();
    Serial.print("state: ");
    Serial.println(state);
    stateMil = millis();
    if (state == 0) {
      weighing();
      displayWifiConnected();
      delay(1000);
    }
    else if (state == 1) {
      //APAKAH HARUS AMBIL USER DATA? / USER ID?
      verification_code = random(10, 99);
      Serial.printf("set verification code... %s\n", Firebase.setInt(fbdo, "device1/RNG", verification_code) ? "ok" : fbdo.errorReason().c_str());
      displayVerifCode();
      verifTimeOut = millis();
      while (verification == 0 ) { //verification perlu di 0 in dari aplikasi selesai penggunaan
        if ( millis() - verifMil > 5000) { //loop pengecekan variabel verification
          Firebase.getInt(fbdo, "device1/verification");
          verification = fbdo.to<int>();
          Serial.print("verification: ");
          Serial.println(verification);
          verifMil = millis();
        }
        else if (millis() - verifTimeOut > 61000) { //lama waktu tunggu device untuk user melakukan verification
          Serial.println("Fail to Verify"); //kalo gagal harus balik ke main menu (APAKAH PERLU KIRIM KONFIRMASI MENGGUNAKAN FAIL FLAG?)
          displayFailVerify();
          delay(5000); //waktu tunggu agar aplikasi membalikan state kembali ke 0
          Serial.printf("set state... %s\n", Firebase.setFloat(fbdo, "device1/state", 0) ? "ok" : fbdo.errorReason().c_str());
          break;
        }
      }
      while (verification == 1) {
        displayStepToScale();
        weightTimeOut = millis();
        if (millis() - weightMil > 1000 && millis() - weightTimeOut < 46000) {
          if (scale.is_ready()) {
            weight_result = scale.get_units();
            Serial.print("weight difference: ");
            Serial.println(weight_result);
            if (weight_result > 2) {
              weightTime = millis();
              while (millis() - weightTime < 11000) { //menunggu dapat nilai timbangan selama 11 detik
                weight_before = scale.get_units(10);
                delay(300);
                weight_after = scale.get_units(10);
                weight_value = weight_after;
                displayWeight();
                if (weight_after - weight_before < 0.01) {
                  weight_value = weight_after;
                  Serial.print("HX711 reading: ");
                  Serial.print(weight_value , 1);
                  Serial.println(" kgs");
                  displayWeightResult();
                  break;
                }
              }
              Serial.printf("set weight... %s\n", Firebase.setFloat(fbdo, "device1/weight", weight_value) ? "ok" : fbdo.errorReason().c_str());
              displayDone();
              pox.resume();
              oxiTimeOut = millis();
              while (millis() - oxiTimeOut < 46000) {
                pox.update();
                displayUseOxi();
                BPM_result = pox.getHeartRate();
                Serial.print("BPM difference: ");
                Serial.println(BPM_result);
                if (BPM_result > 20) {
                  oxiTime = millis();
                  while (millis() - oxiTime < 35000) {
                    pox.update();
                    BPM_before = pox.getHeartRate();
                    SpO2 = pox.getSpO2();
                    if ( millis() - oxiInterval > 700) {
                      BPM_after = pox.getHeartRate();
                      Serial.print("Heart rate after:");
                      Serial.print(BPM_after);
                      Serial.print(" bpm / SpO2:");
                      Serial.print(SpO2);
                      Serial.println(" %");
                      oxiInterval = millis();
                    }
                    else if (BPM_after - BPM_before < 0.1 && SpO2 != 0 && millis() - oxiTime > 15000 ) {
                      BPM = BPM_after;
                      pox.shutdown();
                      break;
                    }
                  }
                  if (millis() - oxiTime > 34999) {
                    displayOxiError();
                    delay(3000);
                  }
                  pox.shutdown();
                  displayBPM();
                  delay(4000);
                  displaySpO2();
                  delay(4000);
                  Serial.printf("set BPM... %s\n", Firebase.setFloat(fbdo, "device1/BPM", BPM) ? "ok" : fbdo.errorReason().c_str());
                  Serial.printf("set SpO2... %s\n", Firebase.setFloat(fbdo, "device1/SpO2", SpO2) ? "ok" : fbdo.errorReason().c_str());
                  displayDone();
                  Serial.printf("set state... %s\n", Firebase.setFloat(fbdo, "device1/state", 0) ? "ok" : fbdo.errorReason().c_str());
                  Serial.printf("set verification... %s\n", Firebase.setFloat(fbdo, "device1/verification", 0) ? "ok" : fbdo.errorReason().c_str());
                  verification = 0;
                  break;
                }
              }
              pox.shutdown();
              weight_result = 0;
              weight_after = 0;
              Serial.printf("set BPM = 0  ... %s\n", Firebase.setFloat(fbdo, "device1/BPM", BPM) ? "ok" : fbdo.errorReason().c_str());
              Serial.printf("set SpO2 = 0  ... %s\n", Firebase.setFloat(fbdo, "device1/SpO2", SpO2) ? "ok" : fbdo.errorReason().c_str());
              displayDone();
              Serial.printf("set state... %s\n", Firebase.setFloat(fbdo, "device1/state", 0) ? "ok" : fbdo.errorReason().c_str());
              Serial.printf("set verification... %s\n", Firebase.setFloat(fbdo, "device1/verification", 0) ? "ok" : fbdo.errorReason().c_str());
            }
            //            else if (weight_result < 0.3) {
            //              weight_value = 0;
            //              displayWeight();
            //            }
          }
          else {
            Serial.println("HX711 not found.");
          }
          weightMil = millis();
        }
      }
    }
  }
}
