#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
Ticker ticker;
boolean ticker_reached;
boolean LED_state;
void ticker_handler(){
 ticker_reached = true;
}
#define USE_SERIAL Serial
ESP8266WiFiMulti WiFiMulti;
SoftwareSerial swser(4,5);
void setup() {
   pinMode(LED_BUILTIN, OUTPUT);

   ticker_reached = false;
   LED_state = HIGH;
   //call ticker_handler() in 5 second
   ticker.attach(5, ticker_handler);
   swser.begin(9600);
   USE_SERIAL.begin(9600);
   // USE_SERIAL.setDebugOutput(true);
   USE_SERIAL.println();
   USE_SERIAL.println();
   USE_SERIAL.println();
   for(uint8_t t = 4; t > 0; t--) {
   USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
   USE_SERIAL.flush();
   delay(1000);
   }
   WiFiMulti.addAP("hari", "harijulu");
}
void loop() {
   if(ticker_reached){
   ticker_reached = false;
   digitalWrite(LED_BUILTIN, LED_state);
   LED_state = !LED_state;

   if((WiFiMulti.run() == WL_CONNECTED)) {
   HTTPClient http;

  http.begin("api.thingspeak.com",80,"/apps/thinghttp/send_request?api_key=DM6CQ6
  7HSJ39LF5Y"); //API
  int httpCode = http.GET();

   if(httpCode > 0) {
   if(httpCode == HTTP_CODE_OK) {
   String payload = http.getString();
   USE_SERIAL.println("Epoch Time Received, Sending data...");
   for(int i=0;i<10;i++)
   {
   swser.print(payload[i]);
  delay(50);
   }
   }
   }
   else {
   USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n",
  http.errorToString(httpCode).c_str());
   }
   http.end();
   }
   delay(500);
   }
}
