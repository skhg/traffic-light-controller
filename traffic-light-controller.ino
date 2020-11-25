/**
 * Copyright 2020 Jack Higgins : https://github.com/skhg
 * All components of this project are licensed under the MIT License.
 * See the LICENSE file for details.
 */

#define RED_LIGHT D6
#define GREEN_LIGHT D5
#define SENSOR_RED_PIN D3
#define SENSOR_GREEN_PIN D4

#define RED_FLASH 0
#define GREEN_FLASH 1
#define BOTH_FLASH 2

#define DHT_SENSOR_TYPE DHT11

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <home_wifi.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsServer.h>
#include <DHT.h>

const String STATIC_CONTENT_INDEX_LOCATION =
"http://jackhiggins.ie/traffic-light-controller/";

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

const int TEMPERATURE_READ_INTERVAL_MILLIS = 10000;

WiFiClient WIFI_CLIENT;
HTTPClient HTTP_CLIENT;
ESP8266WebServer HTTP_SERVER(80);
WebSocketsServer WEB_SOCKET_SERVER(81);
DHT SENSOR_RED(SENSOR_RED_PIN, DHT_SENSOR_TYPE);
DHT SENSOR_GREEN(SENSOR_GREEN_PIN, DHT_SENSOR_TYPE);

boolean _redLit = false;
boolean _greenLit = false;
int _bpm = 100;
boolean _partyOn = false;
uint64_t _currentMillis = millis();
uint64_t _beatStartMillis = millis();
uint64_t _temperatureReadMillis = millis();
int _rhythmStep = 0;

String _currentTitle = "";
String _currentArtist = "";
String _currentAlbum = "";

double _redTemperature = 0.0;
double _redHumidity = 0.0;
double _greenTemperature = 0.0;
double _greenHumidity = 0.0;

const int RHYTHM_STEPS = 8;
const int RHYTHM_PATTERN[] = {
  RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH,
  RED_FLASH, GREEN_FLASH, BOTH_FLASH, BOTH_FLASH
};

void sendToWebSocketClients(String webSocketMessage) {
  WEB_SOCKET_SERVER.broadcastTXT(webSocketMessage);
}

void lightSwitch(int light, boolean newState) {
  digitalWrite(light, !newState);  // LOW means ON for the relay i'm using

  switch (light) {
    case RED_LIGHT: {
      if (newState != _redLit) {
        _redLit = newState;
        sendToWebSocketClients(redLightJson());
      }
      break;
    }
    case GREEN_LIGHT: {
      if (newState != _greenLit) {
        _greenLit = newState;
        sendToWebSocketClients(greenLightJson());
      }

      break;
    }
  }
}

void partyFlash() {
  if (_rhythmStep % 2 == 1) {  // Any odd number
    lightSwitch(RED_LIGHT, false);
    lightSwitch(GREEN_LIGHT, false);
  } else {
    switch (RHYTHM_PATTERN[_rhythmStep / 2]) {
      case RED_FLASH: {
        lightSwitch(RED_LIGHT, true);
        lightSwitch(GREEN_LIGHT, false);
        break;
      }
      case GREEN_FLASH: {
        lightSwitch(RED_LIGHT, false);
        lightSwitch(GREEN_LIGHT, true);
        break;
      }
      case BOTH_FLASH: {
        lightSwitch(RED_LIGHT, true);
        lightSwitch(GREEN_LIGHT, true);
        break;
      }
    }
  }
}

void readSensors() {
  _currentMillis = millis();

  if (_currentMillis - _temperatureReadMillis >
        TEMPERATURE_READ_INTERVAL_MILLIS) {
    _temperatureReadMillis = _currentMillis;

    _redHumidity = SENSOR_RED.readHumidity();
    _redTemperature = SENSOR_RED.readTemperature();

    _greenHumidity = SENSOR_GREEN.readHumidity();
    _greenTemperature = SENSOR_GREEN.readTemperature();

    sendToWebSocketClients(sensorsJson());
  }
}

String sensorsJson() {
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> sensorsJson;
  sensorsJson["redTemperature"] = _redTemperature;
  sensorsJson["greenTemperature"] = _greenTemperature;
  sensorsJson["greenHumidity"] = _greenHumidity;
  sensorsJson["redHumidity"] = _redHumidity;
  serializeJson(sensorsJson, content);
  return content;
}

void rhythm() {
  int timingIntervalMillis = 60000 / (_bpm * 2);

  _currentMillis = millis();

  if (_currentMillis - _beatStartMillis > timingIntervalMillis) {
    _beatStartMillis = _currentMillis;

    if (_rhythmStep >= (RHYTHM_STEPS * 2) - 1) {
      _rhythmStep = 0;
    } else {
      _rhythmStep++;
    }
  }
}

void htmlRootContent() {
  if (HTTP_SERVER.method() != HTTP_GET) {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
    return;
  }

  // Fetch content from source and forward it
  HTTP_CLIENT.begin(WIFI_CLIENT, STATIC_CONTENT_INDEX_LOCATION);
  HTTP_CLIENT.GET();

  HTTP_SERVER.sendHeader("Cache-Control", "no-cache");
  HTTP_SERVER.send(HTTP_OK, "text/html", HTTP_CLIENT.getString());
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

void handleParty() {
  if (HTTP_SERVER.method() == HTTP_PUT) {
    _partyOn = true;
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_TEXT_PLAIN, "Party On!");
    Serial.println("Party started");
    sendToWebSocketClients(partyJson());
  } else if (HTTP_SERVER.method() == HTTP_DELETE) {
    _partyOn = false;
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_TEXT_PLAIN, "Party's Over");
    Serial.println("Party ended");
    sendToWebSocketClients(partyJson());
  } else if (HTTP_SERVER.method() == HTTP_GET) {
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, partyJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String partyJson() {
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> partyJson;
  partyJson["party"] = _partyOn;
  serializeJson(partyJson, content);
  return content;
}

String clientIP() {
  return HTTP_SERVER.client().remoteIP().toString();
}

void handleRed() {
  if (HTTP_SERVER.method() == HTTP_PUT) {
    lightSwitch(RED_LIGHT, true);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (HTTP_SERVER.method() == HTTP_DELETE) {
    lightSwitch(RED_LIGHT, false);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (HTTP_SERVER.method() == HTTP_GET) {
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, redLightJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String redLightJson() {
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> redJson;
  redJson["red"] = _redLit;
  serializeJson(redJson, content);
  return content;
}

String greenLightJson() {
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> greenJson;
  greenJson["green"] = _greenLit;
  serializeJson(greenJson, content);
  return content;
}

void handleGreen() {
  if (HTTP_SERVER.method() == HTTP_PUT) {
    lightSwitch(GREEN_LIGHT, true);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (HTTP_SERVER.method() == HTTP_DELETE) {
    lightSwitch(GREEN_LIGHT, false);
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
  } else if (HTTP_SERVER.method() == HTTP_GET) {
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, greenLightJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
  }
}

void handleStatus() {
  if (HTTP_SERVER.method() != HTTP_GET) {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
  } else {
    String content;

    StaticJsonDocument<JSON_OBJECT_SIZE(8) + 1000> statusJson;
    statusJson["bpm"] = _bpm;
    statusJson["green"] = _greenLit;
    statusJson["red"] = _redLit;
    statusJson["party"] = _partyOn;
    statusJson["title"] = _currentTitle;
    statusJson["artist"] = _currentArtist;
    statusJson["album"] = _currentAlbum;
    statusJson["redTemperature"] = _redTemperature;
    statusJson["greenTemperature"] = _greenTemperature;
    statusJson["greenHumidity"] = _greenHumidity;
    statusJson["redHumidity"] = _redHumidity;
    serializeJson(statusJson, content);

    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, content);
  }
}

void handleTempo() {
  if (HTTP_SERVER.method() == HTTP_PUT) {
    if (HTTP_SERVER.hasArg("plain") == false) {
      HTTP_SERVER.send(HTTP_BAD_REQUEST, CONTENT_TYPE_TEXT_PLAIN,
        "Missing body");
    } else {
      StaticJsonDocument<JSON_OBJECT_SIZE(1) + 10> bpmJson;
      deserializeJson(bpmJson, HTTP_SERVER.arg("plain"));
      _bpm = bpmJson["bpm"];
      Serial.println("BPM is now: " + String(_bpm));
      HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
      sendToWebSocketClients(tempoJson());
    }
  } else if (HTTP_SERVER.method() == HTTP_GET) {
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, tempoJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String tempoJson() {
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> tempoJson;
  tempoJson["bpm"] = _bpm;
  serializeJson(tempoJson, content);
  return content;
}

void handleSong() {
  if (HTTP_SERVER.method() == HTTP_PUT) {
    if (HTTP_SERVER.hasArg("plain") == false) {
      HTTP_SERVER.send(HTTP_BAD_REQUEST, CONTENT_TYPE_TEXT_PLAIN,
        "Missing body");
    } else {
      StaticJsonDocument<JSON_OBJECT_SIZE(3) + 1000> newSongJson;
      deserializeJson(newSongJson, HTTP_SERVER.arg("plain"));
      const char* artist = newSongJson["artist"];
      _currentArtist = artist;
      const char* album = newSongJson["album"];
      _currentAlbum = album;
      const char* title = newSongJson["title"];
      _currentTitle = title;
      Serial.println("Song is now: " + _currentTitle + " from "+ _currentAlbum +
        " by " + _currentArtist);
      HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
      sendToWebSocketClients(songJson());
    }
  } else if (HTTP_SERVER.method() == HTTP_GET) {
    HTTP_SERVER.send(HTTP_OK, CONTENT_TYPE_APPLICATION_JSON, songJson());
  } else if (HTTP_SERVER.method() == HTTP_DELETE) {
    _currentArtist = "";
    _currentAlbum = "";
    _currentTitle = "";
    Serial.println("Song ended");
    HTTP_SERVER.send(HTTP_NO_CONTENT, CONTENT_TYPE_TEXT_PLAIN, EMPTY_STRING);
    sendToWebSocketClients(songJson());
  } else {
    HTTP_SERVER.send(HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT_PLAIN,
      METHOD_NOT_ALLOWED_MESSAGE);
  }
}

String songJson() {
  String content;
  StaticJsonDocument<JSON_OBJECT_SIZE(3) + 1000> changedSongJson;
  changedSongJson["title"] = _currentTitle;
  changedSongJson["artist"] = _currentArtist;
  changedSongJson["album"] = _currentAlbum;
  serializeJson(changedSongJson, content);
  return content;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload,
  size_t length) {
  IPAddress ip = WEB_SOCKET_SERVER.remoteIP(num);

  switch (type) {
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
  Serial.begin(115200);

  SENSOR_RED.begin();
  SENSOR_GREEN.begin();

  pinMode(RED_LIGHT, OUTPUT);
  pinMode(GREEN_LIGHT, OUTPUT);

  digitalWrite(RED_LIGHT, HIGH);
  digitalWrite(GREEN_LIGHT, HIGH);

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
  HTTP_SERVER.on("/api/song", handleSong);

  HTTP_SERVER.onNotFound(handleNotFound);

  HTTP_SERVER.begin();
  Serial.println("HTTP server started");

  WEB_SOCKET_SERVER.begin();

  // Disconnect after a single failed heartbeat
  WEB_SOCKET_SERVER.enableHeartbeat(1000, 1000, 1);
  WEB_SOCKET_SERVER.onEvent(webSocketEvent);
}

void loop(void) {
  HTTP_SERVER.handleClient();
  rhythm();
  readSensors();

  if (_partyOn) {
    partyFlash();
  }
}
