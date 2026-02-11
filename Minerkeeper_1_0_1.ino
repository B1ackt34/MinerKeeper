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

// ---------------------------------------------------- Miner Keeper version

const char * versione = "1.1.8";              // software version

// ---------------------------------------------------- App connection variables

Preferences prefs;
AsyncWebServer server(80);

struct MinerConfig {
  String wifiname;
  String wifipass;
  String nomeminer;
  String poolsUse;
  String walletaddr;
  String minerpass;
  String cointicker;
  String poolSel;
  String ambienttemp;
  String minertemp;
  String airvalve;
  String airvalveauto;
  String fanAuto;         // fan settings
  String fanSpeed;
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

bool BoolpoolsUse = false ;
bool Boolthingspeak = false ;
bool Boolairvalve = false ;
bool Boolairvalveauto = true ;
bool BoolfanAuto = true ;
bool Boolnotifs = false ;
bool Boolemail = false ;
bool Boolwhatsapp = false ;
bool Booltelegram = false ;

MinerConfig Mconfig;

// ----------------------------------------------------- wifi

unsigned long previousMillis = 0;
const long interval = 10000;              // interval to wait for Wi-Fi connection (milliseconds)

WiFiClient  client;

const char* ssid = "";
const char* password = "";

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

// ----------------------------------------------------- voltmeter ammeter + power supply measurements
  
PZEM004Tv30 pzem(Serial2, 16, 17);
float voltage ;
float current ;
float power ;
float energy ;

const int pin12volt = 35;
const int pin5volt = 34;
const int pin33volt = 39;
float vin12 = 0;
float vin5 = 0;
float vin33 = 0;
float tempvin12 = 0;
float tempvin5 = 0;
float tempvin33 = 0;
float misura12 = 0;
float misura5 = 0;
float misura33 = 0;

// ----------------------------------------------------- OneWire temperature probes
  
#define ONE_WIRE_BUS 18                             // Temperature probe pin
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
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22            // DHT 22 (AM2302)
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

int fansSet = 0;
const int pinTacho = 36;
volatile int tachCount = 0;
volatile unsigned long counterRPM = 0;
long rpmFans = 0;
void countPulse();

// --------------------------------------------------- HTTP and JSON handling

String poolAddressAPI ; //
String dataRequestJSON = "" ;

float balance ;
float paidtotal ;
const char* currency ;
long hashrateTS = 0 ;

unsigned long hashrateTimesent = 0;
unsigned long hashrateInterval = 40000;             // time interval between hashrate requests to the pool

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

// ---------------------------------------------------- Alarms

int alarmLedPin = 23;      // alarm led
int alarmRele = 21;        // output relay in case of alarm
int alarmButtonPin = 32;
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
  pinMode(pinTacho, INPUT_PULLUP);
  attachInterrupt(pinTacho, countTach, FALLING);

  // I recover the saved configuration
  loadConfig();

  // I publish the configuration on the Serial monitor
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
      prefs.putString("wifiname", doc["wifiname"].as<String>());
      prefs.putString("wifipass", doc["wifipass"].as<String>());
      prefs.putString("nomeminer", doc["nomeminer"].as<String>());
      prefs.putString("poolsUse", doc["poolsUse"].as<String>());
      prefs.putString("walletaddr", doc["walletaddr"].as<String>());
      prefs.putString("minerpass", doc["minerpass"].as<String>());
      prefs.putString("cointicker", doc["cointicker"].as<String>());
      prefs.putString("poolSel", doc["poolSel"].as<String>());
      prefs.putString("ambienttemp", doc["ambienttemp"].as<String>());
      prefs.putString("airvalve", doc["airvalve"].as<String>());          // air valve present or not
      prefs.putString("airvalveauto", doc["airvalveauto"].as<String>());  // air valve in automatic operation or not
      prefs.putString("minertemp", doc["minertemp"].as<String>());
      prefs.putString("fanAuto", doc["fanAuto"].as<String>());
      prefs.putString("fanSpeed", doc["fanSpeed"].as<String>());
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
    freqFan = map(Mconfig.fanSpeed.toInt(), 10, 100, 100, 1023); // ESP32 has 12-bit ADC resolution
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

    Serial.println("Room temperature reading");
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
      voltsTS = String(misura33) + "," + String(misura5) + "," + String(misura12) ;
      ThingSpeak.setField(1, ariaIN);
      ThingSpeak.setField(2, ariaGPU);
      ThingSpeak.setField(3, ariaOUT);
      ThingSpeak.setField(4, temperatura);
      ThingSpeak.setField(5, energy);
      ThingSpeak.setField(6, rpmFans);
      ThingSpeak.setField(7, hashrateTS);
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
    if (Mconfig.poolsUse != "0") {
      callPool(String(Mconfig.poolSel));
    }
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
    fansSet = 4;
    Serial.println("I turn on the LED");
    ledcWrite(pinFans, freqFan);
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
  }

}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                                    FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

void loadConfig() {
  
  prefs.begin("miner", true); // solo lettura
  //prefs.clear();
  Mconfig.wifiname            = prefs.getString("wifiname", "");
  Mconfig.wifipass            = prefs.getString("wifipass", "");
  Mconfig.nomeminer           = prefs.getString("nomeminer", "Miner Keeper");
  Mconfig.poolsUse            = prefs.getString("poolsUse", "0");
  Mconfig.walletaddr          = prefs.getString("walletaddr", "");
  Mconfig.minerpass           = prefs.getString("minerpass", "");
  Mconfig.cointicker          = prefs.getString("cointicker", "");
  Mconfig.poolSel             = prefs.getString("poolSel", "0");
  Mconfig.ambienttemp         = prefs.getString("ambienttemp", "20");
  Mconfig.minertemp           = prefs.getString("minertemp", "60");
  Mconfig.airvalve            = prefs.getString("airvalve", "0");
  Mconfig.airvalveauto        = prefs.getString("airvalveauto", "0");
  Mconfig.fanAuto             = prefs.getString("fanAuto", "1");           // fanAuto
  Mconfig.fanSpeed            = prefs.getString("fanSpeed", "10");         // fanAuto
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
  Serial.print("WIFI name saved : ");
  Serial.println(Mconfig.wifiname);
  Serial.print("WIFI password saved : ");
  Serial.println(Mconfig.wifipass);
  Serial.print("Miner name saved : ");
  Serial.println(Mconfig.nomeminer);
  Serial.print("Do you use pools? : ");
  Serial.println(Mconfig.poolsUse);
  if (Mconfig.poolsUse == "1") {
    Serial.print("Saved wallet address : ");
    Serial.println(Mconfig.walletaddr);
    Serial.print("Password miner saved : ");
    Serial.println(Mconfig.minerpass);
    Serial.print("Name of the coin minted : ");
    Serial.println(Mconfig.cointicker);
    Serial.print("Pool used : ");
    Serial.println(Mconfig.poolSel);
  }
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
  docConfig["Wifi name"] = Mconfig.wifiname;
  docConfig["Wifi pass"] = Mconfig.wifipass;
  docConfig["Miner name"] = Mconfig.nomeminer;
  docConfig["Pools?"] = Mconfig.poolsUse;
  docConfig["Wallet address"] = Mconfig.walletaddr;
  docConfig["Miner pass"] = Mconfig.minerpass;
  docConfig["Cointicker"] = Mconfig.cointicker;
  docConfig["Pool selected"] = Mconfig.poolSel;
  docConfig["Ambient temp"] = Mconfig.ambienttemp;
  docConfig["Miner temp"] = Mconfig.minertemp;
  docConfig["Air valve installed"] = Mconfig.airvalve;
  docConfig["Air valve auto"] = Mconfig.airvalveauto;
  docConfig["Fans auto"] = Mconfig.fanAuto;
  docConfig["Fans speed"] = Mconfig.fanSpeed;
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
  doc["fanspeed"] = rpmFans;
  doc["hashrateTS"] = hashrateTS;
  doc["paidtotal"] = paidtotal;
  doc["balance"] = balance;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["volt33"] = misura33;
  doc["volt5"] = misura5;
  doc["volt12"] = misura12;
  doc["aValve"] = alarmAirvalve;
  doc["aFans"] = alarmFans;
  doc["aFire"] = alarmFire;
  doc["aVolt"] = alarmVolt;
  doc["aTemp"] = alarmTemp;

  doc.shrinkToFit();

  serializeJson(doc, dataRequestJSON);
}

void settings_on_off() {
  if (String(Mconfig.poolsUse) == "1") {
    BoolpoolsUse = true;
  } else {
    BoolpoolsUse = false;
  }
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

void countTach() { // fan RPM measure
  tachCount++;
}

void countRPM() {
  rpmFans = (tachCount / 2) * 3;
  if (rpmFans < 100) {
    if (!alarmFans) {
      sendAlarmMessage(Alarm_fanss, Alarm_fans_text);
      alarmFans = true;
    }
  }
  tachCount = 0;
  Serial.print("Fans rpm ");
  Serial.println(rpmFans);
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
  float somma12 = 0.00;
  float media12 = 0.00;
  for(int c12=0; c12<=9; c12++) {
    vin12 = analogRead(pin12volt);
    somma12 = somma12 + vin12 ;
    delay(1);
  }
  media12 = somma12 / 10;
  misura12 = ((media12 * 3.3) / 4095) / 0.156 ;
  if (!alarmVolt) {
    if (((misura12 * 100) < 1140) || ((misura12 * 100) > 1260)){
      delay(50);
      Serial.print("12 volts is wrong : ");
      Serial.println(misura12);
      if (!alarmVolt) {
        sendAlarmMessage(Alarm_volts, Alarm_volt_text);
        alarmVolt = true;
      }
    }
  }
  float somma5 = 0.00;
  float media5 = 0.00;
  for(int c5=0; c5<=9; c5++) {
    vin5 = analogRead(pin5volt);
    somma5 = somma5 + vin5;
    delay(1);
  }
  media5 = somma5 / 10;
  misura5 = ((media5 * 3.3) /4095) / 0.2 ;
  if (!alarmVolt) {
    if (((misura5 * 100) < 475) || ((misura5 * 100) > 550)){
      delay(50);
      Serial.print("5 volts is wrong : ");
      Serial.println(misura5);
      if (!alarmVolt) {
        sendAlarmMessage(Alarm_volts, Alarm_volt_text);
        alarmVolt = true;
      }
    }
  }
  float somma33 = 0.00;
  float media33 = 0.00;
  for(int c33=0; c33<=9; c33++) {
    vin33 = analogRead(pin33volt);
    somma33 = somma33 + vin33;
    delay(1);
  }
  media33 = somma33 / 10;
  misura33 = ((media33 * 3.3) /4095) / 0.29 ;
  if (!alarmVolt) {
    if (((misura33 * 100) < 315) || ((misura33 * 100) > 345)){
      delay(50);
      Serial.print("3.3 volts is wrong : ");
      Serial.println(misura33);
      if (!alarmVolt) {
        sendAlarmMessage(Alarm_volts, Alarm_volt_text);
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
        Serial.println("notification sent");
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
        Serial.println("notification sent");
      } else {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                      POOL QUERY
// -------------------------------------------------------------------------------------------------------------------------------------------------------

void callPool(String poolchosen) {
  Serial.print("Choosen pool: ");
  Serial.println(poolchosen);
  HTTPClient http;
  if (poolchosen >= "1" && poolchosen <= "6") {
    if (poolchosen == "2") {
        String viporAddress = String(Mconfig.walletaddr);
        if (viporAddress.startsWith("nexa:")){
          viporAddress.remove(0, 5);
          Serial.print("modified string");
          Serial.println(viporAddress);
        }
        poolAddressAPI = "https://restapi.vipor.net/api/pools/" + String(Mconfig.cointicker) + "/miners/" + viporAddress ;
      }
    if (poolchosen == "3") {
        poolAddressAPI = "https://zergpool.com/api/walletEx?address=" + String(Mconfig.walletaddr) ;
      }
    if (poolchosen == "4") {
        poolAddressAPI = "https://api.woolypooly.com/api/" + String(Mconfig.cointicker) + "-1/accounts/" + String(Mconfig.walletaddr) ;
      }
    if (poolchosen == "5") {
        poolAddressAPI = "https://pool.rplant.xyz/api/walletEx/" + String(Mconfig.cointicker) + "/" + String(Mconfig.walletaddr) + "/" + String(Mconfig.minerpass) ;  // /api/wallet/koto/k16WgTvSLLvLDG64JZVocVJu4YvTuQNUj1s/pwd123
      }
    if (poolchosen == "6") {
        poolAddressAPI = "https://zsolo.bid/api/public/user/" + String(Mconfig.cointicker) + "?address=" + String(Mconfig.walletaddr) ;  // https://zsolo.bid/api/public/user/btc?address=1PS1wbFki6qH3AepVW6NRjaoTEdWnSZ3kD
      }
    if (poolchosen == "7") {
        poolAddressAPI = "https://" + String(Mconfig.cointicker) + ".2miners.com/api/accounts/" + String(Mconfig.walletaddr) ;  // https://nexa.2miners.com/api/accounts/nexa:walletaddress
      }
    http.begin(poolAddressAPI);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
          JsonDocument docPool;
          deserializeJson(docPool, payload);
          Serial.println("punto 2");
          if (poolchosen == "2") {
            balance = docPool["pendingBalance"];
            paidtotal = docPool["totalPaid"];
            hashrateTS = docPool["performance"]["workers"][""]["hashrate"];
          }
          if (poolchosen == "3") {
            balance = docPool["balance"];
            paidtotal = docPool["paidtotal"];
            hashrateTS=  docPool["miners"][0]["accepted"];
          }
          if (poolchosen == "4") {
            balance = docPool["immature_balance"];
            paidtotal = docPool["todayPaid"];
            hashrateTS = docPool["performance"]["pplns"][0]["hashrate"];
          }
          if (poolchosen == "5") {  // ok
            balance = docPool["balance"];
            paidtotal = docPool["total"];
            hashrateTS=  docPool["hashrate"];
          }
          if (poolchosen == "6") {  // ok
            balance = docPool["balance"];
            paidtotal = docPool["blocks"];
            hashrateTS=  docPool["hashrate"];
          }
          if (poolchosen == "7") {  // ok
            balance = docPool["payments"];
            paidtotal = docPool["paymentsTotal"];
            hashrateTS=  docPool["currentHashrate"];
          }
      } else {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  // ------------------------------------------ if the chosen pool is f2pool
  if (poolchosen == "8") {
    // richiesta hashrate
    http.begin("https://api.f2pool.com/v2/hash_rate/info");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("F2P-API-SECRET", "yourAPI");
    String payload = "{\"currency\": \"nexa\", \"mining_user_name\": \"minerkeeper\"}";
    int httpResponseCode = http.sendRequest("POST", payload);
    if (httpResponseCode > 0) {
      //{"info":{"name":"","hash_rate":0,"h1_hash_rate":0,"h24_hash_rate":323116.75259259256,"h1_stale_hash_rate":0,"h24_stale_hash_rate":0,"h24_delay_hash_rate":0,"local_hash_rate":0,"h24_local_hash_rate":0},"history":null,"currency":""}
      String f2pool_response = http.getString();
      JsonDocument docPool;
      deserializeJson(docPool, f2pool_response);
      hashrateTS = docPool["performance"];
    }
    http.end();
    delay(500);
    // richiesta balance
    http.begin("https://api.f2pool.com/v2/assets/balance");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("F2P-API-SECRET", "yourAPI");
    String payload2 = "{\"currency\": \"nexa\", \"user_name\": \"minerkeeper\"}";
    httpResponseCode = http.sendRequest("POST", payload2);
    if (httpResponseCode > 0) {
      //{"balance_info":{"balance":0,"paid":0,"total_income":0,"status":0,"yesterday_income":0,"estimated_today_income":0,"immature_balance":0}}
      String f2pool_response2 = http.getString();
      JsonDocument docPool;
      deserializeJson(docPool, f2pool_response2);
      balance = docPool["balance"];
      paidtotal = docPool["paid"];
    }
    http.end();
  }
}

// ----------------------------------------------------------------- END ------------------------------------------------------------

