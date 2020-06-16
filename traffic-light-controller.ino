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
#include <WebSocketsServer.h>
#include <ezTime.h>

// Static content includes. File contents generated from originals with prepareStaticContent.py
#include "index_html.h"
#include "apple_touch_icon_png.h"
#include "favicon_ico.h"
#include "app_js.h"
#include "style_css.h"

const String HOST_NAME = "traffic-light";

const String FALSE_STRING = "false";
const String TRUE_STRING = "true";
const String EMPTY_STRING = "";

const String CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const String CONTENT_TYPE_APPLICATION_JSON = "application/json";
const String CONTENT_TYPE_TEXT_HTML = "text/html";
const String CONTENT_TYPE_APPLICATION_JAVASCRIPT = "application/javascript";
const String CONTENT_TYPE_TEXT_CSS = "text/css";

const int HTTP_OK = 200;
const int HTTP_NO_CONTENT = 204;
const int HTTP_NOT_FOUND = 404;
const int HTTP_METHOD_NOT_ALLOWED = 405;
const int HTTP_BAD_REQUEST = 400;

const String METHOD_NOT_ALLOWED_MESSAGE = "Method Not Allowed";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
StaticJsonDocument<200> doc; //todo this should not be global because of https://arduinojson.org/v6/faq/i-found-a-memory-leak-in-the-library/

boolean _redLit = false;
boolean _greenLit = false;
int _bpm = 100;
boolean _partyOn = false;
unsigned long _currentMillis = millis();
unsigned long _beatStartMillis = millis();
String _beatStartEpoch = "0";
int _rhythmStep = 0;

const int RHYTHM_STEPS = 8;
const int RHYTHM_PATTERN[] = {RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, BOTH_FLASH, BOTH_FLASH};

void sendToWebSocketClients(String webSocketMessage){
  for(int i=0; i < webSocket.connectedClients(false); i++){
    webSocket.sendTXT(i, webSocketMessage);
  }
}

void lightSwitch(int light, boolean newState){
  switch(light){
    case RED_LIGHT: _redLit = newState; break;
    case GREEN_LIGHT: _greenLit = newState; break;
  }
    
  digitalWrite(light, !newState); // because LOW means ON
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

String epochMillis(){
  char buffer [3];
  sprintf(buffer,"%03d",ms());
  return String(now()) + buffer;
}

void rhythm(){
  int timingIntervalMilis = 60000 / (_bpm * 2);
  
  _currentMillis = millis();
  
  if(_currentMillis - _beatStartMillis > timingIntervalMilis){
    _beatStartMillis = _currentMillis;
    _beatStartEpoch = epochMillis();
    
    if(_rhythmStep >= (RHYTHM_STEPS * 2) - 1){
      _rhythmStep = 0;
    }else{
      _rhythmStep++;
    }
  }
  
}

void htmlRootContent() {
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send(HTTP_OK, CONTENT_TYPE_TEXT_HTML, INDEX_HTML);
}

void appJsContent(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JAVASCRIPT, APP_JS);
}

void styleCssContent(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send(HTTP_OK, CONTENT_TYPE_TEXT_CSS, STYLE_CSS);
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
  if(server.method() == HTTP_PUT) {
    server.send(HTTP_OK, CONTENT_TYPE_TEXT_PLAIN, "Party On!");
    Serial.println("Party started");
    waitForSync();
    _partyOn = true;
  } else if (server.method() == HTTP_DELETE) {
    server.send(HTTP_OK, CONTENT_TYPE_TEXT_PLAIN, "Party's Over");
    Serial.println("Party ended");
    _partyOn = false;
  } else if(server.method() == HTTP_GET){
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, "{ \"party\": " + String(_partyOn ? TRUE_STRING : FALSE_STRING) + "}");
  } else{
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String clientIP(){
  return server.client().remoteIP().toString();
}

void handleRed(){
  if(server.method() == HTTP_PUT){
    lightSwitch(RED_LIGHT, true);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (server.method() == HTTP_DELETE){
    lightSwitch(RED_LIGHT, false);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (server.method() == HTTP_GET){
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, "{ \"lit\": " + String(_redLit ? TRUE_STRING : FALSE_STRING) + "}");
  } else {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void handleGreen(){
  if(server.method() == HTTP_PUT){
    lightSwitch(GREEN_LIGHT, true);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  }else if(server.method() == HTTP_DELETE){
    lightSwitch(GREEN_LIGHT, false);
    server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  }else if(server.method() == HTTP_GET){
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, "{ \"lit\": " + String(_greenLit ? TRUE_STRING : FALSE_STRING) + "}");
  }else{
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void handleStatus(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  } else {
    String content;

    //todo build status json
    serializeJson(_stateJson, content);
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, content);
  }
}

void handleTempo(){
  if(server.method() == HTTP_PUT){
    if(server.hasArg("plain") == false) {
      server.send(HTTP_BAD_REQUEST, CONTENT_TYPE_TEXT_PLAIN, "Missing body");
    }else{
      deserializeJson(doc, server.arg("plain"));
      waitForSync();
      _bpm = doc["bpm"];
      Serial.println("BPM is now: " + String(_bpm));
      server.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
    }
  } else if (server.method() == HTTP_GET){
    server.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, "{ \"bpm\": " + String(_bpm) + "}");
  } else {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void favicon(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send_P(HTTP_OK, "image/x-icon", favicon_io_favicon_ico, sizeof(favicon_io_favicon_ico));
}

void appleTouchIcon(){
  if (server.method() != HTTP_GET) {
    server.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN, METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }
  server.send_P(HTTP_OK, "image/png", favicon_io_apple_touch_icon_png, sizeof(favicon_io_apple_touch_icon_png));
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

  server.on("/", htmlRootContent);
  server.on("/app.js", appJsContent);
  server.on("/style.css", styleCssContent);

  server.on("/favicon.ico", favicon);
  server.on("/apple-touch-icon.png", appleTouchIcon);
  
  server.on("/api/red", handleRed);
  server.on("/api/green", handleGreen);
  server.on("/api/tempo", handleTempo);
  server.on("/api/party", handleParty);
  server.on("/api/status", handleStatus);
  
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  waitForSync();
}

void loop(void) {
  server.handleClient();
  rhythm();
  
  if(_partyOn){
    partyFlash();
  }
}
