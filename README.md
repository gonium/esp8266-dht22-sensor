### Notes

* ESP01 webserver example: https://github.com/platformio/platformio/blob/develop/examples/espressif/esp8266-webserver/src/HelloServer.ino
* OneWire library: http://platformio.org/#!/lib/show/1/OneWire
* Hardware setup/devboard: [Basic Wiring](http://www.esp8266.com/wiki/doku.php?id=getting-started-with-the-esp8266)
* [Initial Bootstrapping](http://williamdurand.fr/2015/03/17/playing-with-a-esp8266-wifi-module/)



### Platformio-specific commands:

Useful commands:
`platformio run` - process/build project from the current directory
`platformio run --target upload` or `platformio run -t upload` - upload firmware to embedded board
`platformio run --target clean` - clean project (remove compiled files)
