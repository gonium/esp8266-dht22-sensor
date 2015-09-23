## A wifi (ESP8266) temperature/humidity sensor for OpenHAB

This project implements a simple and cheap room temperature/humidity sensor 
based on the DHT22 and the ESP8266 ESP-01 board. 
It uses [platformio](http://platformio.org) to manage the cross compiler and
Arduino framework setup tasks. I wanted a rather cheap sensor that can be integrated
into an [OpenHAB](http://openhab.org) home automation setup easily. The sensor
anounces itself using MDNS. A simple text-based webpage is built in and can be 
accessed using your web browser:

![Website of the sensor](https://raw.githubusercontent.com/gonium/esp8266-dht22-sensor/master/images/webbrowser.png)

You can also query the sensor via the JSON interface:

    $ curl http://roomsensor/temperature
    {"temperature": 23.50,"unit": "Celsius"}
		$ curl http://roomsensor/humidity
    {"humidity": 53.00,"unit": "Percent"}

This can easily be integrated in OpenHAB. You need the HTTP binding first. Then,
create two items like this:

    Number Room_Temperature "Temperatur [%.2f °C]" <temperature> (Climate) { http="<[http://192.168.1.160/temperature:300000:JS(ESP8266GetTemperature.js)]" }
    Number Room_Humidity "Luftfeuchtigkeit [%.2f r.H.]" <humidity> (Climate) { http="<[http://192.168.1.160/humidity:300000:JS(ESP8266GetHumidity.js)]" }

and two javascript transformation to parse the JSON: in ````transform/ESP8266GetTemperature.js````, put

    JSON.parse(input).temperature;

and in ````transform/ESP8266GetHumidity.js````, put

    JSON.parse(input).humidity;

I display these two values using these entries in my sitemap:

    Text item=Room_Temperature icon="temperature"
    Text item=Room_Humidity icon="water"

Here you can see a screenshot of the OpenHAB classic interface using the 
sensor values

![OpenHAB classic UI](https://raw.githubusercontent.com/gonium/esp8266-dht22-sensor/master/images/penthouse-ui.png)

Since the values are just normal OpenHAB items you can store them, graph them
and use them in your rules.

### The Hardware

The ESP-01 board is the simplest ESP8266 board you can buy - I paid around 3,30 
Euros (including shipping!) per board. The DHT22 costs between 4 and 6 Euros, 
depending where you buy. You also need a 3V3 DC 400mA power supply 
(another 3-4 Euros) and some resistors. All together this project should cost you 
15 Euros.

I soldered myself a simple programming breakout board similar to the one presented 
[in the ESP8266 wiki getting started article](http://www.esp8266.com/wiki/doku.php?id=getting-started-with-the-esp8266).
I added a connector to be able to use a breadboard for prototyping. This is how it looks
like:

![Sensorboard schematic](https://raw.githubusercontent.com/gonium/esp8266-dht22-sensor/master/images/programming-jig.jpg)

On the left you can see power coming in and the serial interface (3V3
FTDI breakout board) used for progamming the device. The two pushbuttons
allow me to reboot into programming mode. At the bottom there is a
connector with cables connecting to a breadboard.

For the permanent deployment I use a much simpler setup - no need
for a serial interface or programming buttons. The schematic looks like
this:

![Sensorboard schematic](https://raw.githubusercontent.com/gonium/esp8266-dht22-sensor/master/images/schematic.png)

You can easily construct the circuit on some protoboard using THT
components. Mine looks like this:

![Final Sensor](https://raw.githubusercontent.com/gonium/esp8266-dht22-sensor/master/images/final-sensor.jpg)

The PCP is 5x5 cm big. If I find some time I will build an enclosure for
it using my 3D printer. Since the enclosure needs to ensure a good
ventilation stock enclosures don't work well.

### Software

First, you need to install platformio. Just follow the [Getting Started-Guide](http://platformio.org/#!/get-started).
Essentially, I did the following:

    # update dependent packages to the latest versions
    $ pip install -U pip setuptools

    # install/upgrade the latest version of PlatformIO
    $ pip install -U platformio

Afterwards, clone the project (and the submodules in it):

    $ git clone --recursive git@github.com:gonium/esp8266-dht22-sensor.git
    $ cd esp8266-dht22-sensor.git

Then, you have to copy the example of the config file and edit it to match 
your network configuration:

    $ cp config_sample.h config.h
    $ vim config.h

In this file, please adjust SSID, password and the hostname you would like to 
broadcast via MDNS.

You can simply run the platformio toolchain now, it will download all 
needed components/libraries, compile the code and upload it automatically (please 
put the ESP8266 into bootloader mode - press PROG and RESET, then release RESET
first):

    $ platformio run

For development, I directly attach to the serial console afterwards:

    $ platformio run && platformio serialports monitor

On my system this is the output:

    Wifi temperature sensor v0.1
    ......
    Connected to nibbler
    IP address: 192.168.1.160
    MDNS responder started

The sensor should now be accessible under ````http://192.168.1.160```` and ````http://roomsensor````.

### License

The project is available under the terms of the MIT license. It uses 
 * [Platformio](http://platformio.org) and the ESP8266-Arduino
   environment that is bundled
 * [Adafruits DHT22-Library](https://github.com/adafruit/DHT-sensor-library)


### Notes

* [ESP8266 Dallas Onewire example](http://iot-playground.com/2-uncategorised/41-esp8266-ds18b20-temperature-sensor-arduino-ide)
* ESP01 webserver example: https://github.com/platformio/platformio/blob/develop/examples/espressif/esp8266-webserver/src/HelloServer.ino
* OneWire library: http://platformio.org/#!/lib/show/1/OneWire
* Hardware setup/devboard: [Basic Wiring](http://www.esp8266.com/wiki/doku.php?id=getting-started-with-the-esp8266)
* [Initial Bootstrapping](http://williamdurand.fr/2015/03/17/playing-with-a-esp8266-wifi-module/)



### Platformio-specific commands:

Useful commands:
`platformio run` - process/build project from the current directory
`platformio run --target upload` or `platformio run -t upload` - upload firmware to embedded board
`platformio run --target clean` - clean project (remove compiled files)
