// Set up an AP
// https://techtutorialsx.com/2017/04/25/esp8266-setting-an-access-point/
// Blink
// https://iot-playground.com/blog/2-uncategorised/38-esp8266-and-arduino-ide-blink-example
// 
/* e-paper display lib */
#include <GxEPD.h>
//Use the GxGDEW029T5 class if you have Badgy Rev 2C. Make sure you are on GxEPD 3.05 or above
#include <GxGDEW029T5/GxGDEW029T5.h>
//#include <GxGDEW029Z10/GxGDEW029Z10.h>    // 2.9" b/w/r displays
//#include <GxGDEH029A1/GxGDEH029A1.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <Esp.h>
/* include any other fonts you want to use https://github.com/adafruit/Adafruit-GFX-Library */
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
/* WiFi  libs*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
/* Util libs */
#include <TimeLib.h>
#include <ArduinoJson.h>
/* Menu libs */

/* QR lib */
#include "qr.h"

// Pins the buttons are connected to
const byte downPin = 0x1E;
const byte leftPin = 0x1D;
const byte rightPin = 0x17;
const byte upPin = 0x0F;
const byte okPin = 0x1B;

// Array of Key objects that will link GEM key identifiers with dedicated pins
Key keys[] = {{GEM_KEY_UP, upPin}, {GEM_KEY_RIGHT, rightPin}, {GEM_KEY_DOWN, downPin}, {GEM_KEY_LEFT, leftPin}, {GEM_KEY_OK, okPin}};

// Create KeyDetector object
KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key));
// To account for switch bounce effect of the buttons (if occur) you may want to specify debounceDelay
// as the second argument to KeyDetector constructor:
// KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key), 10);

// Constants for the pins SparkFun Graphic LCD Serial Backpack is connected to and SoftwareSerial object
const byte rxPin = 8;
const byte txPin = 9;
SoftwareSerial serialLCD(rxPin, txPin);


const int scrWidth = 296;
const int scrHeight = 128;

const int LED_PIN = 2;

String menu[] = {
  "Settings",
};
char status[192];

/* Player config */
String name = "James Wernicke";
String event = "Cyber Fire Foundry 15";
String org = "LANL A-4";
String title = "Entry Point Instructor";
String email = "wernicke@lanl.gov";
String host = "wernicke.com";

/* QR config */
int qrShowing = 0;

/* Scoreboard config */
String scoreboardPath = "/points.json";
const int scoreboardPort = 80;

/* sequence puzzle config */
int btnSeq[9] = {15,15,30,30,29,23,29,23,2}; // button sequence
int btnSeqIdx = 0; // button sequence index
const int btnSeqSz = sizeof(btnSeq)/sizeof(*btnSeq); // button sequence size
String checkSequence(int guess){
  byte expected = btnSeq[btnSeqIdx];
  if (guess == expected) btnSeqIdx++%btnSeqSz;
  else btnSeqIdx = 0;
  String output;
  sprintf(output, "Sequence %d/%d complete.\nExpected %x. Got %x.",btnSeqIdx,btnSeqSz,expected,guess);
  return output;
}

/* ap puzzle */
const char* notAHiddenSsid = "notAHiddenBadgePuzzle";
const char* notAHiddenPassword = "notAHiddenPassword";
const int notAHiddenChannel = 13;
const int ssid_visible = 0;
const int notAHiddenAPMaxConnections = 1;
const IPAddress notAHiddenIP = IPAddress(192, 168, 1, 1);
const IPAddress notAHiddenGateway = IPAddress(192, 168, 1, 1);
const IPAddress notAHiddenSubnet = IPAddress(255, 255, 255, 0);
const char* notAHiddenHostName = "notAHiddenHost";
const int notAHiddenWebServerPort = 80;
ESP8266WebServer notAHiddenWebServer(notAHiddenWebServerPort);
const String notAHiddenHeader = "text/html";
const String notAHiddenHTML = "O hai! The key is <i>iot playground</i>.";
void servePuzzle() {
  notAHiddenWebServer.send(200, notAHiddenHeader, notAHiddenHTML);
}

/* Time config */
const int timeZone = -8; // e.g. UTC-08:00 = -8

/* Always include the update server, or else you won't be able to do OTA updates! */
const int port = 8888;
ESP8266WebServer httpServer(port);
ESP8266HTTPUpdateServer httpUpdater;

/* Configure pins for display */
GxIO_Class io(SPI, SS, 0, 2);
GxEPD_Class display(io); // default selection of D4, D2

/* A single byte is used to store the button states for debouncing */
byte buttonState = 0;
byte lastButtonState = 0;   //the previous reading from the input pin
unsigned long lastDebounceTime = 0;  //the last time the output pin was toggled
unsigned long debounceDelay = 50;    //the debounce time

void setup(){  
  display.init();
  
  pinMode(1,INPUT_PULLUP); //down
  pinMode(3,INPUT_PULLUP); //left
  pinMode(5,INPUT_PULLUP); //center
  pinMode(12,INPUT_PULLUP); //right
  pinMode(10,INPUT_PULLUP); //up
  pinMode(LED_PIN, OUTPUT);
  
  /* Enter OTA mode if the center button is pressed */
  if(digitalRead(5)  == 0){
    /* WiFi Manager automatically connects using the saved credentials, if that fails it will go into AP mode */
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.autoConnect("Badgy AP");
    /* Once connected to WiFi, startup the OTA update server if the center button is held on boot */
    httpUpdater.setup(&httpServer);
    httpServer.begin();
    showIP();
    while(1){
      httpServer.handleClient();
    }
  }

  showDetails(); //show details on boot

  // ap puzzle setup
  WiFi.softAP(notAHiddenSsid, notAHiddenPassword, notAHiddenChannel, ssid_visible, notAHiddenAPMaxConnections);
  WiFi.softAPConfig(notAHiddenIP, notAHiddenGateway, notAHiddenSubnet);
  notAHiddenWebServer.on("/", servePuzzle);
  notAHiddenWebServer.begin();
}

void loop(){  
  httpServer.handleClient();

  byte reading =  (digitalRead(1)  == 0 ? 0 : (1<<0)) | //down
                  (digitalRead(3)  == 0 ? 0 : (1<<1)) | //left
                  (digitalRead(5)  == 0 ? 0 : (1<<2)) | //center
                  (digitalRead(12) == 0 ? 0 : (1<<3)) | //right
                  (digitalRead(10) == 0 ? 0 : (1<<4));  //up
                  
  if(reading != lastButtonState){
    lastDebounceTime = millis();
  }
  if((millis() - lastDebounceTime) > debounceDelay){
    if(reading != buttonState){
      buttonState = reading;
      for(int i=0; i<5; i++){
        if(bitRead(buttonState, i) == 0){
          if (i < 5){
          }
          switch(i){
            case 0: // down 0x1E
              qrShowing = 0;
              sprintf(status,"%s",checkSequence(buttonState));
              showText(status);
              break;
            case 1: // left 0x1D
              qrShowing = 0;
              sprintf(status,"%s",checkSequence(buttonState));
              showText(status);
              break;
            case 2: // center 0x1B
              if (qrShowing == 0){
                sprintf(status,"%s",checkSequence(buttonState));
                if (btnSeqIdx == btnSeqSz) showQR(rr, 25, 25);
                else showQR(vcard, 128, 128);
                qrShowing = 1;
              }
              else {
                showDetails();
                qrShowing = 0;
              }
              btnSeqIdx = 0;
              break;
            case 3: // right 0x17
              qrShowing = 0;
              sprintf(status,"%s",checkSequence(buttonState));
              showText(status);
              break;
            case 4: // up 0x0F
              qrShowing = 0;
              sprintf(status,"%s\nFlash ID: 0x%x",checkSequence(buttonState),spi_flash_get_id());
              showText(status);
              break;                              
            default:
              break;
          }
        }
      }
    }
  }
  lastButtonState = reading;

  notAHiddenWebServer.handleClient();
  if (WiFi.softAPgetStationNum() > 0) digitalWrite(2, HIGH);
  else digitalWrite(2, LOW);
}

void configModeCallback (WiFiManager *myWiFiManager){
  display.setRotation(3); //even = portrait, odd = landscape
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0,50);
  display.println("Connect to Badgy AP");
  display.println("to setup your WiFi!");
  display.update();  
}

void showIP(){
  display.setRotation(3); //even = portrait, odd = landscape
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0,10);

  String ip = WiFi.localIP().toString();
  String url = WiFi.localIP().toString() + ":"+String(port)+"/update";
  byte charArraySize = url.length() + 1;
  char urlCharArray[charArraySize];
  url.toCharArray(urlCharArray, charArraySize);

  display.println("You are now connected!");
  display.println("");  
  display.println("Go to:");
  display.println(urlCharArray);
  display.println("to upload a new sketch.");
  display.update();  
}

void showText(String text){
  display.setRotation(3); //even = portrait, odd = landscape
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0,20);
  display.println(text);
  display.update();
}

void showDetails() {
  display.setRotation(3); //even = portrait, odd = landscape
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans18pt7b);
  display.setCursor(0,30);
  display.println(name);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0,50);
  display.println(event);
  display.println(org);
  display.println(title);
  display.println(email);
  display.update();
}

void showQR(const uint8_t* qrCode, int qrWidth, int qrHeight) {
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap((scrWidth-qrWidth)/2, (scrHeight-qrHeight)/2, qrCode, qrWidth, qrHeight, 0);
  display.update();
}

bool getScore() {
  String url = scoreboardPath;

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = scoreboardPort;
  if (!client.connect(host, httpPort)) {
    showText("connection failed");
    return false;
  }
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      showText(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  while(client.available()){
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if(strcmp(status, "HTTP/1.1 200 OK") != 0){
      showText("HTTP Status Error!");
      return false;
    }

    /* Find the end of headers */
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      showText("Invalid Response...");
      return false;
    }

    /* Start parsing the JSON in the response body */
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(client);
    if(!root.success()){
      showText("JSON parsing failed!");
      return false;
    }
  }
  
  display.update();
  return true;
}
