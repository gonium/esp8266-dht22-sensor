#ifndef SRC_DS18S20_HPP
#define SRC_DS18S20_HPP 1

#include "sensor.hpp"
#include <cstdint>
#include <OneWire.h>
#define ONE_WIRE_BUS 2  // DS18S20 pin
#include <runningaverage.h>

const std::uint8_t UNDEFINED = 255;
const std::uint8_t MAX_BUS_SENSORS = 8;

class DS18S20Sensor : public Sensor{
	public:
		DS18S20Sensor(
				const unsigned long measurement_interval,
				const std::uint8_t bus_id) 
			: Sensor(measurement_interval)
			, _bus_id(bus_id)
			, _ds(ONE_WIRE_BUS)
			, _accumulator(6)
			{};
		int init();
		int read();
		int get(float& value);
		virtual ~DS18S20Sensor() {};

	private:
		DS18S20Sensor (const DS18S20Sensor& original);
		DS18S20Sensor& operator= (const DS18S20Sensor& rhs);
		std::uint8_t _bus_id = UNDEFINED;
		OneWire _ds;
		RunningAverage _accumulator;
		
};


#endif /* SRC_DS18S20_HPP */

