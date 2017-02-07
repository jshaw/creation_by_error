#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ArduinoOTA.h>

#include <Servo.h>
#include <NewPingESP8266.h>
#include "settings.h"
//
#include "ESPAsyncTCP.h"
#include "SyncClient.h"

#define TRIGGER_PIN   12 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN      13 // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPingESP8266 sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

Servo myservo;  // create servo object to control a servo

unsigned int pingSpeed = 33; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer;     // Holds the next ping time.
int pos = 0;    // variable to store the servo position
int increment = 2;

unsigned long movementTimer;     
unsigned int movementSpeed = 40;

unsigned long requestTimer;     // Holds the next ping time.
unsigned int requestSpeed = 10000; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.

int value = 0;

SyncClient client;

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.printf("WiFi Connected!\n");
  Serial.println(WiFi.localIP());
//  #ifdef ESP8266
//    ArduinoOTA.begin();
//  #endif
  
//  SyncClient client;
  const int l_httpPort = 80;
  if(!client.connect(g_host, l_httpPort)){
    Serial.println("Connect Failed");
    Serial.println("Pubnub Connection Failed");
    return;
  }
//  client.setTimeout(2);
//  if(client.printf("GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n") > 0){
//    while(client.connected() && client.available() == 0){
//      delay(1);
//    }
//    while(client.available()){
//      Serial.write(client.read());
//    }
//    if(client.connected()){
//      client.stop();
//    }
//  } else {
//    client.stop();
//    Serial.println("Send Failed");
//    while(client.connected()) delay(0);
//  }
  
  pingTimer = millis(); // Start now.
  myservo.attach(14);  // attaches the servo on pin 9 to the servo object
}

void loop() {
  
//  WiFiClient client;
//  SyncClient client;
  const int l_httpPort = 80;
  if (!client.connect(g_host, l_httpPort)) {
    Serial.println("Pubnub Connection Failed");
    return;
  }
  
  // Notice how there's no delays in this sketch to allow you to do other processing in-line while doing distance pings.
  if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
    pingTimer += pingSpeed;      // Set the next ping time.
    Serial.print("Ping: ");
    Serial.print(sonar.ping_cm()); // Send ping, get distance in cm and print result (0 = outside set distance range)
    Serial.println("cm");
  }

  if((millis() - movementTimer) > movementSpeed)  // time to update
  {
    movementTimer = millis();
    pos += increment;
    myservo.write(pos);
    if ((pos >= 170) || (pos <= 0))
    {
      myservo.detach();
      myservo.attach(14);
      increment = -increment;
    }
  }

  if((millis() - requestTimer) > requestSpeed)
  {
    requestTimer = millis();
    // Sending data to pubnub
    // ============
    //DATA FORMAT : http://pubsub.pubnub.com /publish/pub-key/sub-key/signature/channel/callback/message
  
    String stringOne = "{\"num\":";
    stringOne += value;
    stringOne += ", \"txt\":\"Yo!\"}";
    char charBuf[200];
    stringOne.toCharArray(charBuf, 200);
  
    String json = "{\"text\":" + String(value) + "}";
    
    String url = "/publish/";
    url += g_pubKey;
    url += "/";
    url += g_subKey;
    url += "/0/";
    url += g_channel;
    url += "/0/";
    url += urlencode(json);
    
    Serial.println(url);

//    if (!client.connect(g_host, l_httpPort)) {
//      Serial.println("Connect Failed");
//      return;
//    }
    //client.setTimeout(2);
//    if(client.printf("GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n") > 0){
    if(client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + g_host + "\r\n" + "Connection: keep-alive\r\n\r\n") > 0){
      Serial.println(client.available());
      Serial.println(client.connected());
      while(client.connected() && client.available() == 0){
        delay(1);
        Serial.print("---");
      }
      while(client.available()){
        Serial.write(client.read());
      }
      // should probably have a connection close feature
//      if(client.connected()){
//       // client.stop();
//       Serial.println("Where STOP USED TO BE");
//      }
    } else {
      client.stop();
      Serial.println("Send Failed");
      while(client.connected()) delay(0);
    }
    
//    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//               "Host: " + g_host + "\r\n" + 
//               "Connection: close\r\n\r\n");
//    delay(10);
//    
//    while(client.available()){
//      String line = client.readStringUntil('\r');
//      Serial.print(line);
//    }

    Serial.println();
//    Serial.println("Pubnub Connection Closed");
  
    value++;
  }

//  #ifdef ESP8266
//    ArduinoOTA.handle();
//  #endif
  
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}
