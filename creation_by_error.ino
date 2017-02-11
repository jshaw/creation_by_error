#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <Servo.h>
#include <NewPingESP8266.h>
#include <SimplexNoise.h>
#include "settings.h"
//
#include "ESPAsyncTCP.h"
#include "SyncClient.h"

#define TRIGGER_PIN   12 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN      13 // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPingESP8266 sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
Servo myservo;  // create servo object to control a servo
SyncClient client;

const int buttonPin = 2;
int buttonState = HIGH;             
// the previous reading from the input pin
int lastButtonState = HIGH;

int min_degree = 0;
int max_degree = 0;
int buttonPushCounter = 0;

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned int pingSpeed = 33; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer;     // Holds the next ping time.
int pos = 0;    // variable to store the servo position
int increment = 2;

unsigned long movementTimer;     
unsigned int movementSpeed = 40;

unsigned long requestTimer;     // Holds the next ping time.
unsigned int requestSpeed = 10000; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.

int value = 0;

unsigned long previousMillis = 0;
const long interval = 50;

double n;
float increase = 0.01;
float x = 0.0;
float y = 0.0;
//int pos;
int minAngle = 0;
int maxAngle = 180;

class Sweeper
{
  Servo servo;              // the servo
  SimplexNoise sn;
  int id;
  int pos;              // current servo position 
  int increment;        // increment to move for each interval
  int  updateInterval;      // interval between updates
  unsigned long lastUpdate; // last update of position
  String sweepString = "";

  // number of pings collected
  unsigned long pingTotalCount = 0;
  // number of pings before send for simplexNoise
  unsigned long pingRemainderValue = 50;
  
  NewPingESP8266 *sonar;
  int currentDistance;

  boolean publish_data = false;

  // =============
  // these two vars are pure debug variels to control what gets sent over serial
  boolean sendJSON = true;
  boolean storeDataJSON = true;
  boolean printStringTitle = false;
  // =============
   
public: 
  Sweeper(int ide, int interval, NewPingESP8266 &sonar, int position)
  {
    updateInterval = interval;
    // can be 1,2,3,4 etc...
    increment = 2;
    id = constrain(ide, 0, 13);
    pos = position;
  }
  
  void Attach(int pin)
  {
    if(servo.attached() == 0){
      servo.attach(pin); 
    }
  }
  
  void Detach()
  {
    servo.detach();
  }

  void SetPos(int startPosition)
  {
    pos = startPosition;
    servo.write(pos);
  }

  int isAttached()
  {
    return servo.attached();
  }

  void SetDistance(int d)
  {
    currentDistance = d;

    if(storeDataJSON == true){
      StoreData(currentDistance);
    }
  }

  boolean GetPublishDataStatus()
  {
    return publish_data;
  }

  String GetPublishData()
  {
    return sweepString;
  }

  void ResetPublishDataStatus()
  {
    publish_data = false;
    sweepString = "";
  }

  void SendBatchData() {
    // helping debug the serial buffer issue
    if(sendJSON == true){
        if(sweepString.endsWith("/")){
          int char_index = sweepString.lastIndexOf("/");
          sweepString.remove(char_index);
        }

        if(printStringTitle == true){
          Serial.println("");
          Serial.print("sweepString: ");
        }
        Serial.println(sweepString);
        publish_data = true;
    }
  }

  void StoreData(int currentDistance)
  {
    if(printStringTitle == true){
      if(String(currentDistance).length() > 0){
        Serial.print("currentDistance: ");
        Serial.print((String)currentDistance);
        Serial.print(" ||||");
        Serial.println(" ");
      }
    }

    if(String(currentDistance).length() > 0){
      String tmp = String(id);
      tmp.concat(":");
      tmp.concat(String(pos));
      tmp.concat(":");
      tmp.concat(String(currentDistance));
      
      sweepString.concat(tmp);
      sweepString.concat("/");

      pingTotalCount++;
    }
  }
  
  void Update()
  {

    if (pos == -1) {
      pos = 0;
      servo.write(pos);
    }

    if((millis() - lastUpdate) > updateInterval)  // time to update
    {
      lastUpdate = millis();

      if (buttonPushCounter == 1){
        min_degree = 0;
        max_degree = 170;
        pos += increment;
      } else {
        min_degree = 15;
        max_degree = 155;

        n = sn.noise(x, y);
        x += increase;
  
        pos = (int)map(n*100, -100, 100, minAngle, maxAngle);
      }
      
      servo.write(pos);

      if (buttonPushCounter == 1){
      
        if ((pos >= max_degree) || (pos <= min_degree)) // end of sweep
        {
          // send data through serial here
          SendBatchData();
          Detach();
          Attach(9);
          // reverse direction
          increment = -increment;
        }
      } else {
        // Send the ping data readings on every nth count
        if(pingTotalCount % pingRemainderValue == 0){
          SendBatchData();
        }
      }
    }
  }
};

Sweeper sweeper(0, 40, sonar, 0);

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.

  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  
  Serial.printf("WiFi Connected!\n");
  Serial.println(WiFi.localIP());

  const int l_httpPort = 80;
  if(!client.connect(g_host, l_httpPort)){
    Serial.println("Connect Failed");
    Serial.println("Pubnub Connection Failed");
    return;
  }
  
  pingTimer = millis(); // Start now.

  sweeper.Attach(14);
  sweeper.SetPos(90);
}

int measurement = 0;
String JSONString = "";

void loop() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      Serial.println("*********************");
      Serial.println(buttonState);
      if (buttonState == HIGH) {
        buttonPushCounter++;

        // this makes sure that it runs through 0,1,2 states
        if(buttonPushCounter == 3){
          buttonPushCounter = 0;
        }

        Serial.print("buttonPushCounter: ");
        Serial.print(buttonPushCounter);
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;

  if(buttonPushCounter != 0){
    // update
    if(sweeper.isAttached() == true){
      sweeper.Update();
    } else {
      sweeper.Attach(9);
      sweeper.Update();  
    }
  } else {
    // check to make make sure the server is attached before detaching
    if(sweeper.isAttached() == true){
      sweeper.Detach();  
    }
  }

  if(buttonPushCounter != 0){
    // Notice how there's no delays in this sketch to allow you to do other processing in-line while doing distance pings.
    if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
      pingTimer += pingSpeed;      // Set the next ping time.
      Serial.print("Ping: ");
      measurement = sonar.ping_cm();
      sweeper.SetDistance(measurement);
      Serial.print(measurement); // Send ping, get distance in cm and print result (0 = outside set distance range)
      Serial.println("cm");
    }
  }

  if(sweeper.GetPublishDataStatus() == true)
  {
    // Sending data to pubnub
    // ============
    //DATA FORMAT : http://pubsub.pubnub.com /publish/pub-key/sub-key/signature/channel/callback/message
  
    String to_publish = sweeper.GetPublishData();
    String json = String(to_publish);
    
    String url = "/publish/";
    url += g_pubKey;
    url += "/";
    url += g_subKey;
    url += "/0/";
    url += g_channel;
    url += "/0/";
    url += "\"" + String(to_publish) + "\"";
    
    Serial.println(url);

    //client.setTimeout(2);
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

    Serial.println();
    sweeper.ResetPublishDataStatus();
  
    value++;
  }
  
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
