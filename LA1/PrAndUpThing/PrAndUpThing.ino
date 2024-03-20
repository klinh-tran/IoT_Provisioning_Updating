// PrAndUpThing.ino
#include "PrAndUpThing.h"

const int GREEN_LED_PIN = 9;
const int RED_LED_PIN = 6;

// the setup function runs once when you press reset or power the board
void setup() {
  // begin serial for writing to console
  Serial.begin(115200);
  delay(5000);

  setupAP();

  setupServer();

  // Setup pins for Wifi Status Lights
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
}


// the loop function runs over and over again forever
void loop() {
  // Activate LEDs based on wifi status
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
  } else {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
  }

  // Handle webserver connections
  webServer.handleClient();

  // TODO: Once connected to wifi allow for update over the air
  // implement function which checks for user input then runs firmware update check
  // include more led pins to show the progress of update similar to how progress bar is 
  // done in lab exercise
}
