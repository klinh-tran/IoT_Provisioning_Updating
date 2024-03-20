#include "PrAndUpThing.h"

/// Provisioning ///
WebServer webServer(80);

// Boilerplate form to be used for selecting wifi network
const char *boilerForm[]{
    "<html><head><title>",    // 0
    "Connect to Network",     // 1
    "</title>\n",             // 2
    "<meta charset='utf-8'>", // 3

    "<meta name='viewport' content='width=device-width, inital-scale=1.0'>\n"
    "<style>body{background:#FFF; color:#000, font-family: sans-serif;", // 4

    "font-size: 150%;}</style>\n", // 5
    "</head><body>\n<p> Choose a network connection </p>\n<form "
    "action='/connect'>\n"
    "<select name='networks' id='networks'>", // 6
    "",                                       // 7
    "</select>\n"
    "<input type='password' id='pass' name='password' required>\n"
    "<input type='submit'>\n</form>\n", // 8
    "</body></html>\n\n",               // 9
};

String apSSID;

// Startup utilities
// TODO: ESP32 operates as both an AP and a STA. Notify if there is connection issue
void setupAP() {
  apSSID = "ESP32S3 Provisioning";

  // configuring wifi access point
  if(! WiFi.mode(WIFI_AP_STA))  
    Serial.println("failed to set Wifi mode");
  if(! WiFi.softAP(apSSID.c_str(), "provisioning")) 
    Serial.println("failed to start soft AP");
  
  Serial.print("AP SSID: ");
  Serial.println(apSSID);
  Serial.print("AP=");
  Serial.println(WiFi.softAPIP());
}

// Handling web server connections
// Open webpage on http://192.168.4.1/
// TODO: Make it so this page appears automatically on connection with DNS
void setupServer() {
  webServer.on("/", handleRoot);
  webServer.on("/connect", handleConnect);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
}

// Function handles a user connecting to root page
// Displays a list of available wifi networks and allows the user to attempt
// to connect with a password
void handleRoot() {
  Serial.println("seriving page at /");
  int n = WiFi.scanNetworks();
  String replacementString = "";

  // For loop creates wifi ssid options for html form dropdown
  for (int i = 0; i < n; i++) {
    replacementString.concat("<option value='");
    replacementString.concat(WiFi.SSID(i));
    replacementString.concat("'>");
    replacementString.concat(WiFi.SSID(i));
    replacementString.concat("</option>\n");
  };

  // convert to c style string and make sure that the new string is null
  // terminated
  char *char_array = new char[replacementString.length() + 1];
  char_array[replacementString.length()] = '\0';

  for (int i = 0; i < replacementString.length(); i++) {
    char_array[i] = replacementString[i];
  }

  // Set the replacement string to generated ssid options
  replacement_t repls[] = {
      {7, char_array},
  };

  // Generate webpage and send to connected  client
  String toSend = "";
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

// Function that attempts connection with user input
// TODO: Add some validation for user input?
void handleConnect() {
  Serial.println("serving page /connect");
  String ssid = webServer.arg(0);
  String password = webServer.arg(1);

  bool connecting = true;
  String connectionStatus = "";
  WiFi.begin(ssid, password);

  // Attempt to connect to given network
  while (connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      connectionStatus = "Connected";
      connecting = false;
    } else if (WiFi.status() == WL_CONNECT_FAILED) {
      connectionStatus = "Connection failed";
      connecting = false;
    } else if (WiFi.status() == .6) {
      connectionStatus = "No network with this SSID found";
      connecting = false;
    }
    delay(100);
  }

  setupOTA();

  // TODO: Change response to include button to take user back if they want to
  // try again.
  webServer.send(200, "text/plain", connectionStatus);
}

void handleNotFound() { webServer.send(404, "text/plain", "Page not found"); }

void getHtml(String &html, const char *boiler[], int boilerLen,
             replacement_t repls[], int replsLen) {
  for (int i = 0, j = 0; i < boilerLen; i++) {
    if (j < replsLen && repls[j].position == i)
      html.concat(repls[j++].replacement);
    else
      html.concat(boiler[i]);
  }
}

/// Updating ///
// IP address and port number: CHANGE THE IP ADDRESS!
#define FIRMWARE_SERVER_IP_ADDR "192.168.13.143"
#define FIRMWARE_SERVER_PORT    "8000"

void setupOTA() {
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

// callback handler for tracking OTA progress ///////////////////////////////
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float) done / (float) total;
  // dbf(otaDBG, "OTA written %d of %d, progress = %f\n", done, total, progress);

  int barWidth = 70;
  Serial.printf("[");
  int pos = barWidth * progress;
  for(int i = 0; i < barWidth; ++i) {
    if(i < pos)
      Serial.printf("=");
    else if(i == pos)
      Serial.printf(">");
    else
      Serial.printf(" ");
  }
  Serial.printf(
    "] %d %%%c", int(progress * 100.0), (progress == 1.0) ? '\n' : '\r'
  );
  // Serial.flush();
}