// This is a defaults file, you need to add your details
#pragma once

// This defines the name of your device
#define WIFI_HOSTNAME "ikea-led"

#ifdef ESP8266
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif

// If you would like to perform OTA updates, you need to define the credentials here
#define OTA_USERNAME "admin"
#define OTA_PASSWORD "ikea-led-wall"

// RapidAPI key for HTTP time fallback (world-time-api3)
#define RAPIDAPI_KEY ""
