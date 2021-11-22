#include <Arduino.h>
#include <ESP8266WiFi.h> 
#include <DNSServer.h> 
#include <ESP8266WebServer.h> 
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h> 
#include <WiFiClient.h>
#include <PubSubClient.h>

#define             EEPROM_LENGTH 1024
#define             RESET_PIN 0
ESP8266WebServer    webServer(80);
char                eRead[30];
char                ssid[30];
char                password[30]; 
char                mqttServer[30]; 

int                 interval = 2000;
unsigned long       lastPublished = - interval;

const int Relay = 15;

HTTPClient http;
WiFiClient espClient;
PubSubClient client(espClient);

String responseHTML = ""
    "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body><center>"
    "<p>Captive Sample Server App</p>"
    "<form action='/save'>"
    "<p><input type='text' name='ssid' placeholder='SSID' onblur='this.value=removeSpaces(this.value);'></p>"
    "<p><input type='text' name='password' placeholder='WLAN Password'></p>" 
    "<p><input type='text' name='mqttServer' placeholder='MQTT Server'></p>"//mqtt Server
    "<p><input type='submit' value='Submit'></p></form>"
    "<p>This is a captive portal example</p></center></body>"
    "<script>function removeSpaces(string) {"
    "   return string.split(' ').join('');"
    "}</script></html>";

// Saves string to EEPROM
void SaveString(int startAt, const char* id) { 
    for (byte i = 0; i <= strlen(id); i++) {
        EEPROM.write(i + startAt, (uint8_t) id[i]);
    }
    EEPROM.commit();
}

// Reads string from EEPROM
void ReadString(byte startAt, byte bufor) {
    for (byte i = 0; i <= bufor; i++) {
        eRead[i] = (char)EEPROM.read(i + startAt);
    }
}

void save(){
    Serial.println("button pressed");
    Serial.println(webServer.arg("ssid"));
    Serial.println(webServer.arg("mqttServer"));
    SaveString( 0, (webServer.arg("ssid")).c_str());
    SaveString(30, (webServer.arg("password")).c_str());
    SaveString(60, (webServer.arg("mqttServer")).c_str()); //mqtt Server save
    webServer.send(200, "text/plain", "OK");
    ESP.restart(); 
}

void configWiFi() {
    const byte DNS_PORT = 53;
    IPAddress apIP(192, 168, 1, 1);
    DNSServer dnsServer;
    
    WiFi.mode(WIFI_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("wonazz");     // change this to your portal SSID
    
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/save", save);

    webServer.onNotFound([]() {
        webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();
    while(true) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        yield();
    }
}

void load_config_wifi() {
    ReadString(0, 30);
    if (!strcmp(eRead, "")) {
        Serial.println("Config Captive Portal started");
        configWiFi();
    } else {
        Serial.println("IOT Device started");
        strcpy(ssid, eRead);
        ReadString(30, 30);
        strcpy(password, eRead);
        ReadString(60, 30);
        strcpy(mqttServer, eRead); //read mqtt server
    }
}

IRAM_ATTR void GPIO0() {
    SaveString(0, ""); // blank out the SSID field in EEPROM
    ESP.restart();
} 

void callback(char* topic, byte* payload, unsigned int length){
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    for (int i=0;i<length; i++){
        //if light over 100, publish '1' to esp8266
        if((char)payload[i]=='1'){ 
            Serial.print("Relay on"); //when you get '1', Relay on
            digitalWrite(Relay,HIGH);
        }else {
            Serial.print("Relay off"); //otherwise
            digitalWrite(Relay,LOW);
        }
    }
    Serial.println();
}

void setup() {

    Serial.begin(115200);

    pinMode(Relay,OUTPUT);
    digitalWrite(Relay,LOW);

    EEPROM.begin(EEPROM_LENGTH);
    pinMode(RESET_PIN, INPUT_PULLUP);
    attachInterrupt(RESET_PIN, GPIO0, FALLING);
    while(!Serial);
    Serial.println();

    load_config_wifi(); // load or config wifi if not configured
   
    WiFi.mode(WIFI_AP);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(i++ > 15) {
            configWiFi();
        }
    }
    Serial.print("Connected to "); Serial.println(ssid);
    Serial.print("IP address: "); Serial.println(WiFi.localIP());

    if (MDNS.begin("wonazz")) {
        Serial.println("MDNS responder started");
    }
    
    client.setServer(mqttServer,1883);
    client.setCallback(callback);
    
    while(!client.connected()){
        Serial.println("Connecting to MQTT...");
        
        if(client.connect("wonaz2")){
            Serial.println("connected");
        }else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }
    client.subscribe("deviceid/wonaz2/#");
}


void loop() {
    MDNS.update();
    // your loop code here
    client.loop();

    unsigned long currentMillis = millis();
    if(currentMillis - lastPublished >= interval){
        lastPublished = currentMillis;
        client.publish("esp/test","Hello from ESP8266");
    }
}