#include "ds18s20.hpp"
#include <Arduino.h>

int DS18S20Sensor::init() {
	if (_bus_id == UNDEFINED) {
		Serial.println("Bus ID of DS18S20 Sensor undefined - cannot initialize.");
		return INIT_FAILED;
	} else {
		return INIT_OK;
	}
}


int DS18S20Sensor::read() {
	// Wait at least interval milliseconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - _previousMillis >= _interval) {
    // save the last time you read the sensor 
    _previousMillis = currentMillis;

		// This is the code for receiving temperature readings from a DS18S20.
		// see https://github.com/esp8266/Arduino/blob/esp8266/libraries/OneWire/examples/DS18x20_Temperature/DS18x20_Temperature.pde
		byte i;
		byte present = 0;
		byte type_s;
		byte data[12];
		byte addr[MAX_BUS_SENSORS][8]; // can hold four addresses of 8 byte each
		byte addr_count = 0;

		_ds.reset();
		_ds.reset_search();

		for (addr_count = 0; addr_count < MAX_BUS_SENSORS; ++addr_count) {
			if ( !_ds.search(addr[addr_count])) {
				//Serial.println("No more addresses.");
				_ds.reset_search();
				delay(250);
				break;
			} //else {
				//Serial.print("found address ");
				//for( i = 0; i < 8; i++) {
				//	Serial.write(' ');
				//	Serial.print(addr[addr_count][i], HEX);
				//}
				//Serial.println();
			//}
		}

		//Serial.print("found ");
		//Serial.print(addr_count);
		//Serial.println(" addresses.");

		if (addr_count == 0) {
			Serial.println("No sensors found.");
			return MEASURED_FAILED;
		}

		if (_bus_id >= addr_count) {
			Serial.print("Requested sensor ");
			Serial.print(_bus_id);
			Serial.println(" but not that many sensors on bus.");
			return MEASURED_FAILED;
		}

		if (OneWire::crc8(addr[_bus_id], 7) != addr[_bus_id][7]) {
			Serial.println("CRC is not valid!");
			return MEASURED_FAILED;
		}
		//Serial.println();

		// the first ROM byte indicates which chip
		switch (addr[_bus_id][0]) {
			case 0x10:
				Serial.println("  Chip = DS18S20");  // or old DS1820
				type_s = 1;
				break;
			case 0x28:
				Serial.println("  Chip = DS18B20");
				type_s = 0;
				break;
			case 0x22:
				Serial.println("  Chip = DS1822");
				type_s = 0;
				break;
			default:
				Serial.println("Device is not a DS18x20 family device.");
				return MEASURED_FAILED;
		}

		_ds.reset();
		_ds.select(addr[_bus_id]);

		_ds.write(0x44, 0);        // start conversion, no parasitic power

		delay(750);     // maybe 750ms is enough, maybe not
		// we might do a _ds.depower() here, but the reset will take care of it.

		present = _ds.reset();
		_ds.select(addr[_bus_id]);
		_ds.write(0xBE);         // Read Scratchpad

		//Serial.print("  Data = ");
		//Serial.print(present, HEX);
		//Serial.print(" ");
		for ( i = 0; i < 9; i++) {           // we need 9 bytes
			data[i] = _ds.read();
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
		float temp = (float)raw / 16.0;
		Serial.print("Temperature: ");
		Serial.println(temp);

		if (isnan(temp) || temp==85.0 || temp==(-127.0)) {
			Serial.println("Failed to read from sensor");
			// resetting the previous measurement time so that a failed attempt
			// will be repeated with the next query.
			_previousMillis=currentMillis-2000;
			if (_previousMillis < 0)
				_previousMillis = 0;
			return MEASURED_FAILED;
		} else {
			_accumulator.addValue(temp);
			return MEASURED_OK;
		}
	} else {// no measurement taken - time not elapsed
		return TOO_EARLY; 
	}

}

int DS18S20Sensor::get(float& value) {
	value = _accumulator.getAverage();
	return MEASURED_OK;
}
