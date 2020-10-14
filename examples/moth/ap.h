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

