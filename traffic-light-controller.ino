#define RED_LIGHT D1
#define GREEN_LIGHT D2

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <home_wifi.h>

const String HOST_NAME = "traffic-light";
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;

boolean _redLit = false;
boolean _greenLit = false;

String renderPage() {

  String redFormAction;
  String greenFormAction;
  
  String redButton;
  String greenButton;
  
  if(_redLit){
    redFormAction = "/red/off";
    redButton = "OFF";
  }else{
    redFormAction = "/red/on";
    redButton = "ON";
  }

  if(_greenLit){
    greenFormAction = "/green/off";
    greenButton = "OFF";
  }else{
    greenFormAction = "/green/on";
    greenButton = "ON";
  }

  return
  "<html>\
    <head>\
      <title>Traffic Light</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
      </style>\
    </head>\
    <body>\
      <h1>RED Light</h1>\
      <form method=\"post\" action=\"" + redFormAction + "\">\
        <input type=\"submit\" value=\"" + redButton + "\">\
      </form>\
      <h1>GREEN Light</h1>\
      <form method=\"post\" action=\"" + greenFormAction + "\">\
        <input type=\"submit\" value=\"" + greenButton + "\">\
      </form>\
    </body>\
  </html>";
}

void handleRoot() {
  digitalWrite(led, 1);
  String content = renderPage();
  server.send(200, "text/html", content);
  digitalWrite(led, 0);
}

void handlePlain() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "POST body was:\n" + server.arg("plain"));
    digitalWrite(led, 0);
  }
}

void handleForm() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(200, "text/plain", message);
    digitalWrite(led, 0);
  }
}

void handleNotFound() {
  digitalWrite(led, 1);
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
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handlePostLightControl(int light, boolean newState){
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    
    switch(light){
      case RED_LIGHT: _redLit = newState; break;
      case GREEN_LIGHT: _greenLit = newState; break;
    }
    
    digitalWrite(light, newState);
    Serial.println(String(light) + " is now "+ String(newState));

    String content = renderPage();
    server.send(200, "text/html", content);

  }
}

void redOn(){
  handlePostLightControl(RED_LIGHT, HIGH);
}

void greenOn(){
  handlePostLightControl(GREEN_LIGHT, HIGH);
}

void redOff(){
  handlePostLightControl(RED_LIGHT, LOW);
}

void greenOff(){
  handlePostLightControl(GREEN_LIGHT, LOW);
}

void setup(void) {
  pinMode(RED_LIGHT, OUTPUT);
  pinMode(GREEN_LIGHT, OUTPUT);
  
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.hostname(HOST_NAME);

  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/red/on", redOn);
  server.on("/red/off", redOff);
  server.on("/green/on", greenOn);
  server.on("/green/off", greenOff);
  
  server.on("/postplain/", handlePlain);

  server.on("/postform/", handleForm);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}
