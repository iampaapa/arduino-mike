#include <WiFi.h> // include the WiFi library
#include <HTTPClient.h> // include the HTTPClient library for making HTTP requests
#include <Adafruit_Sensor.h> // include the Adafruit_Sensor library for using the DHT22 sensor
#include <DHT.h> // include the DHT library for reading the temperature

const char* ssid = "your_SSID"; // replace with your WiFi network name (SSID)
const char* password = "your_PASSWORD"; // replace with your WiFi password

const char* endpoint = "http://your_endpoint_url.com/data"; // replace with your endpoint URL for sending sensor data
const char* relayEndpoint = "http://your_endpoint_url.com/relay"; // replace with your endpoint URL for controlling the relay

const int currentPin = A0; // analog input pin for the ACS712 current sensor
const int voltagePin = A1; // analog input pin for the ZMPT101B voltage sensor
const int relayPin = 3; // digital output pin for the relay
const int DHTPIN = 2; // digital input pin for the DHT22 temperature sensor
const int DHTTYPE = DHT22; // type of DHT sensor (DHT22 or DHT11)

bool relayState = false; // initial state of the relay (OFF)

DHT dht(DHTPIN, DHTTYPE); // initialize the DHT22 sensor object with the pin number and type

void setup() {
  Serial.begin(9600); // initialize the serial communication
  pinMode(currentPin, INPUT); // set the current pin as an input
  pinMode(voltagePin, INPUT); // set the voltage pin as an input
  pinMode(relayPin, OUTPUT); // set the relay pin as an output

  WiFi.begin(ssid, password); // connect to the WiFi network
  while (WiFi.status() != WL_CONNECTED) { // wait for the WiFi connection to be established
    delay(1000); // wait for 1 second
    Serial.println("Connecting to WiFi..."); // print a message to indicate connection status
  }
  Serial.println("Connected to WiFi"); // print a message to indicate connection status
  
  dht.begin(); // initialize the DHT22 sensor
}

void loop() {
  float temperature = dht.readTemperature(); // read the temperature from the DHT22 sensor
  float current = (analogRead(currentPin) - 512) * 0.185; // read the current from the ACS712 sensor and convert the analog value to amperes with a sensitivity of 185mV/A
  float voltage = (analogRead(voltagePin) * 3.3) / 4096.0 * 228.57; // read the voltage from the ZMPT101B sensor, convert the analog value to volts with a voltage divider

  // create JSON payload
  String payload = "{\"temperature\":" + String(temperature) + ",\"current\":" + String(current) + ",\"voltage\":" + String(voltage) + "}";
  
  // send POST request to endpoint with payload
  HTTPClient http;
  http.begin(endpoint); // initialize the HTTP client with the endpoint URL
  http.addHeader("Content-Type", "application/json"); // add the content type header to the request
  int httpResponseCode = http.POST(payload); // send the POST request with the payload and get the HTTP response code
  if (httpResponseCode > 0) { // if the HTTP response code is positive
    String response = http.getString(); // get the response body as a string
    Serial.println(httpResponseCode); // print the HTTP response code
    Serial.println(response); // print the response body
  } else { // if the HTTP response code is negative
    Serial.println("Error sending POST request"); // print an error message
  }
  http.end(); // close the HTTP connection

  // check for incoming requests to control the relay
  WiFiClient client = server.available(); // listen for incoming clients
  if (client) { // if there is a client
    while (client.connected()) { // while the client is connected
      if (client.available()) { // if there is data available
        String request = client.readStringUntil('\r'); // read the request
        client.flush(); // flush the client
        if (request.indexOf("on") != -1) { // if the request contains "on"
          digitalWrite(relayPin, HIGH); // turn on the relay
          relayState = true; // update the relay state
          Serial.println("Relay turned ON"); // print a message
        } else if (request.indexOf("off") != -1) { // if the request contains "off"
          digitalWrite(relayPin, LOW); // turn off the relay
          relayState = false; // update the relay state
          Serial.println("Relay turned OFF"); // print a message
        }
        // create response
        String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
        response += "<h2>Relay state: ";
        response += (relayState ? "ON" : "OFF");
        response += "</h2>\r\n";
        response += "</html>\n";
        client.print(response); // send the response
        delay(1); // wait for the response to be sent
        client.stop(); // stop the client
      }
    }
  }
}

