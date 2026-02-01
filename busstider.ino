#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define RESET_PIN 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Format types: 0=simple, 1=list, 2=verbose
#define FORMAT_SIMPLE  0
#define FORMAT_LIST    1
#define FORMAT_VERBOSE 2

struct Config {
  char ssid[32];
  char password[64];
  char endpoint[128];
  uint8_t format;
  bool configured;
  char checksum;
};

Config config;
ESP8266WebServer server(80);
DNSServer dnsServer;

const char* AP_NAME = "BussTider-Setup";
const byte DNS_PORT = 53;
bool setupMode = false;

// ===== PAGINATION =====
int currentPage = 0;
int totalPages = 1;
unsigned long lastPageSwitch = 0;
const unsigned long PAGE_INTERVAL = 5000;  // 5 seconds per page

// ===== DATA REFRESH =====
unsigned long lastDataFetch = 0;
const unsigned long DATA_INTERVAL = 20000;  // 20 seconds between API calls
String cachedPayload = "";
bool hasData = false;

// ===== EEPROM =====

void loadConfig() {
  EEPROM.begin(sizeof(Config) + 10);
  EEPROM.get(0, config);
  char expectedChecksum = config.ssid[0] ^ config.password[0] ^ config.endpoint[0] ^ config.format;
  if (config.checksum != expectedChecksum || !config.configured) {
    memset(&config, 0, sizeof(Config));
    config.configured = false;
    config.format = FORMAT_SIMPLE;
  }
  EEPROM.end();
}

void saveConfig() {
  config.configured = true;
  config.checksum = config.ssid[0] ^ config.password[0] ^ config.endpoint[0] ^ config.format;
  EEPROM.begin(sizeof(Config) + 10);
  EEPROM.put(0, config);
  EEPROM.commit();
  EEPROM.end();
}

void clearConfig() {
  memset(&config, 0, sizeof(Config));
  EEPROM.begin(sizeof(Config) + 10);
  EEPROM.put(0, config);
  EEPROM.commit();
  EEPROM.end();
}

// ===== DISPLAY =====

// Replace Norwegian characters (UTF-8) with ASCII equivalents
void sanitizeNorwegian(char* dest, const char* src, size_t maxLen) {
  size_t j = 0;
  for (size_t i = 0; src[i] != '\0' && j < maxLen - 1; i++) {
    if ((uint8_t)src[i] == 0xC3 && src[i+1] != '\0') {
      uint8_t next = (uint8_t)src[i+1];
      if (next == 0xA6) { // æ
        if (j + 2 < maxLen) { dest[j++] = 'a'; dest[j++] = 'e'; }
        i++;
      } else if (next == 0xB8) { // ø
        if (j + 2 < maxLen) { dest[j++] = 'o'; dest[j++] = 'e'; }
        i++;
      } else if (next == 0xA5) { // å
        if (j + 2 < maxLen) { dest[j++] = 'a'; dest[j++] = 'a'; }
        i++;
      } else if (next == 0x86) { // Æ
        if (j + 2 < maxLen) { dest[j++] = 'A'; dest[j++] = 'e'; }
        i++;
      } else if (next == 0x98) { // Ø
        if (j + 2 < maxLen) { dest[j++] = 'O'; dest[j++] = 'e'; }
        i++;
      } else if (next == 0x85) { // Å
        if (j + 2 < maxLen) { dest[j++] = 'A'; dest[j++] = 'a'; }
        i++;
      } else {
        dest[j++] = src[i];
      }
    } else {
      dest[j++] = src[i];
    }
  }
  dest[j] = '\0';
}

void showMessage(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  if (strlen(line2) > 0) { display.setCursor(0, 16); display.println(line2); }
  if (strlen(line3) > 0) { display.setCursor(0, 32); display.println(line3); }
  display.display();
}

void showSetupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== SETUP MODE ===");
  display.setCursor(0, 16);
  display.println("1. Koble til WiFi:");
  display.setCursor(0, 26);
  display.print("   ");
  display.println(AP_NAME);
  display.setCursor(0, 40);
  display.println("2. Apne nettleser");
  display.setCursor(0, 52);
  display.println("   192.168.4.1");
  display.display();
}

// ===== WEB SERVER =====

void handleRoot() {                                                                                                                                                    
    String html = "<!DOCTYPE html><html><head>";                                                                                                                         
    html += "<meta charset=\"UTF-8\">";                                                                                                                                  
    html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";                                                                                   
    html += "<title>BussTider Setup</title>";                                                                                                                            
    html += "<style>";                                                                                                                                                   
    html += "*{box-sizing:border-box;font-family:system-ui,-apple-system,sans-serif;margin:0}";                                                                          
    html += "body{background:#f9fafb;color:#111827;min-height:100vh;display:flex;flex-direction:column}";                                                                
    html += "header{background:#fff;border-bottom:1px solid #e5e7eb;padding:24px 16px}";                                                                                 
    html += "header h1{font-size:20px;font-weight:600;color:#111827;max-width:400px;margin:0 auto}";                                                                     
    html += "header p{font-size:14px;color:#6b7280;max-width:400px;margin:4px auto 0}";                                                                                  
    html += "main{flex:1;padding:32px 16px}";                                                                                                                            
    html += ".c{max-width:400px;margin:0 auto}";                                                                                                                         
    html += "label{display:block;font-size:14px;font-weight:500;color:#374151;margin-bottom:8px}";                                                                       
    html += "input,select{width:100%;padding:12px 16px;border:1px solid #d1d5db;border-radius:8px;background:#fff;color:#111827;font-size:16px;margin-bottom:16px;outline:none}";               
    html += "input:focus,select:focus{border-color:#2563eb;box-shadow:0 0 0 1px #2563eb}";                                                                               
    html += "input::placeholder{color:#9ca3af}";                                                                                                                         
    html += "button{width:100%;padding:14px;background:#2563eb;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:500;cursor:pointer}";                                                                                                                                                               
    html += "button:hover{background:#1d4ed8}";                                                                                                                          
    html += "button:disabled{opacity:0.5;cursor:not-allowed}";                                                                                                           
    html += ".nets{max-height:180px;overflow-y:auto;margin-bottom:16px;border:1px solid #e5e7eb;border-radius:8px;background:#fff}";                                     
    html += ".net{padding:12px 16px;cursor:pointer;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #e5e7eb}";                      
    html += ".net:last-child{border-bottom:none}";                                                                                                                       
    html += ".net:hover{background:#f9fafb}";                                                                                                                            
    html += ".net.sel{background:#eff6ff}";                                                                                                                              
    html += ".net span:first-child{font-weight:500;color:#111827}";                                                                                                      
    html += ".sig{font-size:12px;color:#6b7280;background:#f3f4f6;padding:2px 8px;border-radius:9999px}";                                                                
    html += ".scanning{padding:16px;text-align:center;color:#6b7280;font-size:14px}";                                                                                    
    html += ".section{margin-bottom:24px}";                                                                                                                              
    html += ".divider{display:flex;align-items:center;gap:16px;margin:24px 0}";                                                                                          
    html += ".divider span{font-size:14px;color:#6b7280}";                                                                                                               
    html += ".divider::before,.divider::after{content:'';flex:1;height:1px;background:#e5e7eb}";                                                                         
    html += "footer{background:#fff;border-top:1px solid #e5e7eb;padding:16px;text-align:center}";                                                                       
    html += "footer p{font-size:14px;color:#6b7280;max-width:400px;margin:0 auto}";                                                                                      
    html += "footer a{color:#6b7280;text-decoration:underline}";                                                                                                         
    html += "footer a:hover{color:#374151}";                                                                                                                             
    html += "</style></head><body>";                                                                                                                                     
                                                                                                                                                                         
    // Header                                                                                                                                                            
    html += "<header>";                                                                                                                                                  
    html += "<h1>BussTider Setup</h1>";                                                                                                                                  
    html += "<p>Koble displayet til WiFi og konfigurer API</p>";                                                                                                         
    html += "</header>";                                                                                                                                                 
                                                                                                                                                                         
    // Main content                                                                                                                                                      
    html += "<main><div class=\"c\">";                                                                                                                                   
                                                                                                                                                                         
    // WiFi section                                                                                                                                                      
    html += "<div class=\"section\">";                                                                                                                                   
    html += "<label>Velg WiFi-nettverk</label>";                                                                                                                         
    html += "<div class=\"nets\" id=\"n\"><div class=\"scanning\">Skanner etter nettverk...</div></div>";                                                                
    html += "</div>";                                                                                                                                                    
                                                                                                                                                                         
    // Password                                                                                                                                                          
    html += "<div class=\"section\">";                                                                                                                                   
    html += "<label>WiFi-passord</label>";                                                                                                                               
    html += "<input type=\"password\" id=\"p\" placeholder=\"Skriv inn passord\">";                                                                                      
    html += "</div>";                                                                                                                                                    
                                                                                                                                                                         
    html += "<div class=\"divider\"><span>API-innstillinger</span></div>";                                                                                               
                                                                                                                                                                         
    // Endpoint                                                                                                                                                          
    html += "<div class=\"section\">";                                                                                                                                   
    html += "<label>API Endpoint</label>";                                                                                                                               
    html += "<input type=\"text\" id=\"e\" placeholder=\"https://departures.filipjohn.workers.dev/d/...\">";                                                             
    html += "</div>";                                                                                                                                                    
                                                                                                                                                                         
    // Format                                                                                                                                                            
    html += "<div class=\"section\">";                                                                                                                                   
    html += "<label>Responsformat</label>";                                                                                                                              
    html += "<select id=\"f\">";                                                                                                                                         
    html += "<option value=\"0\">Simple — {\"31\": 5, \"74\": 2}</option>";                                                                                              
    html += "<option value=\"1\">List — [{\"line\":\"31\",\"minutes\":5}]</option>";                                                                                     
    html += "<option value=\"2\">Verbose — inkluderer destinasjon</option>";                                                                                             
    html += "</select>";                                                                                                                                                 
    html += "</div>";                                                                                                                                                    
                                                                                                                                                                         
    // Submit button                                                                                                                                                     
    html += "<button onclick=\"save()\">Lagre og koble til</button>";                                                                                                    
                                                                                                                                                                         
    html += "</div></main>";                                                                                                                                             
                                                                                                                                                                         
    // Footer                                                                                                                                                            
    html += "<footer><p>Powered by <a href=\"https://entur.no\">Entur</a></p></footer>";                                                                                 
                                                                                                                                                                         
    // JavaScript                                                                                                                                                        
    html += "<script>";                                                                                                                                                  
    html += "var sel='';";                                                                                                                                               
    html += "fetch('/scan').then(function(r){return r.json()}).then(function(d){";                                                                                       
    html += "var h='';";                                                                                                                                                 
    html += "if(d.length===0){h='<div class=\"scanning\">Ingen nettverk funnet</div>';}";                                                                                
    html += "else{for(var i=0;i<d.length;i++){";                                                                                                                         
    html += "h+='<div class=\"net\" onclick=\"pick(this)\" data-s=\"'+d[i].ssid+'\">';";                                                                                 
    html += "h+='<span>'+d[i].ssid+'</span>';";                                                                                                                          
    html += "h+='<span class=\"sig\">'+d[i].rssi+' dBm</span></div>';";                                                                                                  
    html += "}}";                                                                                                                                                        
    html += "document.getElementById('n').innerHTML=h;";                                                                                                                 
    html += "}).catch(function(){";                                                                                                                                      
    html += "document.getElementById('n').innerHTML='<div class=\"scanning\">Kunne ikke skanne</div>';";                                                                 
    html += "});";                                                                                                                                                       
    html += "function pick(el){";                                                                                                                                        
    html += "var all=document.querySelectorAll('.net');";                                                                                                                
    html += "for(var i=0;i<all.length;i++)all[i].className='net';";                                                                                                      
    html += "el.className='net sel';";                                                                                                                                   
    html += "sel=el.getAttribute('data-s');";                                                                                                                            
    html += "}";                                                                                                                                                         
    html += "function save(){";                                                                                                                                          
    html += "if(!sel){alert('Velg et WiFi-nettverk');return;}";                                                                                                          
    html += "var e=document.getElementById('e').value;";                                                                                                                 
    html += "if(!e){alert('Skriv inn API endpoint');return;}";                                                                                                           
    html += "var p=document.getElementById('p').value;";                                                                                                                 
    html += "var f=document.getElementById('f').value;";                                                                                                                 
    html += "var body='ssid='+encodeURIComponent(sel)+'&password='+encodeURIComponent(p)+'&endpoint='+encodeURIComponent(e)+'&format='+f;";                              
    html += "fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})";                                                     
    html += ".then(function(r){return r.text()})";                                                                                                                       
    html += ".then(function(m){alert(m);});";                                                                                                                            
    html += "}";                                                                                                                                                         
    html += "</script></body></html>";                                                                                                                                   
                                                                                                                                                                         
    server.send(200, "text/html", html);                                                                                                                                 
  }       

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String endpoint = server.arg("endpoint");
  String format = server.arg("format");

  ssid.toCharArray(config.ssid, sizeof(config.ssid));
  password.toCharArray(config.password, sizeof(config.password));
  endpoint.toCharArray(config.endpoint, sizeof(config.endpoint));
  config.format = format.toInt();

  saveConfig();
  server.send(200, "text/plain", "Lagret! Starter pa nytt...");
  delay(1000);
  ESP.restart();
}

void handleNotFound() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

// ===== SETUP MODE =====

void startSetupMode() {
  setupMode = true;
  showSetupScreen();
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME);
  
  dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
  
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("Setup: " + String(AP_NAME));
}

// ===== BUS DATA =====

void fetchBusData() {
  if (WiFi.status() != WL_CONNECTED) {
    showMessage("WiFi frakoblet!", "Starter setup...");
    delay(2000);
    startSetupMode();
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  // Build URL with format query string
  String url = String(config.endpoint);
  if (config.format == FORMAT_LIST) {
    url += (url.indexOf('?') >= 0) ? "&format=list" : "?format=list";
  } else if (config.format == FORMAT_VERBOSE) {
    url += (url.indexOf('?') >= 0) ? "&format=verbose" : "?format=verbose";
  }
  // FORMAT_SIMPLE is default, no query needed

  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(10000);

  int httpCode = http.GET();

  if (httpCode == 200) {
    cachedPayload = http.getString();
    hasData = true;
  } else {
    showMessage("HTTP feil:", String(httpCode).c_str());
  }

  http.end();
}

// Temporary storage for parsed departures
struct Departure {
  char line[8];
  char destination[24];
  int minutes;
};

#define MAX_DEPARTURES 20
Departure departures[MAX_DEPARTURES];
int departureCount = 0;

void parseBusData() {
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, cachedPayload);

  if (error) {
    departureCount = 0;
    return;
  }

  departureCount = 0;

  if (config.format == FORMAT_SIMPLE) {
    // {"2": 0, "3": 1, "4": 3, ...}
    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair kv : obj) {
      if (departureCount >= MAX_DEPARTURES) break;
      strncpy(departures[departureCount].line, kv.key().c_str(), 7);
      departures[departureCount].line[7] = '\0';
      departures[departureCount].destination[0] = '\0';
      departures[departureCount].minutes = kv.value().as<int>();
      departureCount++;
    }
  }
  else if (config.format == FORMAT_LIST) {
    // [{"line": "31", "minutes": 0}, ...]
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject item : arr) {
      if (departureCount >= MAX_DEPARTURES) break;
      strncpy(departures[departureCount].line, item["line"] | "", 7);
      departures[departureCount].line[7] = '\0';
      departures[departureCount].destination[0] = '\0';
      departures[departureCount].minutes = item["minutes"] | 0;
      departureCount++;
    }
  }
  else if (config.format == FORMAT_VERBOSE) {
    // {"departures": [{"line": "31", "destination": "Tonsenhagen", "minutes": 0}, ...]}
    JsonArray arr = doc["departures"].as<JsonArray>();
    for (JsonObject item : arr) {
      if (departureCount >= MAX_DEPARTURES) break;
      strncpy(departures[departureCount].line, item["line"] | "", 7);
      departures[departureCount].line[7] = '\0';
      strncpy(departures[departureCount].destination, item["destination"] | "", 23);
      departures[departureCount].destination[23] = '\0';
      departures[departureCount].minutes = item["minutes"] | 0;
      departureCount++;
    }
  }
}

void displayBusData() {
  if (!hasData || cachedPayload.length() == 0) {
    showMessage("Henter data...");
    return;
  }

  // Parse data (could optimize to only parse on new data)
  static String lastPayload = "";
  if (cachedPayload != lastPayload) {
    parseBusData();
    lastPayload = cachedPayload;
  }

  if (departureCount == 0) {
    showMessage("Ingen avganger");
    return;
  }

  const int entriesPerPage = 5;
  totalPages = (departureCount + entriesPerPage - 1) / entriesPerPage;
  if (totalPages < 1) totalPages = 1;

  // Auto-advance page
  if (millis() - lastPageSwitch >= PAGE_INTERVAL) {
    currentPage = (currentPage + 1) % totalPages;
    lastPageSwitch = millis();
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ===== GUL SONE (y: 0-15) - Header =====
  display.setCursor(0, 4);
  display.print("Neste avganger");

  // ===== BLÅ SONE (y: 16-63) - Busstider =====
  int y = 17;
  int startEntry = currentPage * entriesPerPage;
  int endEntry = min(startEntry + entriesPerPage, departureCount);

  for (int i = startEntry; i < endEntry; i++) {
    display.setCursor(0, y);

    // Bussnummer i boks - calculate width based on line length
    int lineLen = strlen(departures[i].line);
    int boxWidth = max(14, lineLen * 6 + 4);
    display.fillRoundRect(0, y - 1, boxWidth, 9, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.print(departures[i].line);
    display.setTextColor(SSD1306_WHITE);

    // Minutter
    int xOffset = boxWidth + 3;
    display.setCursor(xOffset, y);
    int minutter = departures[i].minutes;

    if (minutter <= 0) {
      display.print("na");
      xOffset += 14;
    } else {
      display.print(minutter);
      display.print(" min");
      xOffset += (minutter >= 10 ? 12 : 6) + 24;
    }

    // Destination (verbose format only) - show truncated if space
    if (config.format == FORMAT_VERBOSE && departures[i].destination[0] != '\0') {
      display.setCursor(xOffset + 2, y);
      // Sanitize Norwegian chars and truncate to fit
      char dest[16];
      sanitizeNorwegian(dest, departures[i].destination, 14);
      display.print(dest);
    }

    y += 9;
  }
  display.display();
}

// ===== MAIN =====

void setup() {
  Serial.begin(115200);
  pinMode(RESET_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED feil!");
    while (true);
  }

  showMessage("Starter...");
  delay(500);

  if (digitalRead(RESET_PIN) == LOW) {
    showMessage("Reset config...");
    clearConfig();
    delay(1000);
  }

  loadConfig();

  if (!config.configured) {
    startSetupMode();
  } else {
    showMessage("Kobler til:", config.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      showMessage("Tilkoblet!", WiFi.localIP().toString().c_str());
      delay(1500);
    } else {
      showMessage("WiFi feilet!", "Starter setup...");
      delay(2000);
      clearConfig();
      startSetupMode();
    }
  }
}

void loop() {
  if (setupMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  } else {
    // Fetch new data periodically
    if (millis() - lastDataFetch >= DATA_INTERVAL || !hasData) {
      fetchBusData();
      lastDataFetch = millis();
    }

    // Check for button press to advance page (with debounce)
    static bool lastButtonState = HIGH;
    static unsigned long lastDebounce = 0;
    bool buttonState = digitalRead(RESET_PIN);

    if (buttonState != lastButtonState && millis() - lastDebounce > 200) {
      lastDebounce = millis();
      if (buttonState == LOW) {
        // Button just pressed - advance page
        currentPage = (currentPage + 1) % totalPages;
        lastPageSwitch = millis();  // Reset auto-advance timer
      }
    }
    lastButtonState = buttonState;

    // Update display
    displayBusData();

    delay(100);  // Small delay to prevent flicker
  }
}
