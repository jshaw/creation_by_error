#include <ESP8266WiFi.h>
#include <Servo.h>
#include <NewPingESP8266.h>

#define TRIGGER_PIN   12 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN      13 // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPingESP8266 sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

Servo myservo;  // create servo object to control a servo

unsigned int pingSpeed = 33; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer;     // Holds the next ping time.
int pos = 0;    // variable to store the servo position
int increment = 2;

unsigned long movementTimer;     // Holds the next ping time.
unsigned int movementSpeed = 40; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pingTimer = millis(); // Start now.
  myservo.attach(14);  // attaches the servo on pin 9 to the servo object
}

void loop() {
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
}
