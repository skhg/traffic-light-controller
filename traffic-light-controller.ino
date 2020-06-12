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

const String HOST_NAME = "traffic-light";
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const String _falseString = "false";
const String _trueString = "true";

ESP8266WebServer server(80);
StaticJsonDocument<200> doc;

boolean _redLit = false;
boolean _greenLit = false;
int _bpm = 100;
boolean _partyOn = false;
unsigned long _currentMillis = 0;
unsigned long _beatStartMillis = 0;
int _rhythmStep = 0;

// Minified content of web.html - minified using http://minifycode.com/html-minifier/
const String STATIC_HTML_PAGE = "<html><head><title>Traffic Light</title><link href='https://fonts.googleapis.com/css2?family=IBM+Plex+Sans:ital,wght@1,700&display=swap' rel='stylesheet'><style>html,body,.grid-container{height:100%;margin:0}.grid-container{background-color:#000;display:grid;grid-template-columns:1fr;grid-template-rows:1fr 1fr 1fr 1fr;gap:0px 0px;grid-template-areas:'red' 'red' 'green' 'green'}.dark{opacity:25%}#red{grid-area:red;background-color:#b00000}#green{grid-area:green;background-color:#03b000}.button{width:100%;height:100%;opacity:0%}#circle-container{position:absolute;top:50%;left:50%;-moz-transform:translateX(-50%) translateY(-50%);-webkit-transform:translateX(-50%) translateY(-50%);transform:translateX(-50%) translateY(-50%)}#circle{font-family:'IBM Plex Sans',sans-serif;width:600px;height:600px;border-radius:50%;font-size:100px;line-height:600px;text-align:center}.noParty{background:#111;color:#666}.party{color:#000;background:#fff;-webkit-animation:rotation 2s infinite linear}@-webkit-keyframes rotation{from{-webkit-transform:rotate(0deg)}to{-webkit-transform:rotate(359deg)}}</style> <script>var serverAddr='';var greenOn=true;var redOn=true;var refreshQuery=new XMLHttpRequest();var isParty=false;var bpm=100;function setupLoop(){setInterval(refreshState,500);};function updateColourBlocks(){var circleDiv=document.getElementById('circle');var greenDiv=document.getElementById('green');var redDiv=document.getElementById('red');if(greenOn){greenDiv.className='green';}else{greenDiv.className='green dark';} if(redOn){redDiv.className='red';}else{redDiv.className='red dark';} if(isParty){circleDiv.className='party';circleDiv.innerHTML=''+bpm+' BPM';}else{circleDiv.className='noParty';circleDiv.innerHTML='Party Mode';}};function refreshState(){refreshQuery=new XMLHttpRequest();refreshQuery.open('GET',serverAddr+'/status',true);refreshQuery.onreadystatechange=function(){if(refreshQuery.readyState==XMLHttpRequest.DONE){if(refreshQuery.status==200){var result=JSON.parse(refreshQuery.responseText);greenOn=result.green;redOn=result.red;isParty=result.party;bpm=result.bpm;updateColourBlocks();}}};refreshQuery.send();};function green(){refreshQuery.abort();var xhr=new XMLHttpRequest();var cmd='off';if(greenOn){cmd='off';}else{cmd='on';} xhr.open('POST',serverAddr+'/green/'+cmd,true);xhr.send();greenOn=!greenOn;updateColourBlocks();};function red(){refreshQuery.abort();var xhr=new XMLHttpRequest();var cmd='off';if(redOn){cmd='off';}else{cmd='on';} xhr.open('POST',serverAddr+'/red/'+cmd,true);xhr.send();redOn=!redOn;updateColourBlocks();};function party(){refreshQuery.abort();if(isParty){var xhr=new XMLHttpRequest();xhr.open('DELETE',serverAddr+'/party',true);xhr.send();}else{var xhr=new XMLHttpRequest();xhr.open('POST',serverAddr+'/party',true);xhr.send();} isParty=!isParty;updateColourBlocks();};</script> </head><body onload='setupLoop()'><div class='grid-container'><div id='red' class='dark' onclick='red()'></div><div id='green' class='dark' onclick='green()'></div></div><div id='circle-container'><div id='circle' class='noParty' onclick='party()'>Party Mode</div></div></body></html>";

const int RHYTHM_STEPS = 8;
const int RHYTHM_PATTERN[] = {RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, RED_FLASH, GREEN_FLASH, BOTH_FLASH, BOTH_FLASH};

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
  server.send(200, "text/html", STATIC_HTML_PAGE);
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

void lightSwitch(int light, boolean newState){
  switch(light){
    case RED_LIGHT: _redLit = newState; break;
    case GREEN_LIGHT: _greenLit = newState; break;
  }
    
  digitalWrite(light, !newState); // because LOW means ON
}

void handlePostLightControl(int light, boolean newState){
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    lightSwitch(light, newState);
    server.send(204, "text/html", "");
  }
}

void controlParty(){
  if(server.method() == HTTP_POST) {
    server.send(200, "text/plain", "Party On!");
    Serial.println("Party started");
    _partyOn = true;
  } else if (server.method() == HTTP_DELETE) {
    server.send(200, "text/plain", "Party's Over");
    Serial.println("Party ended");
    _partyOn = false;
  }else{
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

String clientIP(){
  return server.client().remoteIP().toString();
}

void lightStatus(){
  if (server.method() != HTTP_GET) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String content = "\
    {\
      \"red\" : " + String(_redLit ? _trueString : _falseString) + ",\
      \"green\" : " + String(_greenLit ? _trueString : _falseString) + ",\
      \"party\" : " + String(_partyOn ? _trueString : _falseString) + ",\
      \"bpm\" : " + String(_bpm) + "\
    }";
    
    server.send(200, "application/json", content);

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

void setTempo(){
  Serial.println("setTempo");
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  if(server.hasArg("plain") == false) {
    server.send(400, "text/plain", "Missing body");
    return;
  }
    
  deserializeJson(doc, server.arg("plain"));
  _bpm = doc["bpm"];

  Serial.println("BPM is now: " + String(_bpm));
  
  server.send(204, "text/html", "");
}

void setup(void) {
  _currentMillis = millis();
  
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

  server.on("/", handleRoot);

  server.on("/red/on", redOn);
  server.on("/red/off", redOff);
  server.on("/green/on", greenOn);
  server.on("/green/off", greenOff);
  server.on("/tempo", setTempo);
  server.on("/party", controlParty);
  server.on("/status", lightStatus);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  rhythm();
  
  if(_partyOn){
    partyFlash();
  }
}
