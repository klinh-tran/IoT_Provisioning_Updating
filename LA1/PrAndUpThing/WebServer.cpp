#include "PrAndUpThing.h"

const char *boilerPage[]{
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

// Handling web server connections
// Open webpage on http://192.168.4.1/
void setupServer() {
  webServer.on("/", handleRoot);
  webServer.on("/wifi", handleWifi);
  webServer.on("/connect", handleConnect);
  webServer.on("/status", handleStatus);
  webServer.on("/update", handleUpdate);
  webServer.on("/setupOTA", OTAUpdate);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

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
  getHtml(toSend, boilerPage, ALEN(boilerPage), repls, ALEN(repls));
  webServer.send(200, "text/html", toSend);
}

void handleNotFound() { webServer.send(404, "text/plain", "Page not found"); }

String ip2str(IPAddress address) { // utility for printing IP addresses
  return
    String(address[0]) + "." + String(address[1]) + "." +
    String(address[2]) + "." + String(address[3]);
}

void getHtml(String &html, const char *boiler[], int boilerLen,
             replacement_t repls[], int replsLen) {
  for (int i = 0, j = 0; i < boilerLen; i++) {
    if (j < replsLen && repls[j].position == i)
      html.concat(repls[j++].replacement);
    else
      html.concat(boiler[i]);
  }
}
