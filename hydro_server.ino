#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "Wire.h" // This library allows you to communicate with I2C devices.
#include <Ticker.h>

Ticker timer;
const int MPU=0x68; // I2C address of the MPU-6050
int16_t Tmp; // 16-bit integers
int tcal; // Calibration variables
double t,tx;

// Connecting to the Internet
char* ssid = "";
char* password = "";

// Running a web server
ESP8266WebServer server; 

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);

// Serving a web page (from flash memory)
// formatted as a string literal
char webpage[] PROGMEM = R"=====(
<html>
<body onload="javascript:init()">
<div>
<canvas id="line-chart" width="800" height="450"></canvas>
</div>
<!-- Adding a websocket to the client (webpage) -->
<script>
  let webSocket;
  function init() {
    webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
    webSocket.onmessage = function(event) {
      let data = JSON.parse(event.data);
      console.log(data);
    }
  }
</script>
</body>
</html>
)=====";

void setup()
{
    WiFi.begin(ssid,password);
    Serial.begin(115200);
    while(WiFi.status()!=WL_CONNECTED)
    {
      Serial.print(".");
      delay(500);
    }
    Serial.println("");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/",[]()
    {
       server.send_P(200, "text/html", webpage);
     });
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    
    Wire.begin(); // Initiate wire library and I2C
    Wire.beginTransmission(MPU); // Begins a transmission to the I2C slave (GY-521 board)
    Wire.write(0x6B); // PWR_MGMT_1 register
    Wire.write(0); // Set to zero (wakes up the MPU-6050)
    Wire.endTransmission(true);
    
    timer.attach(3, getData);
}

void loop()
{
     webSocket.loop();
     server.handleClient();
}

void getData() {
    Wire.beginTransmission(MPU); // Begin transmission to I2C slave device
    Wire.write(0x41); // Starting with register 0x41 (TEMP_OUT_L)
    Wire.endTransmission(false); // Restarts transmission to I2C slave device
    Wire.requestFrom(MPU,2,true); // Request 2 registers in total  

    // Temperature correction
    tcal = -1600;
  
    // Read temperature data 
    Tmp=Wire.read()<<8|Wire.read(); // 0x41 (TEMP_OUT_H) 0x42 (TEMP_OUT_L) 

    // Temperature calculation
    tx = Tmp + tcal;
    t = tx/340 + 36.53; // Equation for temperature in degrees C from datasheet
  
    // Printing values to serial port
    Serial.println(t); 

    String json = "{\"value\":";
    json += t;
    json += "}";
    webSocket.broadcastTXT(json.c_str(), json.length());
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    // Do something later
}
