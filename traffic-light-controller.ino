#define RED_LIGHT D6
#define GREEN_LIGHT D5

#define RED_FLASH 0
#define GREEN_FLASH 1
#define BOTH_FLASH 2

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <home_wifi.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsServer.h>

// Static content includes. File contents generated from originals with prepareStaticContent.py
#include "index_html_gz.h"

const String HOST_NAME = "traffic-light";

const String EMPTY_STRING = "";

const String CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const String CONTENT_TYPE_APPLICATION_JSON = "application/json";
const String CONTENT_TYPE_TEXT_HTML = "text/html";

const int HTTP_OK = 200;
const int HTTP_NO_CONTENT = 204;
const int HTTP_NOT_FOUND = 404;
const int HTTP_METHOD_NOT_ALLOWED = 405;
const int HTTP_BAD_REQUEST = 400;

const String METHOD_NOT_ALLOWED_MESSAGE = "Method Not Allowed";

ESP8266WebServer HTTP_SERVER(80);
WebSocketsServer WEB_SOCKET_SERVER(81);

boolean _redLit = false;
boolean _greenLit = false;
int _bpm = 100;
boolean _partyOn = false;
unsigned long _currentMillis = millis();
unsigned long _beatStartMillis = millis();
int _rhythmStep = 0;

const int RHYTHM_STEPS = 8;
const int RHYTHM_PATTERN[] = {RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, BOTH_FLASH, BOTH_FLASH};

void sendToWebSocketClients(String webSocketMessage){
  WEB_SOCKET_SERVER.broadcastTXT(webSocketMessage);
}

void lightSwitch(int light, boolean newState){
  digitalWrite(light, !newState); // because LOW means ON
  
  switch(light){
    case RED_LIGHT: {
      if(newState != _redLit){
        _redLit = newState;
        sendToWebSocketClients(redLightJson());
      }
      break;
    }
    case GREEN_LIGHT: {
      if(newState != _greenLit){
        _greenLit = newState;
        sendToWebSocketClients(greenLightJson());
      }
      
      break;
    }
  }
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

void htmlRootContent() {
  if (HTTP_SERVER.method() != HTTP_GET) {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  HTTP_SERVER.sendHeader("Content-Encoding", "gzip");
  HTTP_SERVER.send_P(HTTP_OK, "text/html", public_index_html_gz, sizeof(public_index_html_gz));
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += HTTP_SERVER.uri();
  message += "\nMethod: ";
  message += (HTTP_SERVER.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HTTP_SERVER.args();
  message += "\n";
  for (uint8_t i = 0; i < HTTP_SERVER.args(); i++) {
    message += " " + HTTP_SERVER.argName(i) + ": " + HTTP_SERVER.arg(i) + "\n";
  }
  HTTP_SERVER.send(HTTP_NOT_FOUND, CONTENT_TYPE_TEXT_PLAIN, message);
}

void handleParty(){
  if(HTTP_SERVER.method() == HTTP_PUT) {
    _partyOn = true;
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_TEXT_PLAIN, "Party On!");
    Serial.println("Party started");
    sendToWebSocketClients(partyJson());
  } else if (HTTP_SERVER.method() == HTTP_DELETE) {
    _partyOn = false;
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_TEXT_PLAIN, "Party's Over");
    Serial.println("Party ended");
    sendToWebSocketClients(partyJson());
  } else if(HTTP_SERVER.method() == HTTP_GET){
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, partyJson());
  } else{
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String partyJson(){
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> partyJson;
  partyJson["party"] = _partyOn;
  serializeJson(partyJson, content);
  return content;
}

String clientIP(){
  return HTTP_SERVER.client().remoteIP().toString();
}

void handleRed(){
  if(HTTP_SERVER.method() == HTTP_PUT){
    lightSwitch(RED_LIGHT, true);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (HTTP_SERVER.method() == HTTP_DELETE){
    lightSwitch(RED_LIGHT, false);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (HTTP_SERVER.method() == HTTP_GET){
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, redLightJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String redLightJson(){
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> redJson;
  redJson["red"] = _redLit;
  serializeJson(redJson, content);
  return content;
}

String greenLightJson(){
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> greenJson;
  greenJson["green"] = _greenLit;
  serializeJson(greenJson, content);
  return content;
}

void handleGreen(){
  if(HTTP_SERVER.method() == HTTP_PUT){
    lightSwitch(GREEN_LIGHT, true);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  }else if(HTTP_SERVER.method() == HTTP_DELETE){
    lightSwitch(GREEN_LIGHT, false);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  }else if(HTTP_SERVER.method() == HTTP_GET){
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, greenLightJson());
  }else{
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void handleStatus(){
  if (HTTP_SERVER.method() != HTTP_GET) {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  } else {
    String content;

    StaticJsonDocument<JSON_OBJECT_SIZE(5) + 50> statusJson;
    statusJson["bpm"] = _bpm;
    statusJson["green"] = _greenLit;
    statusJson["red"] = _redLit;
    statusJson["party"] = _partyOn;
    serializeJson(statusJson, content);
    
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, content);
  }
}

void handleTempo(){
  if(HTTP_SERVER.method() == HTTP_PUT){
    if(HTTP_SERVER.hasArg("plain") == false) {
      HTTP_SERVER.send(HTTP_BAD_REQUEST, CONTENT_TYPE_TEXT_PLAIN, "Missing body");
    }else{
      StaticJsonDocument<JSON_OBJECT_SIZE(1) + 10> bpmJson;
      deserializeJson(bpmJson, HTTP_SERVER.arg("plain"));
      _bpm = bpmJson["bpm"];
      Serial.println("BPM is now: " + String(_bpm));
      HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
      sendToWebSocketClients(tempoJson());
    }
  } else if (HTTP_SERVER.method() == HTTP_GET){
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, tempoJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String tempoJson(){
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> tempoJson;
  tempoJson["bpm"] = _bpm;
  serializeJson(tempoJson, content);
  return content;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  IPAddress ip = WEB_SOCKET_SERVER.remoteIP(num);

  switch(type) {
    case WStype_DISCONNECTED: {
      Serial.println("WebSocket client disconnected.");
      break;
    }
    case WStype_CONNECTED: {
      Serial.print("WebSocket client at ");
      Serial.print(ip);
      Serial.println(" connected.");
      break;
    }
  }
}  

void setup(void) {  
  pinMode(RED_LIGHT, OUTPUT);
  pinMode(GREEN_LIGHT, OUTPUT);

  digitalWrite(RED_LIGHT, HIGH);
  digitalWrite(GREEN_LIGHT, HIGH);
  
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.hostname(HOST_NAME);

  Serial.println(EMPTY_STRING);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(EMPTY_STRING);
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("Access at http://");
  Serial.print(WiFi.localIP());
  Serial.print("/ or http://");
  Serial.print(HOST_NAME);
  Serial.println("/");

  HTTP_SERVER.on("/", htmlRootContent);
  
  HTTP_SERVER.on("/api/red", handleRed);
  HTTP_SERVER.on("/api/green", handleGreen);
  HTTP_SERVER.on("/api/tempo", handleTempo);
  HTTP_SERVER.on("/api/party", handleParty);
  HTTP_SERVER.on("/api/status", handleStatus);
  
  
  HTTP_SERVER.onNotFound(handleNotFound);

  HTTP_SERVER.begin();
  Serial.println("HTTP server started");

  WEB_SOCKET_SERVER.begin();
  WEB_SOCKET_SERVER.enableHeartbeat(1000, 1000, 1); // Disconnect after a single failed heartbeat
  WEB_SOCKET_SERVER.onEvent(webSocketEvent);
}

void loop(void) {
  HTTP_SERVER.handleClient();
  rhythm();
  
  if(_partyOn){
    partyFlash();
  }
}
