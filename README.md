# Simple MQTT MSP432P401R example

Simple MQTT example made for university project with [Dino Dizdarević](https://github.com/dizda13). 

Relies on a wifi connection which needs to be configured in code (ssid and password are hardcoded) and on [Energia IDE](http://energia.nu/). If everything is set up properly, device will try to connect to the MQTT server. If it fails, secondary LED will light up red.

After connection to MQTT server is made, device will publish rssi to the server every 5 seconds and it will also publish button presses. It will subscribe to topics which allow turning on 3 leds (red, green and blue). It will also subscribe to topic which allows the error led to be cleared.

This project is just a simple representation of a very simple control/monitoring via lightweight MQTT protocol.
