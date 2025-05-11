// =================================================================================================
//                        ----------------------
//                             Miner Keeper
//                        ----------------------
// Description:
// Miner Keeper is an hardware watchdog designed to protect your miner from malfunctions or failures
// This project is developed by Blacktea of BitsFromItaly.it
// 
// Target Board: ESP32 Dev Module
// Compilation: Tested with ESP32 on Arduino IDE version 2.3.4//
//
// You can contribute to the development of the project by donating coins to the following addresses
//
// NEXA - nexa:nqtsq5g5k93k6xpljm03kwrzvqznpdltr506edyv27ylg7fj
// BTC - 1PS1wbFki6qH3AepVW6NRjaoTEdWnSZ3kD
// KAS - kaspa:qpgkwj8d7q2xmhrt4fd0l94gxgwygvtcy0vakec08975jjuqq5wwxzgx7w09z
// DOGE - DTa7UrCQ1WjZaAqRE65vyVkQLNMp5Hex6r 
// =================================================================================================

#include <ItemList.h>
#include <ItemSubMenu.h>
#include <ItemCommand.h>
#include <LcdMenu.h>                        // https://github.com/forntoh/LcdMenu/
#include <MenuScreen.h>
#include <SimpleRotary.h>
#include <display/LiquidCrystal_I2CAdapter.h>
#include <input/SimpleRotaryAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>
#include <widget/WidgetList.h>

#include "alarms_text.h"                    // Alarm texts to send
#include "icone.h"                          // Personalized symbols
#include "PoolsList.h"                      // List of pools
#include <OneWire.h>                        // One Wire connection management
#include <DallasTemperature.h>              // Temperature probe management
#include <PZEM004Tv30.h>                    // Voltmeter and ammeter management
#include <ESP32Servo.h>
#include <NTPClient.h>                      // Time management
#include <ESP_Mail_Client.h>                // email management

#include <WiFiManager.h>                    // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
#include <HTTPClient.h>                     // JSON pool management
#include <ArduinoJson.h>
#include "ThingSpeak.h"                     // Thingspeak connection management

// ---------------------------------------------------- Miner Keeper version

const char * versione = "0.2.28";                                    // software versione
String nomeMiner;                                                    // miner name

// ---------------------------------------------------- SMTP service and alarms
  
String mittente;
String destinatario;
String serverMail ;
int serverPort ;
String passwordServer;
String walletAddress = "";
char * poolName = "";
String cointype = "";
String mineremail = "";
String minerpass = "";
int ack = 0;
SMTPSession smtp;
Session_Config config;
void smtpCallback(SMTP_Status status);
String ssidEmail;
String passEmail;
#include "HeapStat.h"
HeapStat heapInfo;

boolean alarmTemp = false;
boolean alarmVolt = false;
boolean alarmFans = false;
boolean alarmAirvalve = false;
boolean alarmFire = false;

// ---------------------------------------------------- NTP service

WiFiUDP Udp;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600); // 3600 sono i secondi per GMT+1

// ---------------------------------------------------- Configuration Portal Variables

char nome[15] = "";
char tempAmb[5] = "";
char tempSet[5] = "";
char TSchannel[10] = "";
char TSapi[18] = "";
char Thingspeakstate[5] = "";
char mitt[30] = "";
char dest[30] = "";
char server[20] = "";
char mailport[5] = "";
char passServer[15];
char wallet[53];
char coin[7];
char emailminer[30];
char passminer[10];

WiFiManager wifiManager;
WiFiManagerParameter param_nome("name", "Miner name", "Miner 1", 10);
WiFiManagerParameter param_tempAmb("Tempamb", "Ambient temperature set", "20", 10);
WiFiManagerParameter param_tempSet("Temper", "Miner temperature set", "70", 10);
WiFiManagerParameter param_TSchannel("tschannel", "Thingspeak channel", "1234567", 10);
WiFiManagerParameter param_TSapi("tsapi", "Thingspeak API", "ABC1DEF2GHI3", 10);
WiFiManagerParameter param_Thingspeakstate("thingspeak", "Thingspeak on/off", "0", 10);
WiFiManagerParameter param_mitt("mitt", "Email sender", "sender@company.com", 10);
WiFiManagerParameter param_dest("dest", "Email recipient", "recipient@company.com", 10);
WiFiManagerParameter param_server("server", "Server smtp", "mail.smtp2go.com", 10);
WiFiManagerParameter param_mailport("mailport", "Server smtp port", "465", 10);
WiFiManagerParameter param_passServer("passServer", "Password email", "xxxxx", 10);
WiFiManagerParameter param_wallet("wallet", "Wallet address", "nexa:xxxxx", 10);
WiFiManagerParameter param_coin("coin", "coin name", "NEXA", 10);
WiFiManagerParameter param_emailminer("emailminer", "Email miner", "address@email.com", 10);
WiFiManagerParameter param_passminer("passminer", "Pass miner", "123", 10);

bool portalfirstrun = false;

// ----------------------------------------------------- servo motor
  Servo servoAir;
  int servoPin = 19;
  int posizione = 0;  // variable to know where the valve is positioned - start with air pushed out
  int maxAperturaServo = 0;
  int finecorsaPin = 27;  // Shutter closed button
  bool finecorsa = false;
  ESP32PWM pwm;

// --------------------------------------------------- ThingSpeak - Thingspeak settings

  boolean TSstate = false;
  long unsigned int myChannelNumber;
  char ch_TSapi[19];


// --------------------------------------------------- voltmeter ammeter + power supply measurements
  
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
  
// ----------------------------------------------------- wifi

unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)
WiFiClient  client;

// ------------------------------------------------- menu items
void readPool();
void voltaggiVoid();
void energiaVoid();
void temperatureVoid();
void rpmfansVoid();
void ipaddressVoid();
void airvalveVoid();
void resetAlarm();
void ConfigPortalVoid();
void AirValveCallback(const char* posAir);
std::vector<const char*> fanspeeds = {"AUTO", "25%", "50%", "75%", "100%"};
void PoolsCallback(const char* posPools);
std::vector<const char*> PoolsPos = {"none", "Vipor", "Zerg", "Wooly", "Rplant"};
void fanspeedsCallback(const char* posFans);
std::vector<const char*> airValvePos = {"AUTO", "IN", "OUT"};
void viewAlarms();
void infoScreen();

extern MenuScreen* settingsScreen;

MENU_SCREEN(mainScreen, mainItems,
    ITEM_COMMAND("Info screen", infoScreen),
    ITEM_COMMAND("Pool data", readPool),
    ITEM_COMMAND("Power supply", energiaVoid),
    ITEM_COMMAND("Board voltages", voltaggiVoid),
    ITEM_COMMAND("Miner temps", temperatureVoid),
    ITEM_COMMAND("Fans speed", rpmfansVoid),
    ITEM_COMMAND("Air valve", airvalveVoid),
    ITEM_SUBMENU("Settings", settingsScreen));

// Create submenu and precise its parent

MENU_SCREEN(settingsScreen, settingsItems,
    ITEM_COMMAND("Params portal", ConfigPortalVoid),
    ITEM_LIST("Pools", PoolsPos, [](const uint8_t posPools) {PoolsCallback(PoolsPos[posPools]);}),
    ITEM_LIST("Air Valve ", airValvePos, [](const uint8_t posAir) {AirValveCallback(airValvePos[posAir]);}),
    ITEM_LIST("Fans RPM ", fanspeeds, [](const uint8_t posFans) {fanspeedsCallback(fanspeeds[posFans]);}),
    ITEM_COMMAND("View local IP", ipaddressVoid),
    ITEM_COMMAND("View alarms", viewAlarms),
    ITEM_COMMAND("Reset alarms", resetAlarm));

// ----------------------------------------------------- lcd screen

#define LCD_ROWS 4
#define LCD_COLS 20

LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
CharacterDisplayRenderer renderer(new LiquidCrystal_I2CAdapter(&lcd), LCD_COLS, LCD_ROWS, 0x7E, 0x7F, NULL, NULL); // menu senza freccie laterali
LcdMenu menu(renderer);

int splash = 0;

// --------------------------------------------------- OneWire temperature probes
  
  #define ONE_WIRE_BUS 32                             // Temperature probe pins
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  DeviceAddress airIncoming, airMiner, airOutgoing;

  int ariaIN = 0;
  int ariaGPU = 0;
  int ariaOUT = 0;
  unsigned long ariaTimesent = 0;
  unsigned long ariaInterval = 60000;

// ---------------------------------------------------- ambient temperature sensor
  #include <Adafruit_Sensor.h>
  #include <DHT.h>
  #include <DHT_U.h>
  
  #define DHTPIN 25     // Digital pin connected to the DHT sensor
  // Uncomment the type of sensor in use:
  // #define DHTTYPE    DHT11     // DHT 11
  #define DHTTYPE    DHT22            // DHT 22 (AM2302)
  //#define DHTTYPE    DHT21     // DHT 21 (AM2301)
  DHT_Unified dht(DHTPIN, DHTTYPE);
  uint32_t delayMS;
  int temperatura ;              // temperatura misurata dal DHT22
  int umidita ;                  // umidita misurata dal DHT22
  int temperaturaSet ;           // temperatura a cui deve lavorare il miner
  int temperaturaAmbiente ;      // temperatura che voglio in casa

// --------------------------------------------------- flame sensor
  
  #define sensoreFIAMMA 33
  int valoreFIAMMA = HIGH ;
  boolean autoTemp = true;    // variable to manually control the valve

// --------------------------------------------------- fans

  const int pinFans = 23;
  int freqFan = 768;
  int pinChannel = 0;
  const int resolution = 10;
  boolean autoFan = true ;
  int fansSet = 0;
  const int pinTacho = 36;
  volatile int tachCount = 0;
  volatile unsigned long counterRPM = 0;
  long rpmFans = 0;
  

// --------------------------------------------------- rotative encoder

SimpleRotary encoder(4, 5, 18);
SimpleRotaryAdapter rotaryInput(&menu, &encoder);

// --------------------------------------------------- HTTP and JSON handling

const char* poolSelected = "0" ;

String poolAddressAPI ; // 

float balance ;
float paidtotal ;
const char* currency ;
long hashrateTS = 0 ;

// *****************************************************************************************************************************************************************************************
// SETUP
// *****************************************************************************************************************************************************************************************

void setup() {

  Serial.begin(115200);

  // I start the screen
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(3, 2);
  lcd.print(".. loading ..");

  // I initialize the icons
  lcd.createChar(1, CELSIUS);
  lcd.createChar(2, CASA);
  lcd.createChar(3, CAMINO);
  lcd.createChar(4, FAN);
  lcd.createChar(5, TEMPERATURE);
  lcd.createChar(6, VOLT);
  lcd.createChar(7, FIAMMA);

  // I start the fans
  ledcAttach(pinFans, freqFan, resolution);                          // fan speed
  ledcWrite(pinFans, freqFan);                                       // fan speed
  pinMode(pinTacho, INPUT_PULLUP);                                   // RPM count forse non serve

  // I start the flame detector
  pinMode(sensoreFIAMMA, INPUT);

  // I start the servomotor
  pinMode(finecorsaPin, INPUT);
  servoAir.setPeriodHertz(50);
  servoAir.attach(servoPin);
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("configuring");
  lcd.setCursor(3, 2);
  lcd.print("the air valve");
  for (int posServo = 0; posServo < 90; posServo ++)
  { // in steps of 1 degree
    servoAir.write(posServo);
    delay(50);
    if (digitalRead(finecorsaPin) == LOW) {
      finecorsa = true;
      maxAperturaServo = posServo;                                  // once the maximum opening is reached I assign the reached value to maxAperturaServo
      break ;
    }
  }

  lcd.clear();
  lcd.setCursor(3, 2);
  lcd.print(".. booting ..");

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
  if (!sensors.getAddress(airOutgoing, 2)) Serial.println("Unable to find address for Device 1");
  // first reading of temperature probes
  sensors.requestTemperatures();
  ariaIN = sensors.getTempC(airIncoming);
  ariaGPU = sensors.getTempC(airMiner);
  ariaOUT = sensors.getTempC(airOutgoing);

  // additional parameters configuration
  setupConfigParameters();
  delay(1000);
  wifiManager.setConfigPortalBlocking(false);
  // i start wifi
  WiFi.begin();                     //try to connect to saved network (blank credentials uses saved ones, put there by wifimanager)
  unsigned long beginTime=millis();
  while(WiFi.status()!=WL_CONNECTED){
    yield();                        //prevent WDT reset
    if(millis()-beginTime>6000){
      break;
    }
  }
  if(WiFi.status()==WL_CONNECTED){ 
    Serial.println("Succesfully connected to saved network, config portal off");
  }else{
    Serial.println("failed to connect, starting config portal");
    wifiManager.startConfigPortal("Miner Keeper");
    portalfirstrun = true;
  }
  
  //reset settings - wipe only WIFI credentials for testing
  //wifiManager.resetSettings();
  // reset EEPROM
  /*for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println("done");*/
  
  // presentazione
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("MINER KEEPER");
  lcd.setCursor(7, 1);
  lcd.print(versione);
  lcd.setCursor(2, 3);
  lcd.print("Bits from Italy");
  delay(2000);
  lcd.clear();
  
  // Check your wifi connection
  if(wifiManager.autoConnect("AutoConnectAP")){
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("WiFi connected");
    lcd.setCursor(2, 2);
    lcd.print("IP: ");
    lcd.setCursor(6, 2);
    lcd.print(WiFi.localIP());
    delay(2000);
    lcd.clear();
    ssidEmail = wifiManager.getWiFiSSID();
    passEmail = wifiManager.getWiFiPass();
    } else {
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("Configuration");
    lcd.setCursor(2, 2);
    lcd.print("portal is");
    lcd.setCursor(2, 3);
    lcd.print("running");
    delay(2000);
    lcd.clear();
  }

  // I start pools

  
  // I start Thingspeak
  ThingSpeak.begin(client);
  
  // I start NTP client
  timeClient.begin();
  timeClient.update();  // aggiorno l'ora
  Serial.println(timeClient.getFormattedTime());
  // I check if I used the configuration portal
  /*if(portalfirstrun){
    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("Restarting");
    delay(1000);
    ESP.restart();
  }*/
  
  // I start email service
  MailClient.networkReconnect(true);
  MailClient.clearAP();
  MailClient.addAP(ssidEmail, passEmail);
  smtp.debug(1);
  smtp.callback(smtpCallback);
  config.server.host_name = serverMail;
  config.server.port = (F("esp_mail_smtp_port_"), serverPort);
  config.login.email = mittente;
  config.login.password = passwordServer;
  config.login.user_domain = WiFi.localIP();
  // configuro il server NTP
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;
  // avvio il menu
  delay(1000);
  renderer.begin();
  menu.setScreen(mainScreen);
}

// *****************************************************************************************************************************************************************************************
// LOOP
// *****************************************************************************************************************************************************************************************

void loop() {

  // --------------------------------- I start the portal if requested ---------------------------------------------
  if(portalfirstrun){
    wifiManager.process();
  }

  

  // ---------------------------------------------------------- Stop Info screen  -----------------------------------------------------------------------

    if ((splash == 1) && (encoder.rotate() == 1)) {
    delay(5);
    splash = 0;
    lcd.clear();
    menu.show();
  }
  // --------------------- I read the temperatures of the probes, I measure the voltages and if necessary I send them to Thingspeak ---------------------
  unsigned long ariaTime = millis();

  if (splash == 0) {
    iconAirValve();
    renderer.updateTimer();
  }
  showAlarmIcons();
  
  if(ariaTime - ariaTimesent > ariaInterval) {
    wifiReconnect();
    ariaTimesent = ariaTime;

    if (splash == 1) {
      splashInfo();
    }

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
    letturaVoltaggi();
    letturaEnergiaVoid();
    countRPM();
    delay(2000);
    // inserisco la richiesta alla pool
    if (poolSelected != "0") {
      callPool("loop", poolSelected);
    }
    // -------------
    if (TSstate) {
      ThingSpeak.setField(1, ariaIN);
      ThingSpeak.setField(2, ariaGPU);
      ThingSpeak.setField(3, ariaOUT);
      ThingSpeak.setField(4, temperatura);
      ThingSpeak.setField(5, energy);
      ThingSpeak.setField(6, rpmFans);
      ThingSpeak.setField(7, hashrateTS);
      delay(50);
      int xTS = ThingSpeak.writeFields(myChannelNumber, ch_TSapi);
      if(xTS == 200){
        Serial.println("Thingspeak channel update successful.");
      }
      else{
        Serial.println("Problem updating Thingspeak channel. HTTP error code " + String(xTS));
      }
    }
  }
    // ------------------------------------------------I'm checking for a fire --------------------------
    
    valoreFIAMMA = digitalRead(sensoreFIAMMA);

    if (valoreFIAMMA == LOW) {                      // in case of flame
      sendEmail (Alarm_fires, Alarm_fire_text);     // I send email
      alarmFire = true;
      lcd.setCursor(19, 3);
      lcd.write(byte(7));
      if(posizione == 1){                           // I move the air valve to expel any fumes
        ariaCamino(); 
       }
      freqFan = 1023 ;                              // I raise fan at 100%
      fansSet = 4;
      ledcWrite(pinFans, freqFan);
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("Possible fire!");
      lcd.setCursor(1, 2);
      lcd.print("Miner shutting down");
      delay(20000);                                  // I await the evacuation of any fumes
      // shutdown();                                // voltage cut-off
    }
    if (autoTemp) {
    if(temperatura > (temperaturaAmbiente + 1) && posizione == 1){
        ariaCamino();
        Serial.print("ariaCamino");
        }
    if(temperatura < (temperaturaAmbiente - 1) && posizione == 0){
      ariaCasa();
      Serial.print("ariaCasa");
      }
    }
    // ---------------------------------------- I regulate the miner cooling -----------------------
    if (autoFan) {
      if (ariaGPU > (temperaturaSet + 10)) {
        alarmTemp = true ;
      }
      if ((ariaGPU > (temperaturaSet + 2)) && (freqFan < 1022)){
        freqFan = freqFan + 1;
        ledcWrite(pinFans, freqFan);
        }
      if ((ariaGPU < (temperaturaSet - 2)) && (freqFan > 10)){
        freqFan = freqFan - 1;
        ledcWrite(pinFans, freqFan);
      }
    }

    if(!portalfirstrun){
      rotaryInput.observe();
    }

}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                                    MENU FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

void splashInfo() {
  lcd.clear();
  readPoolSplash();
  lcd.setCursor(0, 0);
  lcd.print("Ambient");
  lcd.setCursor(9, 0);
  lcd.print(temperatura);
  lcd.setCursor(11, 0);
  lcd.write(byte(1));
  lcd.setCursor(12, 0);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Miner");
  lcd.setCursor(9, 1);
  lcd.print(ariaGPU);
  lcd.setCursor(11, 1);
  lcd.write(byte(1));
  lcd.setCursor(12, 1);
  lcd.print("C");

  lcd.setCursor(18, 1);
  if (posizione==0) {
    lcd.write(byte(3));
  } else {
    lcd.write(byte(2));
  }

  lcd.setCursor(0, 2);
  lcd.print("Energy");
  lcd.setCursor(9, 2);
  lcd.print(energy);
  lcd.setCursor(14, 2);
  lcd.print("KWh");
}

void infoScreen() {
  splash = 1;
  menu.hide();
  splashInfo();
}

void wifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin();
  }
}

void countRPM() {
  attachInterrupt(digitalPinToInterrupt(pinTacho), countTach, RISING);
  delay(500);
  detachInterrupt(digitalPinToInterrupt(pinTacho));
  rpmFans = (tachCount * 60)/10 ;
  tachCount = 0;
}

void countTach() { // fan RPM measure
  tachCount++;
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
  menu.hide();
  alarmTemp = false;
  alarmVolt = false;
  alarmFans = false;
  alarmAirvalve = false;
  alarmFire = false;
  lcd.setCursor(2, 1);
  lcd.print("Alarms cancelled");
  delay(3000);
  menu.show();
}

void showAlarmIcons() {
  if (alarmTemp) {
    lcd.setCursor(19, 0);
    lcd.write(byte(5));
  }
  if (alarmVolt) {
    lcd.setCursor(19, 1);
    lcd.write(byte(6));
  }
  if (alarmFans) {
    lcd.setCursor(19, 2);
    lcd.write(byte(4));
  }
}

void iconAirValve() {
  if(posizione == 1) {
    lcd.setCursor(18, 0);
    lcd.write(byte(2));
  }
  if(posizione == 0) {
    lcd.setCursor(18, 0);
    lcd.write(byte(3));
  }
}

void viewAlarms() {
  menu.hide();
  delay(50);
  lcd.clear();
  if ((alarmTemp) || (alarmVolt) || (alarmFans) || (alarmAirvalve) || (alarmFire)) {
    lcd.setCursor(2, 1);
    lcd.print("There are alarms");
    String tempAlarms ;
    if (alarmTemp) {
      tempAlarms = tempAlarms + Alarm_temps;
    }
    if (alarmVolt) {
      tempAlarms = tempAlarms + Alarm_volts;
    }
    if (alarmFans) {
      tempAlarms = tempAlarms + Alarm_fanss;
    }
    if (alarmAirvalve) {
      tempAlarms = tempAlarms + Alarm_airvalves;
    }
    if (alarmFire) {
      tempAlarms = tempAlarms + Alarm_fires;
    }
    for (int i=0; i < 20; i++) {
      tempAlarms = " " + tempAlarms;  
    } 
    tempAlarms = tempAlarms + " "; 
    for (int position = 0; position < tempAlarms.length(); position++) {
      lcd.setCursor(0, 2);
      lcd.print(tempAlarms.substring(position, position + 20));
      delay(400);
    }
  } else {
    lcd.setCursor(1, 1);
    lcd.print("There are no alarms");
  }
  delay(2000);
  lcd.clear();
  menu.show();
}

void letturaVoltaggi() {
  float misurazione12[10] = {};
  float somma12 = 0.00;
  float media12 = 0.00;
  for(int c12=0; c12<=9; c12++) {
    vin12 = analogRead(pin12volt);
    tempvin12 = (vin12 * 3.3) / 4095.0; //1,874432
    misura12 = tempvin12 / 0.156;
    misurazione12[c12] = misura12;
    somma12 = somma12 + misurazione12[c12];
    delay(1);
  }
  media12 = somma12 / 10;
  if (!alarmVolt) {
    int media12temp = media12*100;
    if ((media12temp < 1140) || (media12temp > 1260)){
      delay(50);
      alarmVolt = true;
      sendEmail (Alarm_volts, Alarm_volt_text);
    }
  }
  float misurazione5[10] = {};
  float somma5 = 0.00;
  float media5 = 0.00;
  for(int c5=0; c5<=9; c5++) {
    vin5 = analogRead(pin5volt);
    tempvin5 = (vin5 * 3.3) / 4095.0; //1,874432
    misura5 = tempvin5 / 0.194;
    misurazione5[c5] = misura5;
    somma5 = somma5 + misurazione5[c5];
    delay(1);
  }
  media5 = somma5 / 10;
  if (!alarmVolt) {
    int media5temp = media5 * 100;
    if ((media5temp < 475) || (media5temp > 550)){
      delay(50);
      alarmVolt = true;
      sendEmail (Alarm_volts, Alarm_volt_text);
    }
  }
  float misurazione33[10] = {};
  float somma33 = 0.00;
  float media33 = 0.00;
  for(int c33=0; c33<=9; c33++) {
    vin33 = analogRead(pin33volt);
    tempvin33 = (vin33 * 3.3) / 4095.0; //1,874432
    misura33 = tempvin33 / 0.28;
    misurazione33[c33] = misura33;
    somma33 = somma33 + misurazione33[c33];
    delay(1);
  }
  media33 = somma33 / 10;
  if (!alarmVolt) {
    int media33temp = media33*100;
    if ((media33temp < 315) || (media33temp > 345)){
      delay(50);
      alarmVolt = true;
      sendEmail (Alarm_volts, Alarm_volt_text);
    }
  }
}

void voltaggiVoid() {
  menu.hide();
  // 12 volt
  lcd.setCursor(0, 0);
  lcd.print("Voltage  12V: ");
  lcd.print(misura12);
  // 5 volt
  lcd.setCursor(0, 1);
  lcd.print("Voltage   5V: ");
  lcd.print(misura5);
  // 3.3 volt
  lcd.setCursor(0, 2);
  lcd.print("Voltage 3.3V: ");
  lcd.print(misura33);
  delay(3000);
  lcd.clear();
  menu.show();
}

void rpmfansVoid() {
  menu.hide();
  delay(50);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fans speed: ");
  lcd.print(rpmFans);
  lcd.print(" rpm");
  delay(3000);
  lcd.clear();
  menu.show();
}

void ipaddressVoid() {
  menu.hide();
  delay(50);
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("WiFi connected");
  lcd.setCursor(2, 2);
  lcd.print("IP: ");
  lcd.setCursor(6, 2);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print(WiFi.localIP());
  } else {
    lcd.print(WiFi.softAPIP());
  }
  delay(3000);
  lcd.clear();
  menu.show();
}
void letturaEnergiaVoid() {
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
}

void energiaVoid() {
  menu.hide();
  delay(50);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Voltage: ");
  lcd.print(voltage);
  lcd.print("V");
  lcd.setCursor(0, 1);
  lcd.print("Current: ");
  lcd.print(current);
  lcd.print("A");
  lcd.setCursor(0, 2);
  lcd.print("Power:   ");
  lcd.print(power);
  lcd.print("W");
  lcd.setCursor(0, 3);
  lcd.print("Energy:  ");
  lcd.print(energy);
  lcd.print("Kwh");
  //float frequency = pzem.frequency();
  //float pf = pzem.pf();
  delay(3000);
  lcd.clear();
  menu.show();
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

void airvalveVoid() {
  menu.hide();
  delay(50);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Air valve is set");
  lcd.setCursor(4, 1);
  lcd.print("to push air");
  if (posizione == 0) {
    lcd.setCursor(6, 2);
    lcd.print("OUTSIDE");
  } else {
    lcd.setCursor(6, 2);
    lcd.print("INSIDE");
  }
  delay(3000);
  lcd.clear();
  menu.show();
}

void temperatureVoid() {
  menu.hide();
  delay(50);
  sensors.requestTemperatures();
  ariaIN = sensors.getTempC(airIncoming);
  ariaGPU = sensors.getTempC(airMiner);
  ariaOUT = sensors.getTempC(airOutgoing);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp IN:    ");
  lcd.print(ariaIN);
  lcd.write(byte(1));
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Temp MINER: ");
  lcd.print(ariaGPU);
  lcd.write(byte(1));
  lcd.print("C");

  lcd.setCursor(0, 2);
  lcd.print("Temp OUT:   ");
  lcd.print(ariaOUT);
  lcd.write(byte(1));
  lcd.print("C");

  lcd.setCursor(0, 3);
  lcd.print("Ambient:    ");
  lcd.print(temperatura);
  lcd.write(byte(1));
  lcd.print("C");

  delay(3000);
  lcd.clear();
  menu.show();
}

void AirValveCallback(const char* posAirV) {
    Serial.print("valvola aria: ");
    Serial.println(posAirV);
    if (posAirV == "AUTO"){
      autoTemp = true ;
    }
    if (posAirV == "IN"){
      autoTemp = false ;
      ariaCasa();
    } else if (posAirV == "OUT"){
      autoTemp = false ;
      ariaCamino();
    }
}



void fanspeedsCallback(const char* posFans) {
    if (posFans == "AUTO"){
      autoFan = true ;
      fansSet = 0;
    }
    if (posFans == "25%"){
      autoFan = false ;
      freqFan = 256 ;
      fansSet = 1;
      ledcWrite(pinFans, freqFan);
    } else if (posFans == "50%"){
      autoFan = false ;
      freqFan = 512 ;
      fansSet = 2;
      ledcWrite(pinFans, freqFan);
    } else if (posFans == "75%"){
      autoFan = false ;
      freqFan = 768 ;
      fansSet = 3;
      ledcWrite(pinFans, freqFan);
    } else if (posFans == "100%"){
      autoFan = false ;
      freqFan = 1023 ;
      fansSet = 4;
      ledcWrite(pinFans, freqFan);
    }
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------      EMAIL       -------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

void sendEmail (String subjectTosend, String messageTosend ) {
  menu.hide();
  SMTP_Message message ;
  message.sender.name = nomeMiner;
  message.sender.email = "andrea@friuliamo.it";
  message.subject = subjectTosend.c_str();
  message.addRecipient(F("user"), destinatario);
  message.text.content = messageTosend.c_str();

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("There is an alert");
  lcd.setCursor(2, 1);
  lcd.print(subjectTosend);
  lcd.setCursor(3, 3);
  lcd.print("sending email");

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
  lcd.clear();
  menu.show();
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

// ----------------------------------------------------------------------------------------------------------------------------------------------
//                                                        CONFIGURATION PORTAL
// ----------------------------------------------------------------------------------------------------------------------------------------------

void ConfigPortalVoid () {
  menu.hide();
  Serial.println("config avviata");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Configuration portal");
  lcd.setCursor(3, 1);
  lcd.print("is active at");
  lcd.setCursor(3, 2);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print(WiFi.localIP());
  } else {
    lcd.print(WiFi.softAPIP());
  }
  lcd.setCursor(0, 3);
  lcd.print("Press button to exit");
  delay(500);
  wifiManager.startWebPortal();
  do {
  Serial.println("portal running");
  wifiManager.process();
  } while (encoder.push() == 0);
  wifiManager.stopWebPortal();
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Restarting");
  delay(1000);
  ESP.restart();
  }



// -----------------------------------------------------------------------------------------------------------------------------------------------
//                                              Params on EEPROM
// -----------------------------------------------------------------------------------------------------------------------------------------------


void saveConfigParams(){

  //first, get values input from web portal and put them in char arrays
  String tempVal = param_nome.getValue();
  char ch_nome[16];
  tempVal.toCharArray(ch_nome,16);

  tempVal = param_tempAmb.getValue();
  char ch_tempAmb[6];
  tempVal.toCharArray(ch_tempAmb,6);

  tempVal = param_tempSet.getValue();
  char ch_tempSet[6];
  tempVal.toCharArray(ch_tempSet,6);

  tempVal = param_TSchannel.getValue();
  char ch_TSchannel[11];
  tempVal.toCharArray(ch_TSchannel,11);

  tempVal = param_TSapi.getValue();
  char ch_TSapi[19];
  tempVal.toCharArray(ch_TSapi,19);

  tempVal = param_Thingspeakstate.getValue();
  char ch_Thingspeakstate[6];
  tempVal.toCharArray(ch_Thingspeakstate,5);

  tempVal = param_mitt.getValue();
  char ch_mitt[31];
  tempVal.toCharArray(ch_mitt,31);

  tempVal = param_dest.getValue();
  char ch_dest[31];
  tempVal.toCharArray(ch_dest,31);

  tempVal = param_server.getValue();
  char ch_server[21];
  tempVal.toCharArray(ch_server,21);

  tempVal = param_mailport.getValue();
  char ch_mailport[6];
  tempVal.toCharArray(ch_mailport,6);

  tempVal = param_passServer.getValue();
  char ch_passServer[16];
  tempVal.toCharArray(ch_passServer,16);

  tempVal = param_wallet.getValue();
  char ch_wallet[61];
  tempVal.toCharArray(ch_wallet,61);

  tempVal = param_coin.getValue();
  char ch_coin[7];
  tempVal.toCharArray(ch_coin,7);

  tempVal = param_emailminer.getValue();
  char ch_emailminer[31];
  tempVal.toCharArray(ch_emailminer,31);

  tempVal = param_passminer.getValue();
  char ch_passminer[11];
  tempVal.toCharArray(ch_passminer,11);

  //now we save the values we got above to eeprom
  EEPROM.begin(300);
  int address = 0;
  
  EEPROM.put(address,ch_nome);
  address+=16;
  EEPROM.put(address,ch_tempAmb);
  address+=6;
  EEPROM.put(address,ch_tempSet);
  address+=6;
  EEPROM.put(address,ch_TSchannel);
  address+=11;
  EEPROM.put(address,ch_TSapi);
  address+=19;
  EEPROM.put(address,ch_Thingspeakstate);
  address+=6;
  EEPROM.put(address,ch_mitt);
  address+=31;
  EEPROM.put(address,ch_dest);
  address+=31;
  EEPROM.put(address,ch_server);
  address+=21;
  EEPROM.put(address,ch_mailport);
  address+=6;
  EEPROM.put(address,ch_passServer);
  address+=16;
  EEPROM.put(address,ch_wallet);
  address+=61;
  EEPROM.put(address,ch_coin);
  address+=5;
  EEPROM.put(address,ch_emailminer);
  address+=31;
  EEPROM.put(address,ch_passminer);
  address+=11;

  EEPROM.commit();
  EEPROM.end();
}

void setupConfigParameters(){

  wifiManager.setSaveParamsCallback(saveConfigParams); //when user presses save in captive portal, it'll fire this function so we can get the values
  // * setPreSaveConfigCallback, set a callback to fire before saving wifi or params [this could be useful if other function reboots esp before eeprom can save]

  //first we read in values from EEPROM to know what to display in the browser. We also use this to set global parameters
  EEPROM.begin(250);
  int address=0;

  char ch_nome[16];
  EEPROM.get(address,ch_nome);
  address+=16;
  nomeMiner = String((char *)ch_nome);

  char ch_tempAmb[6];
  EEPROM.get(address,ch_tempAmb);
  address+=6;
  temperaturaAmbiente = atoi(ch_tempAmb);

  char ch_tempSet[6];
  EEPROM.get(address,ch_tempSet);
  address+=6;
  temperaturaSet = atoi(ch_tempSet);

  char ch_TSchannel[11];
  EEPROM.get(address,ch_TSchannel);
  address+=11;
  myChannelNumber = atoi(ch_TSchannel);


  EEPROM.get(address,ch_TSapi);
  address+=19;

  char ch_Thingspeakstate[6];
  EEPROM.get(address,ch_Thingspeakstate);
  address+=6;
  if (atoi(ch_Thingspeakstate) == 1) {
    TSstate = true;
  } else {
    TSstate = false;
  }

  char ch_mitt[31];
  EEPROM.get(address,ch_mitt);
  address+=31;
  mittente = String((char *)ch_mitt);

  char ch_dest[31];
  EEPROM.get(address,ch_dest);
  address+=31;
  destinatario = String((char *)ch_dest);

  char ch_server[21];
  EEPROM.get(address,ch_server);
  address+=21;
  serverMail = String((char *)ch_server);

  char ch_mailport[6];
  EEPROM.get(address,ch_mailport);
  address+=6;
  serverPort = atoi(ch_mailport);
  
  char ch_passServer[16];
  EEPROM.get(address,ch_passServer);
  address+=16;
  passwordServer = String((char *)ch_passServer);

  char ch_wallet[61];
  EEPROM.get(address,ch_wallet);
  address+=61;
  walletAddress = String((char *)ch_wallet); // da qui in poi

  char ch_coin[7];
  EEPROM.get(address,ch_coin);
  address+=7;
  cointype = String((char *)ch_coin);

  char ch_emailminer[31];
  EEPROM.get(address,ch_emailminer);
  address+=31;
  mineremail = String((char *)ch_emailminer);

  char ch_passminer[11];
  EEPROM.get(address,ch_passminer);
  address+=11;
  minerpass = String((char *)ch_passminer);

  EEPROM.end();
  //now update the values in WiFiManager parameters to reflect values from EEPROM
  param_nome.setValue(ch_nome,15);
  param_tempAmb.setValue(ch_tempAmb,5);
  param_tempSet.setValue(ch_tempSet,5);
  param_TSchannel.setValue(ch_TSchannel,10);
  param_TSapi.setValue(ch_TSapi,18);
  param_Thingspeakstate.setValue(ch_Thingspeakstate,5);
  param_mitt.setValue(ch_mitt,30);
  param_dest.setValue(ch_dest,30);
  param_server.setValue(ch_server,20);
  param_mailport.setValue(ch_mailport,5);
  param_passServer.setValue(ch_passServer,15);
  param_wallet.setValue(ch_wallet,60);
  param_coin.setValue(ch_coin,6);
  param_emailminer.setValue(ch_emailminer,30);
  param_passminer.setValue(ch_passminer,10);
  
  //after we've read from eeprom and updated the values, now we add the parameters (before config portal is launched)
  wifiManager.addParameter(&param_nome);
  wifiManager.addParameter(&param_wallet);
  wifiManager.addParameter(&param_tempAmb);
  wifiManager.addParameter(&param_tempSet);
  wifiManager.addParameter(&param_TSchannel);
  wifiManager.addParameter(&param_TSapi);
  wifiManager.addParameter(&param_Thingspeakstate);
  wifiManager.addParameter(&param_mitt);
  wifiManager.addParameter(&param_dest);
  wifiManager.addParameter(&param_server);
  wifiManager.addParameter(&param_mailport);
  wifiManager.addParameter(&param_passServer);
  wifiManager.addParameter(&param_coin);
  wifiManager.addParameter(&param_emailminer);
  wifiManager.addParameter(&param_passminer);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                      POOL QUERY
// -------------------------------------------------------------------------------------------------------------------------------------------------------

void PoolsCallback(const char* posPools) {
    if (posPools == "none"){ //(PoolsPos[posPools])
      poolSelected = "0";
    }
    if (posPools == "Vipor"){
      poolSelected = "1";
      poolName = "Vipor";
    }
    if (posPools == "Zerg"){
      poolSelected = "2";
      poolName = "Zerg";
    }
    if (posPools == "Wooly"){
      poolSelected = "3";
      poolName = "Wooly";
    }
    if (posPools == "Rplant"){
      poolSelected = "4";
      poolName = "Rplant";
    }
}

void readPool() {
  if (poolSelected != "0") {
      callPool("menuMode", poolSelected);
  } else {
    menu.hide();
    lcd.setCursor(2, 1);
    lcd.print("no pool selected");
    lcd.setCursor(4, 2);
    lcd.print("use SETTINGS");
    delay(3000);
    menu.show();
  }
}

void readPoolSplash() {
  if (walletAddress != ""){
    if (poolSelected != "0") {
      callPool("menuSplash", poolSelected);
    } else {
      menu.hide();
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("no pool selected");
      lcd.setCursor(4, 2);
      lcd.print("use SETTINGS");
      delay(3000);
      lcd.clear();
      menu.show();
    }
  } else {
    menu.hide();
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("set wallet address");
      lcd.setCursor(4, 2);
      lcd.print("use SETTINGS");
      delay(3000);
      lcd.clear();
      menu.show();
      return;
  }
}

void callPool(String reason, const char* poolchosen) {
  Serial.print("pool scelta: ");
  Serial.println(poolchosen);
  Serial.print("reason: ");
  Serial.println(reason);
  if (reason != "loop") {
    menu.hide();
    delay(50);
    if (reason == "menuMode") {
      lcd.setCursor(1, 1);
      lcd.print("querying the pool");
    }
  }
  Serial.println("punto 1");
  HTTPClient http;
  if (poolchosen == "1") {
      poolAddressAPI = "https://restapi.vipor.net/api/pools/nexa/miners/" + walletAddress ;
    }
  if (poolchosen == "2") {
      poolAddressAPI = "https://zergpool.com/api/walletEx?address=" + walletAddress ;
    }
  if (poolchosen == "3") {
      poolAddressAPI = "https://api.woolypooly.com/api/" + cointype + "-1/accounts/" + walletAddress ; // nexa-1/accounts/
    }
  if (poolchosen == "4") {
      poolAddressAPI = "https://pool.rplant.xyz/api/walletEx/" + cointype + "/" + walletAddress + "/" + minerpass ;  ///api/wallet/koto/k16WgTvSLLvLDG64JZVocVJu4YvTuQNUj1s/pwd123
    }
  http.begin(poolAddressAPI);
  Serial.print("api query: ");
  Serial.println(poolAddressAPI);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
        JsonDocument docPool;
        deserializeJson(docPool, payload);
        Serial.println("punto 2");
        if (poolchosen == "1") {
          balance = docPool["pendingBalance"];
          paidtotal = docPool["totalPaid"];
          //currency = docPool["lastBlock"]["poolId"];
          hashrateTS = docPool["performance"]["workers"][0]["hashrate"];
        }
        if (poolchosen == "2") {
          balance = docPool["balance"];
          paidtotal = docPool["paidtotal"];
          //currency = docPool["currency"];
          hashrateTS=  docPool["miners"][0]["accepted"];
        }
        if (poolchosen == "3") {
          balance = docPool["immature_balance"];
          paidtotal = docPool["todayPaid"];
          //currency = cointype ;
          hashrateTS = docPool["performance"]["pplns"][0]["hashrate"];
        }
        if (poolchosen == "4") {  // ok
          balance = docPool["balance"];
          paidtotal = docPool["total"];
          //currency = docPool["coin"];
          hashrateTS=  docPool["hashrate"];
        }
        Serial.println("punto 3");
        if (reason == "menuMode") {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(cointype);
          lcd.print(" stats at ");
          lcd.print(poolName);
          lcd.setCursor(0, 1);
          lcd.print("Hashrate:");
          lcd.print(hashrateTS);
          lcd.print("h/s");
          lcd.setCursor(0, 2);
          lcd.print("Bal: ");
          lcd.print(balance);
          lcd.setCursor(0, 3);
          lcd.print("Paid: ");
          lcd.print(paidtotal);
          delay(5000);
          lcd.clear();
          menu.show();
        }
        Serial.println("punto 4");
        if (reason == "menuSplash") {
        lcd.clear();
        lcd.setCursor(0, 3);
        lcd.print("Hashrate");
        lcd.setCursor(9, 3);
        lcd.print(hashrateTS);
        lcd.print("h/s");
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

// ----------------------------------------------------------------- END ------------------------------------------------------------