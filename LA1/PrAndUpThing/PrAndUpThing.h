#ifndef PRANDUPTHING_H
#define PRANDUPTHING_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

extern WebServer webServer; // a simple web server
extern String apSSID;           // SSID of the AP
extern int firmwareVersion; // used to check for updates

// function protos
void setupAP();
void setupServer();
void handleRoot();
void handleConnect();
void handleNotFound();

// boilerplate: constants & pattern parts of template
extern const char *boiler[];

typedef struct {
  int position;
  const char *replacement;
} replacement_t;

void getHtml(String &html, const char *[], int, replacement_t[], int);
// getting the length of an array in C can be complex...
// https://stackoverflow.com/questions/37538/how-do-i-determine-the-size-of-my-array-in-c
#define ALEN(a) ((int)(sizeof(a) / sizeof(a[0]))) // only in definition scope!
#define GET_HTML(strout, boiler, repls)                                        \
  getHtml(strout, boiler, ALEN(boiler), repls, ALEN(repls));
void handleSeven();   // either remove this, or give it TODO and rename

// end of THING_H guard
#endif

#include <HTTPClient.h> // ESP32 library for making HTTP requests
#include <Update.h>     // OTA update library
void readThenWriteVersion();
int doCloudGet(HTTPClient *, String);
void handleOTAProgress(size_t done, size_t total);