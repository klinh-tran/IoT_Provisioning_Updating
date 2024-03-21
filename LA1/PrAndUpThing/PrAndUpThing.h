#ifndef PRANDUPTHING_H
#define PRANDUPTHING_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

extern WebServer webServer; // a simple web server
extern String apSSID;       // SSID of the AP
extern int firmwareVersion; // used to check for updates

// function protos
void setupAP();
void setupWiFiProv();
void handleRoot();
void handleWifi();
void handleConnect();
void handleStatus();
void handleUpdate();
void handleNotFound();
String ip2str(IPAddress address);

typedef struct {
  int position;
  const char *replacement;
} replacement_t;

void getHtml(String &html, const char *[], int, replacement_t[], int);
// getting the length of an array in C can be complex...
// https://stackoverflow.com/questions/37538/how-do-i-determine-the-size-of-my-array-in-c
#define ALEN(a) ((int)(sizeof(a) / sizeof(a[0]))) // only in definition scope!
#define GET_HTML(strout, boilerForm, repls)                                    \
  getHtml(strout, boiler, ALEN(boilerForm), repls, ALEN(repls));

#include <HTTPClient.h> // ESP32 library for making HTTP requests
#include <Update.h>     // OTA update library
void setupOTA();
int doCloudGet(HTTPClient *, String);
void handleUpdate();
void setupUpdate();
void handleOTAProgress(size_t done, size_t total);

#endif
