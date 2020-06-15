#define RED_LIGHT D6
#define GREEN_LIGHT D5

#define RED_FLASH 0
#define GREEN_FLASH 1
#define BOTH_FLASH 2

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <home_wifi.h>
#include <ArduinoJson.h>
#include "traffic_light_static_content.h"

const String CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const String CONTENT_TYPE_APPLICATION_JSON = "application/json";
const String CONTENT_TYPE_TEXT_HTML = "text/html";

const int HTTP_OK = 200;
const int HTTP_NO_CONTENT = 204;
const int HTTP_NOT_FOUND = 404;
const int HTTP_METHOD_NOT_ALLOWED = 405;
const int HTTP_BAD_REQUEST = 400;

const String METHOD_NOT_ALLOWED_MESSAGE = "Method Not Allowed";

const String HOST_NAME = "traffic-light";

const String FALSE_STRING = "false";
const String TRUE_STRING = "true";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
StaticJsonDocument<200> doc;

StaticJsonDocument<JSON_OBJECT_SIZE(1)> _redJson;
StaticJsonDocument<JSON_OBJECT_SIZE(1)> _greenJson;
StaticJsonDocument<JSON_OBJECT_SIZE(1)> _partyJson;
StaticJsonDocument<JSON_OBJECT_SIZE(1)> _bpmJson;

boolean _redLit = false;
boolean _greenLit = false;
int _bpm = 100;
boolean _partyOn = false;
unsigned long _currentMillis = 0;
unsigned long _beatStartMillis = 0;
int _rhythmStep = 0;

const int RHYTHM_STEPS = 8;
const int RHYTHM_PATTERN[] = {RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, BOTH_FLASH, BOTH_FLASH};

void sendToWebSocketClients(String webSocketMessage){
  for(int i=0; i < webSocket.connectedClients(false); i++){
    webSocket.sendTXT(i, webSocketMessage);
  }
}

void lightSwitch(int light, boolean newState){
  String webSocketMessage;
  
  switch(light){
    case RED_LIGHT: {
      _redLit = newState;
      _redJson["red"] = newState;
      serializeJson(_redJson, webSocketMessage);
      break;
    }
    case GREEN_LIGHT: {
      _greenLit = newState;
      _greenJson["green"] = newState;
      serializeJson(_greenJson, webSocketMessage);
      break;
    }
  }
    
  digitalWrite(light, !newState); // because LOW means ON

  sendToWebSocketClients(webSocketMessage);
}

void partyFlash(){
  if(_rhythmStep % 2 == 1){ // an odd number
    lightSwitch(RED_LIGHT, false);
    lightSwitch(GREEN_LIGHT, false);
  }else{
    switch(RHYTHM_PATTERN[_rhythmStep / 2]){
      case RED_FLASH: lightSwitch(RED_LIGHT, true); lightSwitch(GREEN_LIGHT, false); break;
      case GREEN_FLASH: lightSwitch(RED_LIGHT, false); lightSwitch(GREEN_LIGHT, true); break;
      case BOTH_FLASH: lightSwitch(RED_LIGHT, true); lightSwitch(GREEN_LIGHT, true); break;
    }
  }
}

void rhythm(){
  int timingIntervalMilis = 60000 / (_bpm * 2);
  
  _currentMillis = millis();
  
  if(_currentMillis - _beatStartMillis > timingIntervalMilis){
    _beatStartMillis = _currentMillis;

    if(_rhythmStep >= (RHYTHM_STEPS * 2) - 1){
      _rhythmStep = 0;
    }else{
      _rhythmStep++;
    }
  }
  
}

void handleRoot() {
  server.send(HTTP_OK, CONTENT_TYPE_TEXT_HTML, STATIC_HTML_PAGE);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(HTTP_NOT_FOUND, CONTENT_TYPE_TEXT_PLAIN, message);
}

void handleParty(){
  String partyMessage;
  
  if(server.method() == HTTP_PUT) {
    _partyJson["party"] = true;
    _partyOn = true;
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");

    serializeJson(_partyJson, partyMessage);
    sendToWebSocketClients(partyMessage);
    
  } else if (server.method() == HTTP_DELETE) {
    _partyJson["party"] = false;
    _partyOn = false;
    
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");
    
    serializeJson(_partyJson, partyMessage);
    sendToWebSocketClients(partyMessage);
  } else if (server.method() == HTTP_GET){
    serializeJson(_partyJson, partyMessage);
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, partyMessage);
  } else{
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void handleRed(){
  if(server.method() == HTTP_PUT){
    lightSwitch(RED_LIGHT, true);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");
  } else if (server.method() == HTTP_DELETE){
    lightSwitch(RED_LIGHT, false);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");
  } else if (server.method() == HTTP_GET){
    String redMessage;
    serializeJson(_redJson, redMessage);
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, redMessage);
  } else {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void handleGreen(){
  if(server.method() == HTTP_PUT){
    lightSwitch(GREEN_LIGHT, true);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");
  }else if(server.method() == HTTP_DELETE){
    lightSwitch(GREEN_LIGHT, false);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");
  }else if(server.method() == HTTP_GET){
    String greenMessage;
    serializeJson(_greenJson, greenMessage);
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, greenMessage);
  }else{
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void lightStatus(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  } else {
    String content = "\
    {\
      \"red\" : " + String(_redLit ? TRUE_STRING : FALSE_STRING) + ",\
      \"green\" : " + String(_greenLit ? TRUE_STRING : FALSE_STRING) + ",\
      \"party\" : " + String(_partyOn ? TRUE_STRING : FALSE_STRING) + ",\
      \"bpm\" : " + String(_bpm) + "\
    }";
    
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, content);

  }
}

void handleTempo(){
  String bpmMessage;
  
  if(server.method() == HTTP_PUT){
    if(server.hasArg("plain") == false) {
      server.send(HTTP_BAD_REQUEST, CONTENT_TYPE_TEXT_PLAIN, "Missing body");
    }else{
      deserializeJson(doc, server.arg("plain"));
      _bpm = doc["bpm"];
      _bpmJson["bpm"] = _bpm;
      serializeJson(_bpmJson, bpmMessage);
      sendToWebSocketClients(bpmMessage);
      
      server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, "");
    }
  } else if (server.method() == HTTP_GET){
    serializeJson(_bpmJson, bpmMessage);
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, bpmMessage);
  } else {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void favicon(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send_P(HTTP_OK, "image/x-icon", FAVICON_ICO, sizeof(FAVICON_ICO));
}

void appleTouchIcon(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send_P(HTTP_OK, "image/png", APPLE_TOUCH_ICON, sizeof(APPLE_TOUCH_ICON));
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  IPAddress ip = webSocket.remoteIP(num);
  
  switch(type) {
    case WStype_DISCONNECTED: {
      Serial.println("WebSocket client disconnected."); break;
    }
    case WStype_CONNECTED: {
      Serial.print("WebSocket client at ");
      Serial.print(ip);
      Serial.println(" connected.");
    }; break;
  }
}

void setup(void) {
  _redJson["red"] = false;
  _greenJson["green"] = false;
  _partyJson["party"] = false;
  _bpmJson["bpm"] = 100;
  
  _currentMillis = millis();
  
  pinMode(RED_LIGHT, OUTPUT);
  pinMode(GREEN_LIGHT, OUTPUT);

  digitalWrite(RED_LIGHT, HIGH);
  digitalWrite(GREEN_LIGHT, HIGH);
  
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.hostname(HOST_NAME);

  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/api/red", handleRed);
  server.on("/api/green", handleGreen);
  server.on("/api/tempo", handleTempo);
  server.on("/api/party", handleParty);
  server.on("/api/status", lightStatus);
  server.on("/favicon.ico", favicon);
  server.on("/apple-touch-icon.png", appleTouchIcon);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");


  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Web Socket server started");
}

void loop(void) {
  webSocket.loop();
  server.handleClient();
  rhythm();
  
  if(_partyOn){
    partyFlash();
  }
}
