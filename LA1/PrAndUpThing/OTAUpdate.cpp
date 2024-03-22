#include "PrAndUpThing.h"

const char *boilerUpdate[]{
    "<html><head><title>",                                                       // 0
    "default title",                                                             // 1
    "</title>\n",                                                                // 2
    "<meta charset='utf-8'>",                                                    // 3

    "<meta name='viewport' content='width=device-width, inital-scale=1.0'>\n"
    "<style>body{background:#FFF; color:#000, font-family: sans-serif;",         // 4

    "font-size: 150%;}</style>\n",                                               // 5
    "</head><body>\n",                                                           // 6
    "<h2>Welcome!</h2>\n",                                                       // 7
    "<!-- page payload goes here... -->\n",                                      // 8
    "<!-- ...and/or here... -->\n",                                              // 9
    "<!-- ...and/or here... -->\n",                                              // 10
    "</body></html>\n\n",                                                        // 11
};

int pinArray[] = {10, 11, 12, 13}; // From Furthest to Closest connected light
const int greenProgressLight = 15; // Set to pin green update light is connected to
const int redProgressLight = 14; // Set to pin red update light is connected to

void setupOTAUpdate() {
  for (int i=0; i < 4; i++){
    pinMode(pinArray[i], OUTPUT); // Yellow Progress lights
  }

  pinMode(redProgressLight, OUTPUT); // RED Update Light
  pinMode(greenProgressLight, OUTPUT); // Green Update Light
}

/// Updating ///
// function to handle OTA Update process
void handleUpdate() {
  Serial.println("serving page at /update");
  replacement_t repls[] = { // the elements to replace in the boilerplate
    { 1, apSSID.c_str() },
    {7, "<h2>Updates</h2>"},
    {8, "<p> Enter server URL: </p><form action = /setupOTA>"},
    {9, "<input type='text' id='server_url' name='server_url' required>\n"
      "<input type='submit'>\n</form>\n"},
  };
  
  String toSend = "";
  getHtml(toSend, boilerUpdate, ALEN(boilerUpdate), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

// Handles HTTP Get request to user defined web server
void OTAUpdate() {
  digitalWrite(greenProgressLight, LOW);
  digitalWrite(redProgressLight, LOW);
  
  Serial.println("serving page /setupOTA");
  // materials for doing an HTTPS GET on github from the BinFiles/ dir
  #define FIRMWARE_SERVER_IP_ADDR webServer.arg(0)
  #define FIRMWARE_SERVER_PORT    "8000"
  
  String toSend = "";
  String updateStatus = "";
  bool isUpdated = false;
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
  else {
    updateStatus = "<p>Fail to get version!</p>";
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);
  }

  // do we know the latest version, and does the firmware need updating?
  if(respCode < 0) {
    updateStatus = "<p>Fail to get version!</p>";
  } else if(firmwareVersion >= highestAvailableVersion) {
    Serial.printf("firmware is up to date\n");
    updateStatus = "<p>Firmware is up to date</p>";
    replacement_t repls[] = {
      {1, apSSID.c_str()},
      {7, "<h2>Checking Updates</h2>"},
      {8, updateStatus.c_str()},
      {9, ""},
      {10,"<p><a href='/'>Home</a>.</p>"},
    };

    // Generate webpage and send to connected  client
    getHtml(toSend, boilerUpdate, ALEN(boilerUpdate), repls, ALEN(repls));
    webServer.send(200, "text/html", toSend);
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
  delay(300); // settle down
  
  // possible improvement: if size is improbably big or small, refuse
  if(respCode > 0 && respCode != 404) { // check response code (-ve on failure)
    Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
  } else {
    updateStatus =  "<p>Failed to get .bin!</p>";
    Serial.printf("failed to get .bin! return code is: %d\n", respCode);
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
        updateStatus = "<p>Update successfully finished; please reboot...</p>";
        Serial.printf("update successfully finished\n\n");
        digitalWrite(greenProgressLight, HIGH);
        isUpdated = true;
      } else {
        updateStatus = "<p>Fail to update...; please check again :(</p>";
        Serial.printf("update didn't finish correctly :(\n");
        digitalWrite(redProgressLight, HIGH);
        Serial.flush();
      }
    } else {
      updateStatus = "<p>An update error occurred :(</p>";
      Serial.printf("an update error occurred, #: %d\n" + Update.getError());
      digitalWrite(redProgressLight, HIGH);
      Serial.flush();
    }
  } else {
    updateStatus = "<p>Not enough space to start OTA update :(</p>";
    Serial.printf("not enough space to start OTA update :(\n");
    digitalWrite(redProgressLight, HIGH);
    Serial.flush();
  }
  stream.flush();

  for (int i=0; i<4; i++){
    digitalWrite(pinArray[i], LOW);
  }

  // post-OTA stage
  replacement_t repls[] = {
    {1, apSSID.c_str()},
    {7, "<h2>Checking Updates</h2>"},
    {8, updateStatus.c_str()},
    {9, ""},
    {10,"<p><a href='/'>Home</a>.</p>"},
  };

  // Generate webpage and send to connected  client
  getHtml(toSend, boilerUpdate, ALEN(boilerUpdate), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
  delay(5000);

  digitalWrite(greenProgressLight, LOW);
  digitalWrite(redProgressLight, LOW);

  if(isUpdated == true) {
    Serial.printf("rebooting...\n\n");
    ESP.restart();
  }
}

// helper for downloading from cloud firmware server; for experimental
// purposes just use a hard-coded IP address and port (defined above)
int doCloudGet(HTTPClient *http, String fileName) {
  // build up URL from components
  String url =
    String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" +
    FIRMWARE_SERVER_PORT + "/" + fileName;
  Serial.printf("getting %s\n", url.c_str());

  // make GET request and return the response code
  http->begin(url);
  http->addHeader("User-Agent", "ESP32");
  return http->GET();
}

// Handles displaying progress as leds in progress bar
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float) done / (float) total;

  int lightCount = int(progress*100) / 25;
  lightCount += 1;
  
  for (int i = 0; i<lightCount; i++) {
    digitalWrite(pinArray[i], HIGH);
  }

}
