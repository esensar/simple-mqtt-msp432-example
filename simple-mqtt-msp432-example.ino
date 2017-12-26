#define MQTTCLIENT_QOS2 1

// LEDS
#define RED RED_LED
#define GRE GREEN_LED
#define BLU BLUE_LED

// msp432P401R specific pins for second led and right and left buttons
#define ERRORLED 78
#define LEFT 74
#define RIGHT 73

#include <SPI.h>
#include <WiFi.h>
#include <WifiIPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>
#include <pthread.h>

// Network Config - REPLACE WITH SSID
char ssid[] = "wifiSSid";
// Network Password - REPLACE WITH REAL PASSWORD
char password[] = "realpass";

// MQTT Server info - REPLACE WITH REAL HOST NAME
char hostname[] = "host name!";
const int port = 1883;

const int serialbaud = 115200;

char printbuf[100];

int arrivedcount = 0;
int connectionErrors = 0;

// Simple logging of recevied messages on serial port
void logMessage(MQTT::Message& message) 
{
    sprintf(printbuf, "Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n",
          ++arrivedcount, message.qos, message.retained, message.dup, message.id);
    Serial.print(printbuf);
    sprintf(printbuf, "Payload %s\n", (char*)message.payload);
    Serial.print(printbuf);
}

// Callback when a request to change led color has arrived from broker
void colorArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;
  logMessage(message);

	// Consider only messages with 3 characters - one for each of RGB
  if (message.payloadlen == 3) {
      bool red = ((char*)message.payload)[0] - '0';
      bool green = ((char*)message.payload)[1] - '0';
      bool blue = ((char*)message.payload)[2] - '0';

      digitalWrite(RED, red ? HIGH : LOW);
      digitalWrite(GRE, green ? HIGH : LOW);
      digitalWrite(BLU, blue ? HIGH : LOW);
  } else {
      sprintf(printbuf, "Bad payload: %s\n", (char*)message.payload);
      Serial.print(printbuf);
  }
}

// Callback when a request to disable error notification led has arrived from broker
void errorOverrideArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    logMessage(message);
    fixError();
}

WifiIPStack ipstack;
MQTT::Client<WifiIPStack, Countdown> client = MQTT::Client<WifiIPStack, Countdown>(ipstack);

// Topics used - publishing to sensors and subscribing to controls
char* ledControlTopic = "devices/red/controls/led";
char* errorControlTopic = "devices/red/controls/error";
char* leftSensorTopic = "devices/red/sensors/left";
char* rightSensorTopic = "devices/red/sensors/right";
char* wifiSensorTopic = "devices/red/sensors/wifi";

// Counter for conection errors to turn on error notification led
void onConnectionError() {
    connectionErrors++;
    if (connectionErrors >= 3) {
        // Light up red LED
        digitalWrite(ERRORLED, HIGH);
    } else {
        // Keep LED off
        digitalWrite(ERRORLED, LOW);
    }
}

// Clear error notification led
void fixError() {
    connectionErrors = 0;
    // Keep LED off
    digitalWrite(ERRORLED, LOW);
}

// Connection to MQTT broker
void connect()
{
  sprintf(printbuf, "Connecting to %s:%d\n", hostname, port);
  Serial.print(printbuf);
  int rc = ipstack.connect(hostname, port);
  if (rc != 1)
  {
    sprintf(printbuf, "rc from TCP connect is %d\n", rc);
    Serial.print(printbuf);
    onConnectionError();
  }
 
  Serial.println("MQTT connecting");
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
  data.MQTTVersion = 3;
  data.clientID.cstring = (char*)"red";
  rc = client.connect(data);
  if (rc != 0)
  {
    sprintf(printbuf, "rc from MQTT connect is %d\n", rc);
    Serial.print(printbuf);
    onConnectionError();
  }
  Serial.println("MQTT connected");

	// Subscribe to controls
  rc = client.subscribe(ledControlTopic, MQTT::QOS2, colorArrived);
  if (rc != 0)
  {
    sprintf(printbuf, "rc from MQTT subscribe is %d\n", rc);
    Serial.print(printbuf);
    onConnectionError();
  }

  rc = client.subscribe(errorControlTopic, MQTT::QOS2, errorOverrideArrived);
  if (rc != 0)
  {
    sprintf(printbuf, "rc from MQTT subscribe is %d\n", rc);
    Serial.print(printbuf);
    onConnectionError();
  }

  Serial.println("MQTT subscribed");
}

// create the light listener - just waiting for controls
void *listen(void* ptr)
{
    while(1) {
        client.yield(1000);
        delay(50);
    }
}

// create the wifi publisher - publishing current wifi rssi every 5 seconds to /wifi topic
void *publish_wifi(void* ptr)
{
    while(1) {
        char buf[100];

        MQTT::Message message;
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        sprintf(buf, "%d", WiFi.RSSI());
        Serial.println(buf);
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf)+1;
        int rc = client.publish(wifiSensorTopic, message);
        delay(5000);
    }
}

// listen for button presses - debounces for 500ms after releasing - publishes to /left or /right depending on which button was pressed
void *listen_button(void* btn)
{
    bool pressed = false;
    while(1) {
        char buf[100];
        int btnNum = (int)btn;
        int state = digitalRead(btnNum);
        MQTT::Message message;
        message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
        if (pressed) {
            if (state == HIGH) {
                pressed = false;
                delay(500);
            }
        } else {
          if (state == LOW) {
              pressed = true;
              sprintf(buf, "PRESSED");
              Serial.println(buf);
              message.payload = (void*)buf;
              message.payloadlen = strlen(buf)+1;
              int rc = client.publish(btnNum == LEFT ? leftSensorTopic : rightSensorTopic, message);
             client.yield(100);
          }
         }
				 // Small delay to allow other threads to jump in
        delay(10);
    }
}

void setup()
{
  // initialize leds as output
  pinMode(RED, OUTPUT);
  pinMode(BLU, OUTPUT);
  pinMode(GRE, OUTPUT);
  pinMode(ERRORLED, OUTPUT);

  // initialize buttons
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);

	// Initialize serial
  Serial.begin(serialbaud);
  Serial.print("Attempting to connect to Network named: ");
  Serial.println(ssid); 
  // Connect to network
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED) {
    // print dots while we wait to connect
    Serial.print(".");
    delay(300);
  }
  
  Serial.println("\nYou're connected to the network");
  Serial.println("Waiting for an ip address");
  
  while (WiFi.localIP() == INADDR_NONE) {
    // print dots while we wait for an ip addresss
    Serial.print(".");
    delay(300);
  }

  Serial.println("\nIP Address obtained");
  // We are connected and have an IP address.
  Serial.println(WiFi.localIP());
  
  connect();

  // create the threads
  pthread_t thread1, thread2, thread3, thread4;
  int thr = 1;
  // start the listener thread
  pthread_create(&thread1, NULL, *listen, (void*) thr);

	// start wifi RSSI publisher thread
  pthread_create(&thread2, NULL, *publish_wifi, (void*) thr);

	// Start the left and right button listeners
  pthread_create(&thread3, NULL, *listen_button, (void*) LEFT);
  pthread_create(&thread4, NULL, *listen_button, (void*) RIGHT);
}

void loop()
{
	// Reconnect if needed
  if (!client.isConnected())
    connect();
  
	// Don't spam
  delay(5000);
}
