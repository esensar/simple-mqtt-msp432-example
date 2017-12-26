# Simple MQTT MSP432P401R example

Simple MQTT example made for university project with [Dino Dizdarević](https://github.com/dizda13). 

This project is just a simple representation of a very simple control/monitoring via lightweight MQTT protocol.

Relies on a wifi connection which needs to be configured in code (ssid and password are hardcoded) and on [Energia IDE](http://energia.nu/). If everything is set up properly, device will try to connect to the MQTT server. If it fails, secondary LED will light up red. For a simple MQTT broker, [Mosquitto](https://mosquitto.org/) was used.

After connection to MQTT server is made, device will publish rssi to the server every 5 seconds and it will also publish button presses. It will subscribe to topics which allow turning on 3 leds (red, green and blue). It will also subscribe to topic which allows the error led to be cleared.

Simple MQTT monitor written in Python is included in the repository to connect to broker and monitor the device. Monitor will automatically publish led controls based on button presses received from the device. It is written in for Python2.x and relies on [paho-mqtt library](https://pypi.python.org/pypi/paho-mqtt/1.1). 
