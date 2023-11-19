#include <M5Core2.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AXP192.h>
#include "HX711.h"

AXP192 power;
HX711 weightScannerClient;

#define TINY_GSM_MODEM_SIM7080

#define SerialMon Serial
#define SerialAT Serial1

#define LOADCELL_DOUT_PIN 33 //NOTE: this use weight module
#define LOADCELL_SCK_PIN 32 //NOTE: this use weight module

#define GSM_AUTOBAUD_MIN 9600 //NOTE: this use sim module
#define GSM_AUTOBAUD_MAX 115200 //NOTE: this use sim module

#define WIFI_SSID "" //NOTE: please change SSID
#define WIFI_PASSWORD "" //NOTE: please change password

//NOTE: sim mode
// #include <TinyGsmClient.h>
// TinyGsm modem(SerialAT);
// TinyGsmClient client(modem);

const char* line_notify_host     = "notify-api.line.me";
const char* line_notify_token = "";

bool isWightAlert = false;
bool sendLineNotifyLock = true;
float weightData;

void initWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  M5.Lcd.print("Connecting to wifi");
  // loading
  int _cursorX = 0;
  M5.Lcd.setTextFont(4);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, 0);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    M5.Lcd.setCursor(0 + 5 * _cursorX, 30);
    M5.Lcd.print(".");
    delay(300);
    _cursorX++;
    if (_cursorX > 320) {
      _cursorX = 0;
    }
  }
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("connected with IP");
  M5.Lcd.println(WiFi.localIP());
}

//SIM Mode
// void initSim(){
//     delay(300);
//     M5.Lcd.println("modem.init()");

//     modem.restart();
//     M5.Lcd.println("modem.getModemInfo()");
//     String modemInfo = modem.getModemInfo();
//     M5.Lcd.println(modemInfo);

//     bool result;
//     do {
//         result = modem.setNetworkMode(2);//2 Automatic, 13 GSM only,  38 LTE only,  51 GSM and LTE only
//         delay(500);
//     } while (result != true);
//     M5.Lcd.println("modem.setNetworkMode(38)");
//     do {
//         result = modem.setPreferredMode(1);
//         delay(500);
//     } while (result != true);
//     M5.Lcd.println("modem.setPreferredMode(38)");

// //ここでsoracomのAPNを登録する
//     while (!modem.gprsConnect("soracom.io", "sora", "sora")) {
//         M5.Lcd.println("NG.");
//         delay(1000);
//         M5.Lcd.println("retry");
//         break;
//     }
//     M5.Lcd.println("modem.gprsConnect");
//     M5.Lcd.println("OK.");

//     M5.Lcd.println(" success");
//     bool res = modem.isGprsConnected();
//     M5.Lcd.println(res);

//     //各種情報を出力
//     String ccid = modem.getSimCCID();
//     M5.Lcd.println("CCID: " + ccid);
//     String imei = modem.getIMEI();
//     M5.Lcd.println("IMEI: " + imei);
//     String cop = modem.getOperator();
//     M5.Lcd.println("Operator: " + cop);
//     IPAddress local = modem.localIP();
//     M5.Lcd.println("Local IP: ");
//     M5.Lcd.println(local);
//     int csq = modem.getSignalQuality();
//     M5.Lcd.println("Signal quality: ");
//     M5.Lcd.println(csq);

//     if (modem.isNetworkConnected()) {
//         M5.Lcd.println("Network connected");
//     }

//     SerialMon.println("Waiting for network...");
//     if (!modem.waitForNetwork()) {
//         M5.Lcd.println("fail");
//         delay(1000);
//         return;
//     }

//     if (modem.isNetworkConnected()) {
//         M5.Lcd.println("Network connected");
//     }
// }

void setup() {
  M5.begin();
  weightScannerClient.begin(LOADCELL_DOUT_PIN,LOADCELL_SCK_PIN);
  weightScannerClient.set_gain();
  weightScannerClient.set_scale(27.61f); //NOTE: set scale
  weightScannerClient.tare(); //NOTE: Auto Set Offset

  // Serial.begin(115200);
  // initSim();
  initWifi();
}

void _sendLINENotification(String message) {
  //LINE
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();

 if (!wifiClient.connect(line_notify_host,443)) {
    M5.Lcd.println("Connection failed");
    return;
  }

  String query = String("message=") + message;
  String request = String("")
              + "POST /api/notify HTTP/1.1\r\n"
              + "Host: " + line_notify_host + "\r\n"
              + "Authorization: Bearer " + line_notify_token + "\r\n"
              + "Content-Length: " + String(query.length()) +  "\r\n"
              + "Content-Type: application/x-www-form-urlencoded\r\n\r\n"
              + query + "\r\n";
  wifiClient.print(request);
}

unsigned long previousMilles = 0;

void sendLineNotificationCronJob(String msg){
  const long interval= 30000; //NOTE: 30sec
  unsigned long currentMillis = millis();
  if(!sendLineNotifyLock){
    _sendLINENotification(msg);
    sendLineNotifyLock = true;
  }else{
    //NOTE: 最初の一回の通知移行は定期実行処理になる。
    if(currentMillis - previousMilles >= interval){
      _sendLINENotification(msg);
      previousMilles = currentMillis; //NOTE: reset
    }
  }
}

void loop() {
  M5.update();
  weightData = weightScannerClient.get_units(10) / 1000.0; //NOTE: 単位は`kg`

  if(weightData >= 2){ //NOTE: 2kg
    sendLineNotificationCronJob("乗りました。");
  }else{
    sendLineNotifyLock = false;
  }
}