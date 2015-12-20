#ifndef SRC_SENSOR_HPP
#define SRC_SENSOR_HPP 1

enum SensorState {
	MEASURED_OK,
	MEASURED_FAILED,
	TOO_EARLY,
	INIT_FAILED,
	INIT_OK
};

class Sensor {
	public:
		virtual int init() = 0;
		virtual int read() = 0;
		virtual int get(float& value) = 0;
		Sensor (const unsigned long measurement_interval) 
			: _interval(measurement_interval),
			_previousMillis(0) {};
		virtual ~Sensor() {};
	protected:
		unsigned long _previousMillis;
		const unsigned long _interval;
	private:
		Sensor (const Sensor& original);
		Sensor& operator= (const Sensor& rhs);
};

#endif /* SRC_SENSOR_HPP */

