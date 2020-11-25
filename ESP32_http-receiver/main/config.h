#ifndef _CONFIG_H
#define _CONFIG_H

/* ----- CONFIG FILE ----- */

/* Device */
const char *id_name = "ESP32-GETTER";

/* Network Configuration */
const char *ssid_WiFi = "<YOUR WIFI NAME>";
const char *pass_WiFi = "<YOUR WIFI PASS>";

/* HTTP Endpoint Configuration */
const char *address = "<YOUR IP>/messages/last"; /* Endpoint address (HTTP), must NOT include 'http://xxx' or 'tcp://xxx', and must include '/sensor_data' for using in I2T Gateway*/
int port = 3002;

/* Enable Relays */
bool isEnable_Relay_1 = true;
bool isEnable_Relay_2 = false; /*			true: Enable  --  false: Disable			*/
bool isEnable_Relay_3 = false;
bool isEnable_Relay_4 = false;

/* Interval of time */
long interval = 15; /* Time in seconds between */

#endif
