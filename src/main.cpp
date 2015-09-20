#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include "../config.h"
#define DHTTYPE DHT22
#define DHTPIN  2

MDNSResponder mdns;
ESP8266WebServer server(80);

// Initialize DHT sensor
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

float humidity, temp;  // Values read from sensor
String webString="";     // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

static void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp = dht.readTemperature(false);     // Read temperature as Celsius
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}


static void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
 
void setup(void){
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Wifi temperature sensor v0.1");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin(hostname, WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

	server.on("/", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
		webString = "Sensor " + String(hostname) + " reports:\n";
    webString+="Temperature: "+String(temp)+" degree Celsius\n";
    webString+="Humidity: "+String(humidity)+" Percent";
    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });
	server.on("/temperature", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    webString="{\"temperature\": "+String(temp)+",\"unit\": \"Celsius\"}";
    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });
 
  server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();           // read sensor
    webString="{\"humidity\": "+String(humidity)+",\"unit\": \"Percent\"}";
    server.send(200, "text/plain", webString);               // send to someones browser when asked
  });
	server.onNotFound(handleNotFound);

  server.begin();
}

void loop(void){
  server.handleClient();
}

