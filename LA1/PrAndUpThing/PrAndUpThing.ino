// PrAndUpThing.ino
#include "PrAndUpThing.h"

const int GREEN_LED_PIN = 9;
const int RED_LED_PIN = 6;
int firmwareVersion = 4;

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
  
  ///////
  Serial.printf("running firmware is at version %d\n", firmwareVersion);

  // get on the network
  WiFi.begin(); // register MAC first! and add SSID/PSK details if needed
  uint16_t connectionTries = 0;
  
  WiFi.begin("klinhtranlara", "KHANHLINHTRAN10702");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if(connectionTries++ % 75 == 0) Serial.println("");
    delay(250);
  }
  delay(500); // let things settle for half a second

  // materials for doing an HTTPS GET on github from the BinFiles/ dir
  HTTPClient http;  // HTTPClient is a class used to make HTTP requests.
  int respCode;     // store the response code from the server
  int highestAvailableVersion = -1;

  // read the version file from the cloud
  respCode = doCloudGet(&http, "version.txt");
  Serial.print("respCode: ");
  Serial.println(respCode);
  if(respCode > 0) { // check response code (-ve on failure)
    highestAvailableVersion = atoi(http.getString().c_str());
    Serial.print("Highest available version: ");
    Serial.println(highestAvailableVersion);
  }
  else
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);
  http.end(); // free resources

  // do we know the latest version, and does the firmware need updating?
  if(respCode < 0) {
    return;
  } else if(firmwareVersion >= highestAvailableVersion) {
    Serial.printf("firmware is up to date\n");
    return;
  }

  // do a firmware update
  Serial.printf(
    "upgrading firmware from version %d to version %d\n",
    firmwareVersion, highestAvailableVersion
  );

  // do a GET for the .bin, e.g. "23.bin" when "version.txt" contains 23
  String binName = String(highestAvailableVersion);
  binName += ".bin";
  respCode = doCloudGet(&http, binName);
  int updateLength = http.getSize();

  // possible improvement: if size is improbably big or small, refuse
  if(respCode > 0 && respCode != 404) { // check response code (-ve on failure)
    Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
  } else {
    Serial.printf("failed to get .bin! return code is: %d\n", respCode);
    http.end(); // free resources
    return;
  }

  // write the new version of the firmware to flash
  WiFiClient stream = http.getStream();
  Update.onProgress(handleOTAProgress); // print out progress
  if(Update.begin(updateLength)) {
    Serial.printf("starting OTA may take a minute or two...\n");
    Update.writeStream(stream);
    if(Update.end()) {
      Serial.printf("update done, now finishing...\n");
      Serial.flush();
      if(Update.isFinished()) {
        Serial.printf("update successfully finished; rebooting...\n\n");
        ESP.restart();
      } else {
        Serial.printf("update didn't finish correctly :(\n");
        Serial.flush();
      }
    } else {
      Serial.printf("an update error occurred, #: %d\n" + Update.getError());
      Serial.flush();
    }
  } else {
    Serial.printf("not enough space to start OTA update :(\n");
    Serial.flush();
  }
  stream.flush();
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
