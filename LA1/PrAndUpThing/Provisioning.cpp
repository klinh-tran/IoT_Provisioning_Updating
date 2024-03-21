#include "PrAndUpThing.h"

/// Provisioning ///
WebServer webServer(80);

// Boilerplate form to be used for selecting wifi network
const char *boilerForm[]{
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

String apSSID;

int pinArray[] = {10, 11, 12, 13}; // From Furthest to Closest connected light
const int greenProgressLight = 15; // Set to pin green update light is connected to
const int redProgressLight = 14; // Set to pin red update light is connected to

// Startup utilities
void setupAP() {
  apSSID = "CAM ESP32S3 Provisioning";

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
  webServer.on("/wifi", handleWifi);
  webServer.on("/connect", handleConnect);
  webServer.on("/status", handleStatus);
  webServer.on("/update", handleUpdate);
  webServer.on("/setupOTA", setupOTA);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

  for (int i=0; i < 4; i++){
    pinMode(pinArray[i], OUTPUT); // Yellow Progress lights
  }

  pinMode(redProgressLight, OUTPUT); // RED Update Light
  pinMode(greenProgressLight, OUTPUT); // Green Update Light
}

// Function handles a user connecting to root page
void handleRoot() {
  Serial.println("serving page at /");
  replacement_t repls[] = { // the elements to replace in the boilerplate
    {  1, apSSID.c_str() },
    {  8, "<p>Choose a <a href=\"wifi\">wifi access point</a>.</p>" },
    {  9, "<p>Check <a href='/status'>wifi status</a>.</p>" },
    { 10, "<p>Check for <a href='/update'>updates</a>.</p>" },
  };
  
  String toSend = "";
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

// Function handles a user connecting to Wifi connection page
// Displays a list of available wifi networks and allows the user to attempt
// to connect with a password
void handleWifi() {
  Serial.println("serving page at /wifi");
  int n = WiFi.scanNetworks();
  String replacementString = "";

  replacementString.concat("<option value='uos-other'>uos-other</option>\n");

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
      {1, apSSID.c_str()},
      {7, "<h2>Connect to a WiFi</h2>"},
      {8, "<p> Choose a network connection </p>\n<form "
        "action='/connect'>\n"    
        "<select name='networks' id='networks'>"},
      {9, char_array},
      {10,"</select>\n"
        "<input type='password' id='pass' name='password' required>\n"
        "<input type='submit'>\n</form>\n"
        "<p><a href='/'>Home</a>.</p>"},
  };

  // Generate webpage and send to connected  client
  String toSend = "";
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

// Function that attempts connection with user input
// TODO: Add some validation for user input?
void handleConnect() {
  WiFi.begin();
  Serial.println("serving page /connect");
  String ssid = webServer.arg(0);
  String password = webServer.arg(1);

  bool connecting = true;
  String connectionStatus = "";
  WiFi.begin(ssid, password);
  //delay(300);  // settle down


  // TODO: Change response to include button to take user back if they want to
  // try again.
  // Attempt to connect to given network
  while (connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      connectionStatus = "<h2>Connected</h2>";
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

  replacement_t repls[] = { // the elements to replace in the template
  { 1, apSSID.c_str() },
  { 7, connectionStatus.c_str() },
  { 8, "" },
  { 9, "<p>Check <a href='/status'>wifi status</a>.</p>"},
  { 10, "<p><a href='/'>Home</a>.</p>"},
  };
  String toSend = "";
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

// Function handles the status of the wifi connection
void handleStatus() {
  Serial.println("serving page at /status");

  String s = "";
  s += "<ul>\n";
  s += "\n<li>SSID: ";
  s += WiFi.SSID();
  s += "</li>";
  s += "\n<li>Status: ";
  switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      s += "WL_IDLE_STATUS</li>"; break;
    case WL_NO_SSID_AVAIL:
      s += "WL_NO_SSID_AVAIL</li>"; break;
    case WL_SCAN_COMPLETED:
      s += "WL_SCAN_COMPLETED</li>"; break;
    case WL_CONNECTED:
      s += "WL_CONNECTED</li>"; break;
    case WL_CONNECT_FAILED:
      s += "WL_CONNECT_FAILED</li>"; break;
    case WL_CONNECTION_LOST:
      s += "WL_CONNECTION_LOST</li>"; break;
    case WL_DISCONNECTED:
      s += "WL_DISCONNECTED</li>"; break;
    default:
      s += "unknown</li>";
  }

  s += "\n<li>Local IP: ";     s += ip2str(WiFi.localIP());
  s += "</li>\n";
  s += "\n<li>Soft AP IP: ";   s += ip2str(WiFi.softAPIP());
  s += "</li>\n";
  s += "\n<li>AP SSID name: "; s += apSSID;
  s += "</li>\n";

  s += "</ul></p>";

  replacement_t repls[] = { // the elements to replace in the boilerplate
    { 1, apSSID.c_str() },
    { 7, "<h2>Status</h2>\n" },
    { 8, "" },
    { 9, s.c_str() },
    { 10, "<p><a href='/'>Home</a></p>" },
  };

  String toSend = "";
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
  
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
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

// function to input server URL for OTA
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
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

void setupOTA() {
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
    //return;
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
    getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
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
    //http.end(); // free resources
    //return;
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
  getHtml(toSend, boilerForm, ALEN(boilerForm), repls, ALEN(repls));
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

// callback handler for tracking OTA progress ///////////////////////////////
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float) done / (float) total;

  int lightCount = int(progress*100) / 25;
  lightCount += 1;
  
  for (int i = 0; i<lightCount; i++) {
    digitalWrite(pinArray[i], HIGH);
  }

}

String ip2str(IPAddress address) { // utility for printing IP addresses
  return
    String(address[0]) + "." + String(address[1]) + "." +
    String(address[2]) + "." + String(address[3]);
}
