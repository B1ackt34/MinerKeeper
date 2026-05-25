// =================================================================================================
//              -----------------------------------------------------------------
//                Copyright 2025 Blacktea @ minerkeeper.org under BSD-3-Clause
//              -----------------------------------------------------------------
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions
// and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
// conditions and the following disclaimer in the documentation and/or other materials provided with
// the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to
// endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// =================================================================================================
//                        ----------------------
//                             Miner Keeper
//                        ----------------------
//
// Github : 
// https://github.com/B1ackt34/MinerKeeper
//
// Description:
// Miner Keeper is an hardware watchdog designed to protect your miner from malfunctions or failures
// This project is developed by Blacktea from BitsFromItaly.it
// 
// Target Board: ESP32 Dev Module
// Compilation: Tested with ESP32 on Arduino IDE version 2.3.5//
//
// You can contribute to the development of the project by donating coins to the following addresses
//
// NEXA - nexa:nqtsq5g5k93k6xpljm03kwrzvqznpdltr506edyv27ylg7fj
// BTC - 16uwxhsrjpkL2herNE1UBY9wbnKdNqtAhd
// KAS - kaspa:qq8va2lly9cxu2ydcjpsjd05hpdaplyytry79yfn8kesqu4x93axzzdd2c3gq
// DOGE - DKsnrSNLPcMwrufxEVKieTKbF7LLA7j8jD 
// =================================================================================================

// main libraries

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ThingSpeak.h"                     // Thingspeak connection management
#include <HTTPClient.h>                     // JSON pool management
#include "alarms_text.h"                    // texts to be used by alarms
#include <UrlEncode.h>

// sensors libraries

#include <OneWire.h>                        // One Wire connection management
#include <DallasTemperature.h>              // Temperature probe management
#include <PZEM004Tv30.h>                    // Voltmeter and ammeter management
#include <ESP32Servo.h>
#include <NTPClient.h>                      // Time management
#include <ESP_Mail_Client.h>                // email management
#include "HeapStat.h"
#include "driver/pcnt.h"                    // PCNT fans tach

// --------------------------------------------- fallback JSON ---------------------------------

template <typename T>
T getJsonValue(JsonVariant v, T fallback) {
    if (v.isNull()) return fallback;

    if constexpr (std::is_same<T, String>::value) {
        String s = v.as<String>();
        return s.length() ? s : fallback;
    }

    return v.as<T>();
}

// ---------------------------------------------------- Miner Keeper version

const char * versione = "1.1.14";              // software version

// ---------------------------------------------------- App connection variables

Preferences prefs;
AsyncWebServer server(80);

struct MinerConfig {
  String ipminer;
  String portminer;
  String wifiname;
  String wifipass;
  String nomeminer;
  String ambienttemp;
  String minertemp;
  String airvalve;
  String airvalveauto;
  String fanAuto;         // fan settings
  String fanSpeedSet; // rpm i want the fans run at
  String fanSpeedRead1;
  String fan2on;
  String fanSpeedRead2;
  String fan3on;
  String fanSpeedRead3;
  String valveAirPos;
  String thingspeak;      // thingspeak settings
  String idchannel;
  String apiwrite;
  String apiread;
  String notifs;
  String email_notifs;     // email settings
  String senderemail;
  String passwordemail;
  String recipientemail;
  String serveremail;
  String portemail;
  String whatsapp_notifs;  // whatsapp settings
  String WS_phonenumber;
  String WS_callmebot;
  String telegram_notifs;  // telegram settings
  String TG_username;
};

// ----------------------------------------------------- variables that enable/disable features

//bool BoolpoolsUse = false ;
bool Boolthingspeak = false ;
bool Boolairvalve = false ;
bool Boolairvalveauto = true ;
bool BoolfanAuto = true ;
bool Boolfan2on = true ;
bool Boolfan3on = true ;
bool Boolnotifs = false ;
bool Boolemail = false ;
bool Boolwhatsapp = false ;
bool Booltelegram = false ;

MinerConfig Mconfig;

// ----------------------------------------------------- wifi

unsigned long previousMillis = 0;
const long interval = 10000;              // interval to wait for Wi-Fi connection (milliseconds)

WiFiClient  client;

const char* ssid = "Your_WIFI_SSID";
const char* password = "YOUR_WIFI_password";

const char* ssidAP     = "Minerkeeper";
const char* passwordAP = "123456789";

// ----------------------------------------------------- servo motor

Servo servoAir;
int servoPin = 19;
int posizione = 0;  // variable to know where the valve is positioned - start with air pushed out
int maxAperturaServo = 0;
int finecorsaPin = 25;  // Shutter closed button
bool finecorsa = false;
ESP32PWM pwm;

// ----------------------------------------------------- ThingSpeak - Thingspeak settings

String voltsTS = "" ;
String airTS = "" ;
String rpmTS = "" ;
String alarmTS = "" ;
String sharesTS = "" ;
String energyTS = "" ;

// ----------------------------------------------------- voltmeter ammeter + power supply measurements
  
PZEM004Tv30 pzem(Serial2, 16, 17);
float voltage ;
float current ;
float power ;
float energy ;

const int pin12volt = 35;
const int pin5volt = 34;
const int pin33volt = 39;
float v_in12 = 0;
float v_in5 = 0;
float v_in33 = 0;
const float true12v = 12.00;
const float true5v = 5.00;
const float true33v = 3.3;
float cali12v = 1.0;
float cali5v = 1.0;
float cali33v = 1.0;

// ----------------------------------------------------- OneWire temperature probes
  
#define ONE_WIRE_BUS 18
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress airIncoming, airMiner, airOutgoing;

int ariaIN = 0;
int ariaGPU = 0;
int ariaOUT = 0;
unsigned long ariaTimesent = 0;
unsigned long ariaInterval = 20000;

// ---------------------------------------------------- ambient temperature sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 27     // Digital pin connected to the DHT sensor
// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22            // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
int temperatura ;              // temperature measured by DHT22
int umidita ;                  // humidity measured by DHT22
int temperaturaAmbiente ;      // temperature I want at home

// --------------------------------------------------- flame sensor
  
#define sensoreFIAMMA 33
int valoreFIAMMA = HIGH ;


// --------------------------------------------------- fans

const int pinFans = 26;
int freqFan = 768;
int pinChannel = 0;
const int resolution = 10;

#define FAN1_TACH_PIN 36             
#define FAN2_TACH_PIN 13
#define FAN3_TACH_PIN 4
#define FAN1_UNIT PCNT_UNIT_0
#define FAN2_UNIT PCNT_UNIT_1
#define FAN3_UNIT PCNT_UNIT_2
//const int pinTacho = 36;
int16_t count1 = 0 , count2 = 0, count3 = 0;
int rpm1 = 0;
int rpm2 = 0;
int rpm3 = 0;

void setupPCNTUnit(int gpio, pcnt_unit_t unit) { 
  pcnt_config_t pcnt_config = { 
    .pulse_gpio_num = gpio, 
    .ctrl_gpio_num = PCNT_PIN_NOT_USED, 
    .lctrl_mode = PCNT_MODE_KEEP, 
    .hctrl_mode = PCNT_MODE_KEEP, 
    .pos_mode = PCNT_COUNT_INC, 
    .neg_mode = PCNT_COUNT_DIS, 
    .counter_h_lim = 20000, 
    .counter_l_lim = 0, 
    .unit = unit, 
    .channel = PCNT_CHANNEL_0 
  }; 
  pcnt_unit_config(&pcnt_config); 
  pcnt_set_filter_value(unit, 1500); // filtro anti-rumore 1.5 µs 
  pcnt_filter_enable(unit); 
  pcnt_counter_pause(unit); 
  pcnt_counter_clear(unit); 
  pcnt_counter_resume(unit); 
}
// --------------------------------------------------- HTTP and JSON handling

String minerAddressAPI ;
String dataRequestJSON = "" ;
String IDminingSoftware ;
String software;
String software1;
String software2;
String uptime ;
String lastUpdate;
String algorithm ;
String walletAddress ;
String poolAddressLink ;
String poolAddressPort ;
String poolAddress ;
String accepted ;
String invalid ;
String rejected ;
String gpuPower ;
float hashrate ;
String gpuName;
String gpuTotalMem;
String osName;
String cudaDriver;
String gpuCoreTemp ;
String gpuMemTemp ;
String gpuFan ;
String performanceUnit ;

bool jsonContains(JsonVariant element, const char* text);

float balance ;
float paidtotal ;
const char* currency ;
long hashrateTS = 0 ;

unsigned long hashrateTimesent = 0;
unsigned long hashrateInterval = 10000;             // time interval between hashrate requests to the pool

// ---------------------------------------------------- SMTP service and alarms

int ack = 0;
SMTPSession smtp;
Session_Config config;
void smtpCallback(SMTP_Status status);
HeapStat heapInfo;

// ---------------------------------------------------- NTP client

WiFiUDP Udp;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600); // 3600 is the number of seconds for GMT+1
unsigned long now;
unsigned long diffNow;
unsigned long lastUpdateUL;
String lastUpdateFormatted;

String formatMMSS(unsigned long sec) {
    unsigned long m = sec / 60;
    unsigned long s = sec % 60;

    char buffer[6]; // "MM:SS" + terminatore
    sprintf(buffer, "%02lu:%02lu", m, s);
    return String(buffer);
}

// ---------------------------------------------------- Alarms

int alarmLedPin = 23;      // alarm led
int alarmRele = 21;        // output relay in case of alarm
int alarmButtonPin = 22;
bool alarmButtonState = false;

boolean alarmTemp = false;
boolean alarmVolt = false;
boolean alarmFans = false;
boolean alarmAirvalve = false;
boolean alarmFire = false;

// *****************************************************************************************************************************************************************************************
// SETUP
// *****************************************************************************************************************************************************************************************

void setup() {
  Serial.begin(115200);

  pinMode(alarmLedPin, OUTPUT);
  pinMode(alarmRele, OUTPUT);
  digitalWrite(alarmRele, LOW);
  pinMode(alarmButtonPin, INPUT_PULLUP);    // alarm reset button

  // avvio ventole
  setupPCNTUnit(FAN1_TACH_PIN, FAN1_UNIT);
  setupPCNTUnit(FAN2_TACH_PIN, FAN2_UNIT);
  setupPCNTUnit(FAN3_TACH_PIN, FAN3_UNIT);

  // I recover the saved configuration
  loadConfig();

  // I publish the configuration on the Serial monitor for debug
  viewConfig();

  // wifi starts
  WiFi.begin(Mconfig.wifiname.c_str(), Mconfig.wifipass.c_str());
  unsigned long beginTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - beginTime < 6000) {
    Serial.println("Connecting to WiFi...");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect. Starting AP mode...");
    WiFi.softAP(ssidAP, passwordAP);
    Serial.println(WiFi.softAPIP());
  }

  // I prepare the server for requests

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

        request->send(200, "text/plain", "I'm running.");
  }  );

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {

        request->send(200, "text/plain", "Rebooting");
        delay(100);
        ESP.restart();
  }  );

  server.on("/request_data", HTTP_GET, [](AsyncWebServerRequest *request) {

    requestData();
    Serial.print("I retrieve data from Miner Keeper and prepare it in JSON format");
    Serial.println(dataRequestJSON);

    request->send(200, "text/plain", dataRequestJSON);
  }  );

  server.on("/request_configuration", HTTP_GET, [](AsyncWebServerRequest *request) {

    printConfig();
    Serial.print("I retrieve configuration from Miner Keeper and prepare it in JSON format");
    Serial.println(dataRequestJSON);

    request->send(200, "text/plain", dataRequestJSON);
  }  );

  server.on("/reset_energy", HTTP_GET, [](AsyncWebServerRequest *request) {

    ResetEnergiaVoid();
    Serial.print("Reset of energy measure");
    Serial.println(dataRequestJSON);

    request->send(200, "text/plain", dataRequestJSON);
  }  );

  server.on("/detect_software", HTTP_GET, [](AsyncWebServerRequest *request) {

    detectSoftware();

    request->send(200, "text/plain", dataRequestJSON);
  }  );
  

  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();

      DynamicJsonDocument doc(1200);

      DeserializationError error = deserializeJson(doc, value);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      Serial.print("value received : ");
      Serial.println(value);

      prefs.begin("miner", false);
      //prefs.clear();              // for debug - to delete all saved variables
      prefs.putString("ipminer", doc["ipminer"].as<String>());
      prefs.putString("portminer", doc["portminer"].as<String>());
      prefs.putString("wifiname", doc["wifiname"].as<String>());
      prefs.putString("wifipass", doc["wifipass"].as<String>());
      prefs.putString("nomeminer", doc["nomeminer"].as<String>());
      prefs.putString("ambienttemp", doc["ambienttemp"].as<String>());
      prefs.putString("airvalve", doc["airvalve"].as<String>());          // air valve present or not
      prefs.putString("airvalveauto", doc["airvalveauto"].as<String>());  // air valve in automatic operation or not
      prefs.putString("minertemp", doc["minertemp"].as<String>());
      prefs.putString("fanAuto", doc["fanAuto"].as<String>());
      prefs.putString("fanSpeedSet", doc["fanSpeedSet"].as<String>());
      prefs.putString("fan2on", doc["fan2on"].as<String>());
      prefs.putString("fan3on", doc["fan3on"].as<String>());
      prefs.putString("valveAirPos", doc["valveAirPos"].as<String>());
      prefs.putString("thingspeak", doc["thingspeak"].as<String>());
      prefs.putString("idchannel", doc["idchannel"].as<String>());
      prefs.putString("apiwrite", doc["apiwrite"].as<String>());
      prefs.putString("apiread", doc["apiread"].as<String>());
      prefs.putString("notifs", doc["notifs"].as<String>());
      prefs.putString("email_notifs", doc["email_notifs"].as<String>());        // email
      prefs.putString("senderemail", doc["senderemail"].as<String>());
      prefs.putString("passwordemail", doc["passwordemail"].as<String>());
      prefs.putString("recipientemail", doc["recipientemail"].as<String>());
      prefs.putString("serveremail", doc["serveremail"].as<String>());
      prefs.putString("portemail", doc["portemail"].as<String>());
      prefs.putString("whatsapp_notifs", doc["whatsapp_notifs"].as<String>());  // whatsapp
      prefs.putString("WS_phonenumber", doc["WS_phonenumber"].as<String>());
      prefs.putString("WS_callmebot", doc["WS_callmebot"].as<String>());
      prefs.putString("telegram_notifs", doc["telegram_notifs"].as<String>());  // telegram
      prefs.putString("TG_username", doc["TG_username"].as<String>());
      prefs.end();

      request->send(200, "text/plain", "Data saved. Reboot to apply.");
      
    } else {
      request->send(400, "text/plain", "Missing string");
    }
  });

  // I start the flame detector
  pinMode(sensoreFIAMMA, INPUT);

  // I start the servomotor
  pinMode(finecorsaPin, INPUT);
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  servoAir.setPeriodHertz(50);
  servoAir.attach(servoPin, 500, 2500);
  Serial.println("I start adjusting the air valve");
  for (int posServo = 0; posServo < 90; posServo ++) {
    servoAir.write(posServo);
    delay(50);
    if (digitalRead(finecorsaPin) == HIGH) {
      finecorsa = true;
      maxAperturaServo = posServo;                                  // once the maximum opening is reached I assign the reached value to maxAperturaServo
      break ;
    }
  }
  Serial.println("Finished adjusting the air valve");

  // DHT22 and Onewire temperature probe configuration
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delay(500);
  sensors.begin();
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(airIncoming, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(airMiner, 1)) Serial.println("Unable to find address for Device 1");
  if (!sensors.getAddress(airOutgoing, 2)) Serial.println("Unable to find address for Device 2");
  // first reading of temperature probes
  sensors.requestTemperatures();
  ariaIN = sensors.getTempC(airIncoming);
  ariaGPU = sensors.getTempC(airMiner);
  ariaOUT = sensors.getTempC(airOutgoing);

  // I start Thingspeak
  
  ThingSpeak.begin(client);

  // Autotune voltages
  analogReadResolution(12);          // 0–4095
  analogSetAttenuation(ADC_11db);    // fino a ~3.3V reali
  pinMode(pin12volt, INPUT);
  pinMode(pin5volt, INPUT);
  pinMode(pin33volt, INPUT);

  // ---------------------- Calibration 12V ---
  float media12 = 0.0;
  delay(1000);
  Serial.println("Running voltages calibrations - 12 volts");
  for(int i12 = 0; i12 < 100; i12++) {
      media12 += analogRead(pin12volt);
      delay(1);
  }
  media12 /= 100.0;
  float v_adc_12 = (media12 / 4095.0) * 3.3;
  cali12v = true12v / (v_adc_12 * 3.7);
  Serial.print("Calibration factor 12V: ");
  Serial.println(cali12v);

  // ---------------------- Calibration 5V ---
  float media5 = 0.00;
  delay(1000);
  Serial.println("Running voltages calibrations - 5 volts");
    for(int i5 = 0; i5 < 100; i5++) {
      media5 += analogRead(pin5volt);
      delay(1);
  }
  media5 /= 100.0;
  float v_adc_5 = (media5 / 4095.0) * 3.3;
  cali5v = true5v / (v_adc_5 * 1.985);
  Serial.print("Calibration factor 5V: ");
  Serial.println(cali5v);

  // ---------------------- Calibration 3.3V ---

  float media33 = 0.00;
  for(int i33 = 0; i33 < 100; i33++) {
      media33 += analogRead(pin33volt);
      delay(1);
  }
  media33 /= 100.0;
  float v_adc_33 = (media33 / 4095.0) * 3.3;
  cali33v = true33v / v_adc_33;
  Serial.print("Calibration factor 3.3V: ");
  Serial.println(cali33v);
  
  // I start email service and NTP client

  timeClient.begin();
  timeClient.update();  // I update the time
  Serial.println(timeClient.getFormattedTime());

  MailClient.networkReconnect(true);
  MailClient.clearAP();
  MailClient.addAP(ssid, password);
  smtp.debug(1);
  smtp.callback(smtpCallback);
  config.server.host_name = Mconfig.serveremail;
  config.server.port = (F("esp_mail_smtp_port_"), Mconfig.portemail.toInt()); // String(Mconfig.wifiname)
  config.login.email = String(Mconfig.senderemail);
  config.login.password = String(Mconfig.passwordemail);
  config.login.user_domain = WiFi.localIP();
  // I configure the NTP server
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  // I enable the functions required by the app

  settings_on_off();

  // I start the fans

  if (BoolfanAuto) {
    Serial.println("fans set to automatic function");
    ledcAttach(pinFans, freqFan, resolution);
    ledcWrite(pinFans, freqFan);                                       // fan speed
    //pinMode(pinTacho, INPUT_PULLUP); 
  } else {
    Serial.println("fans set to manual function");
    ledcAttach(pinFans, freqFan, resolution);
    freqFan = map(Mconfig.fanSpeedSet.toInt(), 10, 100, 100, 1023); // ESP32 has 12-bit ADC resolution
    ledcWrite(pinFans, freqFan); 
  }

  server.begin();
  Serial.println("web server started");

}
// *****************************************************************************************************************************************************************************************
// LOOP
// *****************************************************************************************************************************************************************************************

void loop() {

  alarmButtonState = digitalRead(alarmButtonPin);
  if (alarmButtonState == LOW) {            // button pressed
    resetAlarm();
  }

  unsigned long ariaTime = millis();
  
  if(ariaTime - ariaTimesent > ariaInterval) {
    wifiReconnect();
    ariaTimesent = ariaTime;

    // Serial.println("Room temperature reading");
    // DHT22 reads
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      temperatura = event.temperature ;
    }
    // Onewire reads
    sensors.requestTemperatures();
    ariaIN = sensors.getTempC(airIncoming);
    ariaGPU = sensors.getTempC(airMiner);
    ariaOUT = sensors.getTempC(airOutgoing);
    // voltages read
    letturaVoltaggi();
    // energy read
    letturaEnergiaVoid();
    // fans rpm count
    countRPM();
    
    // thingspeak
    if (Boolthingspeak) {
      voltsTS = String(v_in33) + "," + String(v_in5) + "," + String(v_in12) ;
      airTS = String(ariaIN) + "," + String(ariaGPU) + "," + String(ariaOUT) ;
      rpmTS = String(rpm1) + "," + String(rpm2) + "," + String(rpm3) ;
      alarmTS = String(alarmFire) + "," + String(alarmTemp) + "," + String(alarmVolt) + "," + String(alarmFans) + "," + String(alarmAirvalve) ;
      sharesTS = String(accepted) + "," + String(invalid) + "," + String(rejected) ;
      energyTS = String(voltage) + "," + String(current) + "," + String(power) + "," + String(energy) ;
      ThingSpeak.setField(1, airTS);
      ThingSpeak.setField(2, alarmTS);
      ThingSpeak.setField(3, sharesTS);
      ThingSpeak.setField(4, temperatura);
      ThingSpeak.setField(5, energyTS);
      ThingSpeak.setField(6, rpmTS);
      ThingSpeak.setField(7, hashrate);
      ThingSpeak.setField(8, voltsTS);
      delay(50);
      int xTS = ThingSpeak.writeFields(Mconfig.idchannel.toInt(), Mconfig.apiwrite.c_str());
      if(xTS == 200){
        Serial.println("Thingspeak channel update successful.");
      }
      else{
        Serial.println("Problem updating Thingspeak channel. HTTP error code " + String(xTS));
      }
    }
  }

  // I fetch data from the pool

  unsigned long hashrateTime = millis();
  if(hashrateTime - hashrateTimesent > hashrateInterval) {
    //if (Mconfig.poolsUse != "0") {
      GetMinerData(IDminingSoftware);
    //}
    hashrateTimesent = hashrateTime;
  }

  // ------------------------------------------------I'm checking for a fire --------------------------
  
  valoreFIAMMA = digitalRead(sensoreFIAMMA);

  if (valoreFIAMMA == LOW) {                      // in case of flame
    Serial.println("Alarm!");
    if (!alarmFire) {
      Serial.println("Send notification");
      sendAlarmMessage(Alarm_fires, Alarm_fire_text);     // I send email
      alarmFire = true;
    }
    if(posizione == 1){                           // I move the air valve to expel any fumes
      Serial.println("I move the air valve");
      ariaCamino(); 
      }
    Serial.println("Increase the fan speed");
    freqFan = 1023 ;                              // I raise fan at 100%
    ledcWrite(pinFans, freqFan);
    Serial.println("I turn on the LED");
    digitalWrite(alarmLedPin, HIGH);
    delay(20000);                                  // I await the evacuation of any fumes
    // shutdown();                                // voltage cut-off
  }

  if (Boolairvalve) {
    if (Boolairvalveauto) {
      if(temperatura > (temperaturaAmbiente + 1) && posizione == 1){
        ariaCamino();
        Serial.print("air directed towards the chimney");
      }
      if(temperatura < (temperaturaAmbiente - 1) && posizione == 0){
        ariaCasa();
        Serial.print("air used to heat the room");
      }
    }
  }
  
  // ---------------------------------------- I regulate the miner cooling -----------------------
  
  if (BoolfanAuto) {
    if (ariaGPU > (Mconfig.minertemp.toInt() + 10)) {
      if (!alarmTemp) {
        sendAlarmMessage(Alarm_temps, Alarm_temps_text);
        alarmTemp = true ;
      }
    }
    if ((ariaGPU > (Mconfig.minertemp.toInt() + 2)) && (freqFan < 1022)){
      freqFan = freqFan + 1;
      ledcWrite(pinFans, freqFan);
      }
    if ((ariaGPU < (Mconfig.minertemp.toInt() - 2)) && (freqFan > 10)){
      freqFan = freqFan - 1;
      ledcWrite(pinFans, freqFan);
    }
  } else {
    Serial.println("fans set to manual function");
    ledcAttach(pinFans, freqFan, resolution);
    freqFan = map(Mconfig.fanSpeedSet.toInt(), 10, 100, 100, 1023); // ESP32 has 12-bit ADC resolution
    ledcWrite(pinFans, freqFan); 
  }

}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                                    FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

void loadConfig() {
  
  prefs.begin("miner", true); // solo lettura
  //prefs.clear();
  Mconfig.ipminer             = prefs.getString("ipminer", "");
  Mconfig.portminer           = prefs.getString("portminer", "");
  //Mconfig.ipminer             = ("192.168.1.5") ;
  Mconfig.wifiname            = prefs.getString("wifiname", "");
  Mconfig.wifipass            = prefs.getString("wifipass", "");
  Mconfig.nomeminer           = prefs.getString("nomeminer", "Miner Keeper");
  Mconfig.ambienttemp         = prefs.getString("ambienttemp", "20");
  Mconfig.minertemp           = prefs.getString("minertemp", "60");
  Mconfig.airvalve            = prefs.getString("airvalve", "0");
  Mconfig.airvalveauto        = prefs.getString("airvalveauto", "0");
  Mconfig.fanAuto             = prefs.getString("fanAuto", "1");           // fanAuto
  Mconfig.fanSpeedSet         = prefs.getString("fanSpeedSet", "10");         // fan speed i set in app
  Mconfig.fan2on              = prefs.getString("fan2on", "1");
  Mconfig.fan3on              = prefs.getString("fan3on", "1");
  Mconfig.valveAirPos         = prefs.getString("valveAirPos", "0");       // air valve position
  Mconfig.thingspeak          = prefs.getString("thingspeak", "0");
  Mconfig.idchannel           = prefs.getString("idchannel", "");
  Mconfig.apiwrite            = prefs.getString("apiwrite", "");
  Mconfig.apiread             = prefs.getString("apiread", "");
  Mconfig.notifs              = prefs.getString("notifs", "0"); 
  Mconfig.email_notifs        = prefs.getString("email_notifs", "0");       // email
  Mconfig.senderemail         = prefs.getString("senderemail", "");
  Mconfig.passwordemail       = prefs.getString("passwordemail", "");
  Mconfig.recipientemail      = prefs.getString("recipientemail", "");
  Mconfig.serveremail         = prefs.getString("serveremail", "");
  Mconfig.portemail           = prefs.getString("portemail", "");
  Mconfig.whatsapp_notifs     = prefs.getString("whatsapp_notifs", "0");    // whatsapp
  Mconfig.WS_phonenumber      = prefs.getString("WS_phonenumber", "");
  Mconfig.WS_callmebot        = prefs.getString("WS_callmebot", "");
  Mconfig.telegram_notifs     = prefs.getString("telegram_notifs", "0");    // telegram
  Mconfig.TG_username         = prefs.getString("TG_username", "");
  prefs.end();
}

void viewConfig() {
  Serial.print("IP miner saved : ");
  Serial.println(Mconfig.ipminer);
  Serial.print("Miner API port : ");
  Serial.println(Mconfig.portminer);
  Serial.print("WIFI name saved : ");
  Serial.println(Mconfig.wifiname);
  Serial.print("WIFI password saved : ");
  Serial.println(Mconfig.wifipass);
  Serial.print("Miner name saved : ");
  Serial.println(Mconfig.nomeminer);
  Serial.print("Reference ambient temperature saved : ");
  Serial.println(Mconfig.ambienttemp);
  Serial.print("Reference miner temperature saved : ");
  Serial.println(Mconfig.minertemp);
  Serial.println("Is the air valve installed? : ");
  Serial.println(Mconfig.airvalve);
  if (Mconfig.airvalve == "1") {
    Serial.print("Is the air valve automatic? : ");
    Serial.println(Mconfig.airvalveauto);
      if (Mconfig.airvalveauto == "0") {
      Serial.print("The valve is set to : ");
      if (Mconfig.valveAirPos == "0") {
        Serial.print("out");
      } else {
        Serial.print("in");
      }
    }
  }
  Serial.print("Are the fans automatic? : ");
  Serial.println(Mconfig.fanAuto);
  Serial.print("Do you use Thingspeak? : ");
  Serial.println(Mconfig.thingspeak);
  if (Mconfig.thingspeak == "1") {
    Serial.print("Thingspeak ID saved : ");
    Serial.println(Mconfig.idchannel);
    Serial.print("Saved Writing API : ");
    Serial.println(Mconfig.apiwrite);
    Serial.print("Saved Reading API : ");
    Serial.println(Mconfig.apiread);
  }
  Serial.print("Do you use notifications? : ");
  Serial.println(Mconfig.notifs);
  if (Mconfig.notifs == "1") {
    Serial.print("Do you use email notifications? : ");
    Serial.println(Mconfig.notifs);
    if (Mconfig.notifs == "1") {
      Serial.print("Sender name saved : ");
      Serial.println(Mconfig.senderemail);
      Serial.print("Recipient name saved : ");
      Serial.println(Mconfig.recipientemail);
      Serial.print("Email server saved : ");
      Serial.println(Mconfig.serveremail);
      Serial.print("Saved email server port : ");
      Serial.println(Mconfig.portemail);
    }
    Serial.print("Do you use WhatsApp notifications? : ");    // whatsapp
    Serial.println(Mconfig.whatsapp_notifs);
    if (Mconfig.whatsapp_notifs == "1") {
      Serial.print("Your phone number : ");
      Serial.println(Mconfig.WS_phonenumber);
      Serial.print("WhatsApp notification API : ");
      Serial.println(Mconfig.WS_callmebot);
    }
    Serial.print("Do you use Telegram notifications? : ");    // whatsapp
    Serial.println(Mconfig.telegram_notifs);
    if (Mconfig.telegram_notifs == "1") {
      Serial.print("Your nickname on TG : ");
      Serial.println(Mconfig.TG_username);
    }
  }
  delay(4000);
}

void printConfig () {
  JsonDocument docConfig;

  docConfig["nomeminer"] = Mconfig.nomeminer;
  docConfig["ip miner"] = Mconfig.ipminer;
  docConfig["miner port"] = Mconfig.portminer;
  docConfig["Wifi name"] = Mconfig.wifiname;
  docConfig["Wifi pass"] = Mconfig.wifipass;
  docConfig["Miner name"] = Mconfig.nomeminer;
  docConfig["Ambient temp"] = Mconfig.ambienttemp;
  docConfig["Miner temp"] = Mconfig.minertemp;
  docConfig["Air valve installed"] = Mconfig.airvalve;
  docConfig["Air valve auto"] = Mconfig.airvalveauto;
  docConfig["Fans auto"] = Mconfig.fanAuto;
  docConfig["Fans speed set"] = Mconfig.fanSpeedSet;
  docConfig["Fan 2 installed"] = Mconfig.fan2on;
  docConfig["Fan 3 installed"] = Mconfig.fan3on;
  docConfig["Air valve position"] = Mconfig.valveAirPos;
  docConfig["Thingspeak?"] = Mconfig.thingspeak;
  docConfig["ID Channel"] = Mconfig.idchannel;
  docConfig["API write"] = Mconfig.apiwrite;
  docConfig["API read"] = Mconfig.apiread;
  docConfig["Notifications?"] = Mconfig.notifs;
  docConfig["Email notifs?"] = Mconfig.email_notifs;
  docConfig["Email sender"] = Mconfig.senderemail;
  docConfig["Email pass"] = Mconfig.passwordemail;
  docConfig["Email recipient"] = Mconfig.recipientemail;
  docConfig["Email server"] = Mconfig.serveremail;
  docConfig["Email server port"] = Mconfig.portemail;
  docConfig["Whatsapp notifs?"] = Mconfig.whatsapp_notifs;
  docConfig["Phone number"] = Mconfig.WS_phonenumber;
  docConfig["API Callmebot"] = Mconfig.WS_callmebot;
  docConfig["Telegram notifs?"] = Mconfig.telegram_notifs;
  docConfig["Telegram username"] = Mconfig.TG_username;

  docConfig.shrinkToFit();

  serializeJson(docConfig, dataRequestJSON);
}

void requestData () {
  JsonDocument doc;

  doc["nomeminer"] = Mconfig.nomeminer;
  doc["ambient"] = temperatura;
  doc["tempIN"] = ariaIN;
  doc["tempMINER"] = ariaGPU;
  doc["tempOUT"] = ariaOUT;
  doc["valveAirPos"] = posizione;
  doc["fanAuto"] = Mconfig.fanAuto;
  doc["fanSpeedSet"] = Mconfig.fanSpeedSet;
  doc["fanspeedRead1"] = rpm1;
  doc["fanspeedRead2"] = rpm2;
  doc["fanspeedRead3"] = rpm3;
  doc["hashrateTS"] = hashrateTS;
  doc["paidtotal"] = paidtotal;
  doc["balance"] = balance;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["volt33"] = v_in33;
  doc["volt5"] = v_in5;
  doc["volt12"] = v_in12;
  doc["aValve"] = alarmAirvalve;
  doc["aFans"] = alarmFans;
  doc["aFire"] = alarmFire;
  doc["aVolt"] = alarmVolt;
  doc["aTemp"] = alarmTemp;
  // Mining software part
  doc["ipminer"] = Mconfig.ipminer;
  doc["portminer"] = Mconfig.portminer;
  doc["software"] = software;
  doc["uptime"] = uptime ;
  doc["lastUpdate"] = lastUpdateFormatted ;
  doc["algorithm"] = algorithm ;
  doc["walletAddress"] = walletAddress ;
  doc["poolAddress"] = poolAddress ;
  doc["accepted"] = accepted ;
  doc["invalid"] = invalid ;
  doc["rejected"] = rejected ;
  doc["gpuPower"] = gpuPower ;
  doc["hashrate"] = hashrate ;
  doc["gpuName"] = gpuName ;
  doc["gpuTotalMem"] = gpuTotalMem ;
  doc["osName"] = osName ;
  doc["cudaDriver"] = cudaDriver ;
  doc["gpuCoreTemp"] = gpuCoreTemp ;
  doc["gpuMemTemp"] = gpuMemTemp ;
  doc["gpuFan"] = gpuFan ;
  doc["performanceUnit"] = performanceUnit ;

  doc.shrinkToFit();

  serializeJson(doc, dataRequestJSON);
}

void settings_on_off() {
  if (String(Mconfig.thingspeak) == "1") {
    Boolthingspeak = true;
  } else {
    Boolthingspeak = false;
  }
  if (String(Mconfig.airvalve) == "1") {
    Boolairvalve = true ;
  } else {
    Boolairvalve = false ;
  }
  if (String(Mconfig.airvalveauto) == "1") {
    Boolairvalveauto = true ;
  } else {
    Boolairvalveauto = false ;
  }
  if (String(Mconfig.fanAuto) == "1") {
    BoolfanAuto = true ;
  } else {
    BoolfanAuto = false ;
  }
  if (String(Mconfig.fan2on) == "1") {
    Boolfan2on = true;
  } else {
    Boolfan2on = false;
  }
  if (String(Mconfig.fan3on) == "1") {
    Boolfan3on = true;
  } else {
    Boolfan3on = false;
  }
  if (String(Mconfig.notifs) == "1") {
    Boolnotifs = true ;
  } else {
    Boolnotifs = false ;
  }
  if (String(Mconfig.email_notifs) == "1") {
    Boolemail = true ;
  } else {
    Boolemail = false ;
  }
  if (String(Mconfig.whatsapp_notifs) == "1") {
    Boolwhatsapp = true ;
  } else {
    Boolwhatsapp = false ;
  }
  if (String(Mconfig.telegram_notifs) == "1") {
    Booltelegram = true ;
  } else {
    Booltelegram = false ;
  }
}

void wifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin();
  }
}

void countRPM() {
  pcnt_get_counter_value(FAN1_UNIT, &count1);
  pcnt_counter_clear(FAN1_UNIT); // Le ventole Arctic F12 PWM PST generano 2 impulsi per giro
  rpm1 = (count1 / 2) * 3;
  Serial.printf("FAN1: impulsi=%d RPM=%d\n", count1, rpm1);
  if (rpm1 < 300) {
    if (!alarmFans) {
      sendAlarmMessage(Alarm_fanss, Alarm_fans_text);
      alarmFans = true;
    }
  }
  if (Boolfan2on) {
    pcnt_get_counter_value(FAN2_UNIT, &count2);
    pcnt_counter_clear(FAN2_UNIT);
    rpm2 = (count2 / 2) * 3;
    Serial.printf("FAN2: impulsi=%d RPM=%d\n", count2, rpm2);
    if (rpm2 < 300) {
      if (!alarmFans) {
        sendAlarmMessage(Alarm_fanss, Alarm_fans_text);
        alarmFans = true;
      }
    }
  }
  if (Boolfan3on) {
    pcnt_get_counter_value(FAN3_UNIT, &count3);
    pcnt_counter_clear(FAN3_UNIT);
    rpm3 = (count3 / 2) * 3;
    Serial.printf("FAN3: impulsi=%d RPM=%d\n\n", count3, rpm3);
    if (rpm3 < 300) {
      if (!alarmFans) {
        sendAlarmMessage(Alarm_fanss, Alarm_fans_text);
        alarmFans = true;
      }
    }
  }
}

void ariaCamino() {
  for (int posServo = 0; posServo <= maxAperturaServo; posServo += 1) {
    servoAir.write(posServo);
    delay(10);
  }
  posizione = 0; // position 0 equals air outwards
}

void ariaCasa() {
  for (int posServo = maxAperturaServo; posServo >= 0; posServo -= 1) {
    servoAir.write(posServo);
    delay(10); 
  }
  posizione = 1; // position 1 equals air inwards
}

void resetAlarm() {
  digitalWrite(alarmLedPin, LOW);
  digitalWrite(alarmRele, LOW);
  alarmTemp = false;
  alarmVolt = false;
  alarmFans = false;
  alarmAirvalve = false;
  alarmFire = false;
}

void letturaVoltaggi() {
  float media12 = 0.00;
  float media5 = 0.00;
  float media33 = 0.00;

  // --------------------------------- misura 12 volt ----------------------------

  float adc12 = analogRead(pin12volt);
  float v_adc12 = (adc12 / 4095.0) * 3.3;
  v_in12 = v_adc12 * 3.7 * cali12v;
  Serial.print("12V measured: ");
  Serial.println(v_in12);

  if (!alarmVolt) {
    if (((v_in12 * 100) < 1140) || ((v_in12 * 100) > 1260)){
      delay(50);
      Serial.print("12 volts is wrong : ");
      Serial.println(v_in12);
      if (!alarmVolt) {
        sendAlarmMessage(Alarm_volts_12, Alarm_volt_text_12);
        alarmVolt = true;
      }
    }
  }

  // --------------------------------- misura 5 volt ----------------------------
  float adc5 = analogRead(pin5volt);
  float v_adc5 = (adc5 / 4095.0) * 3.3;
  v_in5 = v_adc5 * 1.985 * cali5v;

  Serial.print("5V measured: ");
  Serial.println(v_in5);

  if (!alarmVolt) {
    if (((v_in5 * 100) < 475) || ((v_in5 * 100) > 550)){
      delay(50);
      Serial.print("5 volts is wrong : ");
      Serial.println(v_in5);
      if (!alarmVolt) {
        sendAlarmMessage(Alarm_volts_5, Alarm_volt_text_5);
        alarmVolt = true;
      }
    }
  }
  // --------------------------------- misura 3.3 volt ----------------------------
  float adc33 = analogRead(pin33volt);
  float v_adc33 = (adc33 / 4095.0) * 3.3;
  v_in33 = v_adc33 * cali33v;

  Serial.print("3.3V measured: ");
  Serial.println(v_in33);

  if (!alarmVolt) {
    if (((v_in33 * 100) < 315) || ((v_in33 * 100) > 345)){
      delay(50);
      Serial.print("3.3 volts is wrong : ");
      Serial.println(v_in33);
      if (!alarmVolt) {
        sendAlarmMessage(Alarm_volts_33, Alarm_volt_text_33);
        alarmVolt = true;
      }
    }
  }
}

void sendAlarmMessage (String alarmType, String alarmText ) {
  digitalWrite(alarmLedPin, HIGH);
  digitalWrite(alarmRele, HIGH);
  if (Boolemail) {
          sendEmail (alarmType, alarmText);
        }
  if (Boolwhatsapp) {
          sendWhatsapp (alarmType, alarmText);
        }
  if (Booltelegram) {
          sendTelegram (alarmType, alarmText);
        }
}

void letturaEnergiaVoid() {
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
}

void ResetEnergiaVoid() {
  pzem.resetEnergy();
  Serial.println("Valore di energia resettato");
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------      EMAIL       -------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

void sendEmail (String subjectTosend, String messageTosend ) {
  SMTP_Message message ;
  message.sender.name = String(Mconfig.nomeminer);
  message.sender.email = String(Mconfig.senderemail);
  String fullSubject = String(subjectTosend) + " on miner " + Mconfig.nomeminer;
  message.subject = fullSubject.c_str();
  message.addRecipient(F("user"), String(Mconfig.recipientemail));
  message.text.content = messageTosend.c_str();
  if (!smtp.isLoggedIn())
        {
            if (!smtp.connect(&config))
            {
                MailClient.printf("Connection error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
                goto exit;
            }
            if (!smtp.isLoggedIn())
            {
                Serial.println("Error, Not yet logged in.");
                goto exit;
            }
            else
            {
                if (smtp.isAuthenticated())
                    Serial.println("Successfully logged in.");
                else
                    Serial.println("Connected with no Auth.");
            }
        }
        if (!MailClient.sendMail(&smtp, &message, false))
            MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    exit:
        heapInfo.collect();
        heapInfo.print();
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
    if (status.success())
    {
        Serial.println("----------------");
        MailClient.printf("Message sent success: %d\n", status.completedCount());
        MailClient.printf("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");
        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            SMTP_Result result = smtp.sendingResult.getItem(i);
            MailClient.printf("Message No: %d\n", i + 1);
            MailClient.printf("Status: %s\n", result.completed ? "success" : "failed");
            MailClient.printf("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            MailClient.printf("Recipient: %s\n", result.recipients.c_str());
            MailClient.printf("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");
        smtp.sendingResult.clear();
    }
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------    WHATSAPP      -------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

void sendWhatsapp (String subjectTosend, String messageTosend) {
  String wwwWSCallmebot = "https://api.callmebot.com/whatsapp.php?phone=" + Mconfig.WS_phonenumber + "&apikey=" + Mconfig.WS_callmebot + "&text=" + urlEncode(messageTosend + " Miner affected : " + Mconfig.nomeminer) ;
  HTTPClient http;
  http.begin(wwwWSCallmebot);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("notifica inviata");
      } else {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------    TELEGRAM      -------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

void sendTelegram (String subjectTosend, String messageTosend) {
  String wwwTGCallmebot = "https://api.callmebot.com/text.php?user=" + Mconfig.TG_username + "&text=" + urlEncode(messageTosend + " Miner affected : " + Mconfig.nomeminer) ;
  HTTPClient http;
  http.begin(wwwTGCallmebot);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("notifica inviata");
      } else {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                 MINER SOFTWARE QUERY
// -------------------------------------------------------------------------------------------------------------------------------------------------------
bool jsonContains(JsonVariant element, const char* text) {
  if (element.is<const char*>()) {
    return String(element.as<const char*>()).indexOf(text) >= 0;
  }

  if (element.is<JsonObject>()) {
    for (JsonPair kv : element.as<JsonObject>()) {
      if (jsonContains(kv.value(), text)) return true;
    }
  }

  if (element.is<JsonArray>()) {
    for (JsonVariant v : element.as<JsonArray>()) {
      if (jsonContains(v, text)) return true;
    }
  }

  return false;
}

// ------------------------------------------ detect mining software --------------------------------------------

String queryHTTP(String url) {
  HTTPClient http;
  http.begin(url);
  int code = http.GET();

  if (code > 0) {
    String payload = http.getString();
    http.end();
    return payload;
  }

  http.end();
  return "";
}

String queryCgminer(String ip, int port, String command) {
  WiFiClient client;

  if (!client.connect(ip.c_str(), port)) {
    return "";
  }

  client.print(command);
  delay(200);

  String response = "";
  while (client.available()) {
    response += client.readString();
  }

  client.stop();
  return response;
}

String minerAPI(String ip, int httpPort, int cgPort, String httpEndpoint, String cgCommand) { // function to detect cgminer ASIC
  // 1) Prova HTTP
  String httpURL = "http://" + ip + ":" + String(httpPort) + httpEndpoint;
  String httpResp = queryHTTP(httpURL);

  if (httpResp.length() > 0) {
    return httpResp;
  }

  // 2) Prova cgminer
  String cgResp = queryCgminer(ip, cgPort, cgCommand);

  if (cgResp.length() > 0) {
    Serial.print("risposta da cgminer: ");
    Serial.println(cgResp);
    return cgResp;
  }

  return "";
}

void detectSoftware() {

  String response = minerAPI(
    String(Mconfig.ipminer),     // IP miner
    Mconfig.portminer.toInt(),           // Porta HTTP (6060 o 80)
    4028,                        // Porta cgminer da cambiare in variabile se voglio che l'utente la possa scegliere
    "/",                         // endpoint HTTP base
    "{\"command\":\"pools\"}"  // comando cgminer
  );

  if (response.length() == 0) {
    Serial.println("Nessuna risposta da API miner");
    resetMinerData() ; 
    return;
  }

  Serial.println("Risposta miner:");
  Serial.println(response);

  // --- RILEVAZIONE HTML ---
  bool isHtml = response.indexOf('<') >= 0;
  bool isBzMinerHtml = response.indexOf("<meta property=\"og:site_name\" content=\"BzMiner - GUI\">") >= 0;

  if (isHtml && isBzMinerHtml) {
      Serial.println("HTML di BzMiner rilevato");
      IDminingSoftware = "1";
      return;
  } 
  else if (isHtml) {
      Serial.println("HTML generico, non BzMiner");
      return;
  }

  // --- PARSING JSON ---
  JsonDocument docPool;
  DeserializationError err = deserializeJson(docPool, response);

  if (err) {
    Serial.print("Errore JSON: ");
    Serial.println(err.c_str());
    return;
  }

  JsonVariant root = docPool.as<JsonVariant>();

  if (jsonContains(root, "lolMiner")) IDminingSoftware = "2";
  if (jsonContains(root, "Rigel"))    IDminingSoftware = "3";
  if (jsonContains(root, "WildRig"))  IDminingSoftware = "4";
  if (jsonContains(root, "cgminer"))  IDminingSoftware = "5";
  if (jsonContains(root, "Gminer"))   IDminingSoftware = "7";
}

// ------------------------------------------- erase fields in case of mining software not reachable -------------------------------

void resetMinerData () {

  software = "n.d.";
  gpuName = "n.d.";
  gpuTotalMem = "0";
  osName = "n.d.";
  cudaDriver = "n.d.";
  uptime = "";
  algorithm = "n.d.";
  walletAddress = "n.d.";
  poolAddress = "n.d.";
  accepted = "n.d.";
  invalid = "n.d.";
  rejected = "n.d.";
  gpuPower = "n.d.";
  hashrate = 0.0;
  gpuCoreTemp = "n.d.";
  gpuMemTemp = "n.d.";
  gpuFan = "n.d.";

}

// ------------------------------------------------------------- Get miner data ---------------------------------------------

void GetMinerData(String minerFound) {
  if (minerFound == "") {
    detectSoftware();
  } else {
    Serial.print("Detected miner software: ");
    Serial.println(minerFound);
    HTTPClient http;
    if (IDminingSoftware == "1") {
      minerAddressAPI = "http://" + String(Mconfig.ipminer) + ":" + String(Mconfig.portminer) + "/status";
    }
    else if (IDminingSoftware == "5") {
      minerAddressAPI = "http://" + String(Mconfig.ipminer) + ":" + String(Mconfig.portminer) + "/status";
    } else {
    minerAddressAPI = "http://" + String(Mconfig.ipminer) + ":" + String(Mconfig.portminer);
    }
    http.begin(minerAddressAPI);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
          JsonDocument docPool;
          deserializeJson(docPool, payload);
          // ---------------------------------------------------- BZminer -------------------------------------
          if (IDminingSoftware == "1") {
              Serial.println("BZminer detected");
              software      = getJsonValue<String>(docPool["bzminer_version"], "0");
              gpuName       = getJsonValue<String>(docPool["devices"][1]["name"], "0");
              gpuTotalMem   = getJsonValue<String>(docPool["devices"][1]["total_memory"], "0");
              osName        = getJsonValue<String>(docPool["os_name"], "0");
              cudaDriver    = getJsonValue<String>(docPool["cuda_driver_version"], "0");
              uptime        = getJsonValue<String>(docPool["uptime_s"], "0");
              algorithm     = getJsonValue<String>(docPool["pools"][0]["algorithm"], "0");
              walletAddress = getJsonValue<String>(docPool["pools"][0]["wallet"], "0");
              poolAddress   = getJsonValue<String>(docPool["pools"][0]["current_url"], "0");
              accepted      = getJsonValue<String>(docPool["pools"][0]["valid_solutions"], "0");
              invalid       = getJsonValue<String>(docPool["pools"][0]["invalid_solutions"], "0");
              rejected      = getJsonValue<String>(docPool["pools"][0]["rejected_solutions"], "0");
              gpuPower      = getJsonValue<String>(docPool["pools"][1]["power"], "0");
              hashrate      = getJsonValue<float>(docPool["pools"][0]["hashrate"], 0.0f);
              gpuCoreTemp   = getJsonValue<String>(docPool["devices"][1]["core_temp"], "0");
              gpuMemTemp    = getJsonValue<String>(docPool["devices"][1]["mem_temp"], "0");
              gpuFan        = getJsonValue<String>(docPool["devices"][1]["fan"], "0");
          }

          // ---------------------------------------------------- lolminer ------------------------------------
          if (IDminingSoftware == "2") {
              Serial.println("Lolminer detected");
              software        = getJsonValue<String>(docPool["Software"], "0");
              uptime          = getJsonValue<String>(docPool["Session"]["Uptime"], "0");
              lastUpdate      = getJsonValue<String>(docPool["Session"]["Last_Update"], "0");
              gpuName         = getJsonValue<String>(docPool["workers"][0]["name"], "0");
              gpuTotalMem     = getJsonValue<String>(docPool["devices"][1]["total_memory"], "0");
              osName          = getJsonValue<String>(docPool["os_name"], "0");
              cudaDriver      = getJsonValue<String>(docPool["cuda_driver_version"], "0");
              gpuPower        = getJsonValue<String>(docPool["Workers"][0]["Power"], "0");
              gpuCoreTemp     = getJsonValue<String>(docPool["Workers"][0]["Core_Temp"], "0");
              gpuMemTemp      = getJsonValue<String>(docPool["Workers"][0]["Mem_Temp"], "0");
              gpuFan          = getJsonValue<String>(docPool["Workers"][0]["Fan_Speed"], "0");
              algorithm       = getJsonValue<String>(docPool["Algorithms"][0]["Algorithm"], "0");
              poolAddress     = getJsonValue<String>(docPool["Algorithms"][0]["Pool"], "0");
              walletAddress   = getJsonValue<String>(docPool["Algorithms"][0]["User"], "0");
              performanceUnit = getJsonValue<String>(docPool["Algorithms"][0]["Performance_Unit"], "0");
              accepted        = getJsonValue<String>(docPool["Algorithms"][0]["Total_Accepted"], "0");
              invalid         = getJsonValue<String>(docPool["Algorithms"][0]["Total_Stales"], "0");
              rejected        = getJsonValue<String>(docPool["Algorithms"][0]["Total_Rejected"], "0");
              hashrate        = getJsonValue<float>(docPool["Algorithms"][0]["Worker_Performance"], 0.0f);
              if (performanceUnit == "Mh/s") {
                  hashrate *= 1000000;
              }
          }

          // ---------------------------------------------------- rigelminer ----------------------------------
          if (IDminingSoftware == "3") {
              Serial.println("Rigel detected");
              software1 = getJsonValue<String>(docPool["name"], "0");
              software2 = getJsonValue<String>(docPool["version"], "0");
              software  = software1 + " " + software2;
              uptime        = getJsonValue<String>(docPool["uptime"], "0");
              algorithm     = getJsonValue<String>(docPool["algorithm"], "0");
              walletAddress = getJsonValue<String>(docPool["pools"][algorithm][0]["connection_details"]["username"], "0");
              poolAddressLink = getJsonValue<String>(docPool["pools"][algorithm][0]["connection_details"]["hostname"], "0");
              poolAddressPort = getJsonValue<String>(docPool["pools"][algorithm][0]["connection_details"]["port"], "0");
              poolAddress   = poolAddressLink + ":" + poolAddressPort;
              accepted      = getJsonValue<String>(docPool["pools"][algorithm][0]["solution_stat"]["accepted"], "0");
              rejected      = getJsonValue<String>(docPool["pools"][algorithm][0]["solution_stat"]["rejected"], "0");
              invalid       = getJsonValue<String>(docPool["pools"][algorithm][0]["solution_stat"]["invalid"], "0");
              gpuName       = getJsonValue<String>(docPool["devices"][0]["name"], "0");
              gpuTotalMem   = getJsonValue<String>(docPool["devices"][0]["total_mem"], "0");
              osName        = getJsonValue<String>(docPool["os_name"], "0");
              cudaDriver    = getJsonValue<String>(docPool["cuda_driver"], "0");
              gpuCoreTemp   = getJsonValue<String>(docPool["devices"][0]["monitoring_info"]["core_temperature"], "0");
              gpuMemTemp    = getJsonValue<String>(docPool["devices"][0]["monitoring_info"]["memory_temperature"], "0");
              gpuFan        = getJsonValue<String>(docPool["devices"][0]["monitoring_info"]["fan_speed"], "0");
              gpuPower      = getJsonValue<String>(docPool["devices"][0]["monitoring_info"]["power_usage"], "0");
              hashrate      = getJsonValue<float>(docPool["hashrate"][algorithm], 0.0f);
          }

          // ----------------------------------------------------- wildrig ------------------------------------
          if (IDminingSoftware == "4") {
              Serial.println("Wildrig detected");
              software      = getJsonValue<String>(docPool["ua"], "0");
              uptime        = getJsonValue<String>(docPool["uptime"], "0");
              algorithm     = getJsonValue<String>(docPool["algo"], "0");
              poolAddressLink = getJsonValue<String>(docPool["connection"]["pool"], "0");
              poolAddress     = poolAddressLink;
              accepted      = getJsonValue<String>(docPool["results"]["shares_accepted"][0], "0");
              rejected      = getJsonValue<String>(docPool["results"]["shares_rejected"][0], "0");
              invalid       = getJsonValue<String>(docPool["results"]["shares_ignored"][0], "0");
              gpuCoreTemp   = getJsonValue<String>(docPool["hwmon"]["temp"][0], "0");
              gpuMemTemp    = getJsonValue<String>(docPool["hwmon"][0]["monitoring_info"]["memory_temperature"][0], "0");
              gpuFan        = getJsonValue<String>(docPool["hwmon"]["fan"][0], "0");
              gpuPower      = getJsonValue<String>(docPool["hwmon"]["power"][0], "0");
              hashrate      = getJsonValue<float>(docPool["hashrate"]["total"], 0.0f);
          }

          // ----------------------------------------------------- cgminer ------------------------------------
          if (IDminingSoftware == "5") {
              Serial.println("cgminer detected");
              software      = getJsonValue<String>(docPool["STATUS"]["Description"], "0");
              uptime        = getJsonValue<String>(docPool["STATUS"]["When"], "0");
              algorithm     = "null";  // cgminer non lo fornisce
              walletAddress = getJsonValue<String>(docPool["POOLS"]["User"], "0");
              poolAddressLink = getJsonValue<String>(docPool["POOLS"]["URL"], "0");
              poolAddress   = poolAddressLink;
              accepted      = getJsonValue<String>(docPool["POOLS"]["Accepted"], "0");
              rejected      = getJsonValue<String>(docPool["POOLS"]["Rejected"], "0");
              invalid       = getJsonValue<String>(docPool["POOLS"]["Discarded"], "0");
              gpuCoreTemp   = getJsonValue<String>(docPool["hwmon"]["temp"], "0");
              gpuMemTemp    = getJsonValue<String>(docPool["hwmon"][0]["monitoring_info"]["memory_temperature"], "0");
              gpuFan        = getJsonValue<String>(docPool["hwmon"]["fan"], "0");
              gpuPower      = getJsonValue<String>(docPool["hwmon"]["power"], "0");
              hashrate      = getJsonValue<float>(docPool["POOLS"]["MHS av"], 0.0f);
          }

          // ----------------------------------------------------- Gminer ------------------------------------
          if (IDminingSoftware == "7") {
              Serial.println("Gminer detected");
              software      = getJsonValue<String>(docPool["miner"], "0");
              uptime        = getJsonValue<String>(docPool["uptime"], "0");
              algorithm     = getJsonValue<String>(docPool["algorithm"], "0");
              walletAddress = getJsonValue<String>(docPool["user"], "0");
              poolAddressLink = getJsonValue<String>(docPool["server"], "0");
              poolAddress   = poolAddressLink;
              accepted      = getJsonValue<String>(docPool["devices"]["0"]["accepted_shares"], "0");
              rejected      = getJsonValue<String>(docPool["devices"]["0"]["rejected_shares"], "0");
              invalid       = getJsonValue<String>(docPool["devices"]["0"]["invalid_shares"], "0");
              gpuCoreTemp   = getJsonValue<String>(docPool["devices"]["0"]["temperature"], "0");
              gpuMemTemp    = getJsonValue<String>(docPool["devices"]["0"]["memory_temperature"], "0");
              gpuFan        = getJsonValue<String>(docPool["devices"]["0"]["fan"], "0");
              gpuPower      = getJsonValue<String>(docPool["devices"]["0"]["power_usage"], "0");
              hashrate      = getJsonValue<float>(docPool["devices"]["0"]["speed"], 0.0f);
              if (performanceUnit == "G/s") {
                  hashrate *= 1000000000;
              }
          }

        timeClient.update();
        now = timeClient.getEpochTime();
        lastUpdateUL = lastUpdate.toInt();
        diffNow = now - lastUpdateUL;
        lastUpdateFormatted = formatMMSS(diffNow);

      } else {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);
          detectSoftware();
        }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      minerFound = "" ;
      IDminingSoftware = "";
    }
    http.end();
  }
}

// ----------------------------------------------------------------- END ------------------------------------------------------------
