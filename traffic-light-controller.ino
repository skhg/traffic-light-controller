#define RED_LIGHT D5
#define GREEN_LIGHT D6

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <home_wifi.h>

const String HOST_NAME = "traffic-light";
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);

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
      <p>Relay 1</p>\
      <form method=\"post\" action=\"" + redFormAction + "\">\
        <input type=\"submit\" value=\"" + redButton + "\">\
      </form>\
      <h1>GREEN Light</h1>\
      <p>Relay 2</p>\
      <form method=\"post\" action=\"" + greenFormAction + "\">\
        <input type=\"submit\" value=\"" + greenButton + "\">\
      </form>\
    </body>\
  </html>";
}

void handleRoot() {
  String content = renderPage();
  server.send(200, "text/html", content);
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
  server.send(404, "text/plain", message);
}

void handlePostLightControl(int light, boolean newState){
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    
    switch(light){
      case RED_LIGHT: _redLit = newState; break;
      case GREEN_LIGHT: _greenLit = newState; break;
    }
    
    digitalWrite(light, !newState); // because LOW means ON
    Serial.println(String(light) + " is now "+ String(newState));

    String content = renderPage();
    server.send(200, "text/html", content);

  }
}

void redOn(){
  handlePostLightControl(RED_LIGHT, true);
}

void greenOn(){
  handlePostLightControl(GREEN_LIGHT, true);
}

void redOff(){
  handlePostLightControl(RED_LIGHT, false);
}

void greenOff(){
  handlePostLightControl(GREEN_LIGHT, false);
}

void setup(void) {
  homekit_server_reset();
  
  pinMode(RED_LIGHT, OUTPUT);
  pinMode(GREEN_LIGHT, OUTPUT);

  digitalWrite(RED_LIGHT, HIGH);
  digitalWrite(GREEN_LIGHT, HIGH);
  
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
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  my_homekit_setup();
}

void loop(void) {
  server.handleClient();
  my_homekit_loop();
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_switch_on;

static uint32_t next_heap_millis = 0;

#define PIN_SWITCH 2

//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
  bool on = value.bool_value;
  cha_switch_on.value.bool_value = on;  //sync the value
  LOG_D("Switch: %s", on ? "ON" : "OFF");
  digitalWrite(PIN_SWITCH, on ? LOW : HIGH);
}

void my_homekit_setup() {
  pinMode(PIN_SWITCH, OUTPUT);
  digitalWrite(PIN_SWITCH, HIGH);

  //Add the .setter function to get the switch-event sent from iOS Home APP.
  //The .setter should be added before arduino_homekit_setup.
  //HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
  //Maybe this is a legacy design issue in the original esp-homekit library,
  //and I have no reason to modify this "feature".
  cha_switch_on.setter = cha_switch_on_setter;
  arduino_homekit_setup(&config);

  //report the switch value to HomeKit if it is changed (e.g. by a physical button)
  //bool switch_is_on = true/false;
  //cha_switch_on.value.bool_value = switch_is_on;
  //homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_heap_millis) {
    // show heap info every 5 seconds
    next_heap_millis = t + 5 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

  }
}
