#include "../config.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <runningaverage.h>


#ifdef SENSOR_DS18S20
#include <OneWire.h>
#define ONE_WIRE_BUS 2  // DS18S20 pin
OneWire ds(ONE_WIRE_BUS);
#endif

#ifdef SENSOR_DHT22
#include <DHT.h>
#include "../config.h"
#define DHTPIN  2
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
#endif

MDNSResponder mdns;
ESP8266WebServer server(80);
RunningAverage temp_aggregator(6);
RunningAverage hum_aggregator(6);
bool sensor_ok = false;
enum SensorState {
	MEASURED_OK,
	MEASURED_FAILED,
	TOO_EARLY
};

String webString="";     // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 10000;             // interval at which to read sensor

int ICACHE_FLASH_ATTR gettemperature(float& temp, float& humidity) {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;

#ifdef SENSOR_DS18S20
		// This is the code for receiving temperature readings from a DS18S20.
		// see https://github.com/esp8266/Arduino/blob/esp8266/libraries/OneWire/examples/DS18x20_Temperature/DS18x20_Temperature.pde
		byte i;
		byte present = 0;
		byte type_s;
		byte data[12];
		byte addr[8];

		ds.reset();
		ds.reset_search();
		if ( !ds.search(addr)) {
			Serial.println("No more addresses.");
			Serial.println();
			ds.reset_search();
			delay(250);
			return MEASURED_FAILED;
		}

		//Serial.print("ROM =");
		//for( i = 0; i < 8; i++) {
		//	Serial.write(' ');
		//	Serial.print(addr[i], HEX);
		//}

		if (OneWire::crc8(addr, 7) != addr[7]) {
			Serial.println("CRC is not valid!");
			return MEASURED_FAILED;
		}
		//Serial.println();

		// the first ROM byte indicates which chip
		switch (addr[0]) {
			case 0x10:
				//Serial.println("  Chip = DS18S20");  // or old DS1820
				type_s = 1;
				break;
			case 0x28:
				//Serial.println("  Chip = DS18B20");
				type_s = 0;
				break;
			case 0x22:
				//Serial.println("  Chip = DS1822");
				type_s = 0;
				break;
			default:
				Serial.println("Device is not a DS18x20 family device.");
				return MEASURED_FAILED;
		} 

		ds.reset();
		ds.select(addr);
		ds.write(0x44, 0);        // start conversion, no parasitic power

		delay(750);     // maybe 750ms is enough, maybe not
		// we might do a ds.depower() here, but the reset will take care of it.

		present = ds.reset();
		ds.select(addr);
		ds.write(0xBE);         // Read Scratchpad

		//Serial.print("  Data = ");
		//Serial.print(present, HEX);
		//Serial.print(" ");
		for ( i = 0; i < 9; i++) {           // we need 9 bytes
			data[i] = ds.read();
			//Serial.print(data[i], HEX);
			//Serial.print(" ");
		}
		//Serial.print(" CRC=");
		//Serial.print(OneWire::crc8(data, 8), HEX);
		//Serial.println();

		// Convert the data to actual temperature
		// because the result is a 16 bit signed integer, it should
		// be stored to an "int16_t" type, which is always 16 bits
		// even when compiled on a 32 bit processor.
		int16_t raw = (data[1] << 8) | data[0];
		if (type_s) {
			raw = raw << 3; // 9 bit resolution default
			if (data[7] == 0x10) {
				// "count remain" gives full 12 bit resolution
				raw = (raw & 0xFFF0) + 12 - data[6];
			}
		} else {
			byte cfg = (data[4] & 0x60);
			// at lower res, the low bits are undefined, so let's zero them
			if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
			else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
			else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
			//// default is 12 bit resolution, 750 ms conversion time
		}
		temp = (float)raw / 16.0;
		//Serial.print("Temperature: ");
		//Serial.println(temp);
#endif // of DS18S20-related code

#ifdef SENSOR_DHT22 // Read temp&hum from DHT22
		// Reading temperature for humidity takes about 250 milliseconds!
		// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
		humidity = dht.readHumidity();          // Read humidity (percent)
		temp = dht.readTemperature(false);     // Read temperature as Celsius
#endif

		//Serial.print("Free heap:");
		//Serial.println(ESP.getFreeHeap(),DEC);

		if (isnan(temp) || temp==85.0 || temp==(-127.0)) {
			Serial.println("Failed to read from sensor");
			// resetting the previous measurement time so that a failed attempt
			// will be repeated with the next query.
			previousMillis=currentMillis-2000;
			if (previousMillis < 0) previousMillis = 0;
			return MEASURED_FAILED;
		} else {
			return MEASURED_OK;
		}
	} else {
		return TOO_EARLY; // no measurement taken - time not elapsed
	}
}


void ICACHE_FLASH_ATTR handleNotFound(){
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

void ICACHE_FLASH_ATTR setup(void){
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

	server.on("/", [](){
			if (sensor_ok) {       // read sensor
				webString = "Sensor " + String(hostname) + " reports:\n";
				webString+="Temperature: "+String(temp_aggregator.getAverage())+" degree Celsius\n";
#ifdef SENSOR_DHT22 // Read temp&hum from DHT22
				webString+="Humidity: "+String(hum_aggregator.getAverage())+" % r.H.\n";
#endif
				server.send(200, "text/plain", webString);            // send to someones browser when asked
			} else {
				webString="{\"error\": \"Cannot read data from sensor.\"";
				server.send(503, "text/plain", webString);            // send to someones browser when asked
			}
			});
	server.on("/temperature", [](){
			if (sensor_ok) {       // read sensor
				webString="{\"temperature\": "+String(temp_aggregator.getAverage())+",\"unit\": \"Celsius\"}";
				server.send(200, "text/plain", webString);            // send to someones browser when asked
			} else {
				webString="{\"error\": \"Cannot read data from sensor.\"";
				server.send(503, "text/plain", webString);            // send to someones browser when asked
			}
			});
#ifdef SENSOR_DHT22 // Read humidity from DHT22
	server.on("/humidity", [](){
			if (sensor_ok) {       // read sensor
				webString="{\"humidity\": "+String(hum_aggregator.getAverage())+",\"unit\": \"% r.H.\"}";
				server.send(200, "text/plain", webString);            // send to someones browser when asked
			} else {
				webString="{\"error\": \"Cannot read data from sensor.\"";
				server.send(503, "text/plain", webString);            // send to someones browser when asked
			}
			});

#endif

	server.onNotFound(handleNotFound);

	server.begin();
	ESP.wdtEnable(5000);
}

void ICACHE_FLASH_ATTR loop(void){
	server.handleClient();
	ESP.wdtFeed();
	float temp = 0.0;
	float humidity = 0.0;
	switch (gettemperature(temp, humidity)) {
		case MEASURED_OK:
			sensor_ok = true;
			Serial.println("Updating accumulator w/ new measurements");
			temp_aggregator.addValue(temp);
			hum_aggregator.addValue(humidity);
			break;
		case MEASURED_FAILED:
			Serial.println("Measurement failed");
			sensor_ok = false;
			break;
		case TOO_EARLY:
			;;
			break;
	}
}

