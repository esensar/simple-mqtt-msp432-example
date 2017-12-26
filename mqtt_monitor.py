import paho.mqtt.client as mqtt
import time

TOPIC_NAME = 'devices/#'
LIGHTUP_LED = "devices/red/controls/led"
color=""
client = mqtt.Client()

def setColor(number):
    color+=number

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(TOPIC_NAME)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    topicArray = msg.topic.split("/")
    global color
    if topicArray[3] == "left":
        color+="1"

    if topicArray[3] == "right":
        color+="0"

    if topicArray[3] == "wifi":
        print("Wifi strenght is "+str(msg.payload))

    if len(color) == 3:
        i=3
        while i>0:
            print(i)
            time.sleep(1)
            i-=1
        client.publish(LIGHTUP_LED, color, 0, False)
        print("Message sent \n")
        color=""

client.on_connect = on_connect
client.on_message = on_message
ip = raw_input("Enter brooker ip address \n")
client.connect(ip, 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()
