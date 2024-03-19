#include "PrAndUpThing.h"

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
