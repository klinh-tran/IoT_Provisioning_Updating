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

// Startup utilities
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
void handleConnect() {
  WiFi.begin();
  Serial.println("serving page /connect");
  String ssid = webServer.arg(0);
  String password = webServer.arg(1);

  bool connecting = true;
  String connectionStatus = "";
  WiFi.begin(ssid, password);

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
void handleStatus(){
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



