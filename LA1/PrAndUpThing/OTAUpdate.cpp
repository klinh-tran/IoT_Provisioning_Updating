#include "PrAndUpThing.h"

const char *otaForm[]{
    "<html><head><title>",    // 0
    "default title",          // 1
    "</title>\n",             // 2
    "<meta charset='utf-8'>", // 3

    "<meta name='viewport' content='width=device-width, inital-scale=1.0'>\n"
    "<style>body{background:#FFF; color:#000, font-family: sans-serif;", // 4

    "font-size: 150%;}</style>\n",          // 5
    "</head><body>\n",                      // 6
    "<h2>Welcome!</h2>\n",                  // 7
    "<!-- page payload goes here... -->\n", // 8
    "<!-- ...and/or here... -->\n",         // 9
    "<!-- ...and/or here... -->\n",         // 10
    "</body></html>\n\n",                   // 11
};

/// Updating ///
// IP address and port number: CHANGE THE IP ADDRESS!
void setupOTA() {
  webServer.on("/update", handleUpdate);
  webServer.on("/setupOTA", setupUpdate);
}

void handleUpdate() {
  Serial.println("serving page at /update");
  replacement_t repls[] = {
      // the elements to replace in the boilerplate
      {1, apSSID.c_str()},
      {7, "<h2>Updates</h2>"},
      {8, "<p> Enter server URL: </p><form action = /setupOTA>"},
      {9, "<input type='text' id='server_url' name='server_url' required>\n"
          "<input type='submit'>\n</form>\n"},
  };

  String toSend = "";
  getHtml(toSend, otaForm, ALEN(otaForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

void setupUpdate() {
  // materials for doing an HTTPS GET on github from the BinFiles/ dir
  Serial.print("user input: ");
  Serial.println(webServer.arg(0));
#define FIRMWARE_SERVER_IP_ADDR webServer.arg(0)
#define FIRMWARE_SERVER_PORT "8000"

  HTTPClient http; // HTTPClient is a class used to make HTTP requests.
  int respCode;    // store the response code from the server
  int highestAvailableVersion = -1;

  // read the version file from the cloud
  respCode = doCloudGet(&http, "version.txt");
  Serial.print("respCode: ");
  Serial.println(respCode);
  if (respCode > 0) { // check response code (-ve on failure)
    highestAvailableVersion = atoi(http.getString().c_str());
    Serial.print("Highest available version: ");
    Serial.println(highestAvailableVersion);
  } else
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);
  http.end(); // free resources

  // do we know the latest version, and does the firmware need updating?
  if (respCode < 0) {
    return;
  } else if (firmwareVersion >= highestAvailableVersion) {
    Serial.printf("firmware is up to date\n");
    return;
  }

  // do a firmware update
  Serial.printf("upgrading firmware from version %d to version %d\n",
                firmwareVersion, highestAvailableVersion);

  // do a GET for the .bin, e.g. "23.bin" when "version.txt" contains 23
  String binName = String(highestAvailableVersion);
  binName += ".bin";
  respCode = doCloudGet(&http, binName);
  int updateLength = http.getSize();

  // possible improvement: if size is improbably big or small, refuse
  if (respCode > 0 && respCode != 404) { // check response code (-ve on failure)
    Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
  } else {
    Serial.printf("failed to get .bin! return code is: %d\n", respCode);
    http.end(); // free resources
    return;
  }

  // write the new version of the firmware to flash
  WiFiClient stream = http.getStream();
  Update.onProgress(handleOTAProgress); // print out progress
  if (Update.begin(updateLength)) {
    Serial.printf("starting OTA may take a minute or two...\n");
    Update.writeStream(stream);
    if (Update.end()) {
      Serial.printf("update done, now finishing...\n");
      Serial.flush();
      if (Update.isFinished()) {
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

// helper for downloading from cloud firmware server; for experimental
// purposes just use a hard-coded IP address and port (defined above)
int doCloudGet(HTTPClient *http, String fileName) {
  // build up URL from components
  String url = String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" +
               FIRMWARE_SERVER_PORT + "/" + fileName;
  Serial.printf("getting %s\n", url.c_str());

  // make GET request and return the response code
  http->begin(url);
  http->addHeader("User-Agent", "ESP32");
  return http->GET();
}

// callback handler for tracking OTA progress ///////////////////////////////
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float)done / (float)total;
  // dbf(otaDBG, "OTA written %d of %d, progress = %f\n", done, total,
  // progress);

  // TODO: Change from bar printed to serial to set of leds on breadboard
  int barWidth = 70;
  Serial.printf("[");
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos)
      Serial.printf("=");
    else if (i == pos)
      Serial.printf(">");
    else
      Serial.printf(" ");
  }
  Serial.printf("] %d %%%c", int(progress * 100.0),
                (progress == 1.0) ? '\n' : '\r');
  // Serial.flush();
}

String ip2str(IPAddress address) { // utility for printing IP addresses
  return String(address[0]) + "." + String(address[1]) + "." +
         String(address[2]) + "." + String(address[3]);
}
