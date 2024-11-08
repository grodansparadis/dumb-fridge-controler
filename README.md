# dumb-fridge-controler

Very simple fridge controller using the RP2040. Uses NTC temperature sensor and 16 x 02 i2c connected LCD display. Setpoint and hysteresis is set in firmware. 

There is a sister project [here](https://github.com/grodansparadis/vscp-demo-pico-fridge-ctrl) that use Ethernet and the VSCP protocol over MQTT and thus is an intelligent controller where parameters can be set and checked and reports of temperatures, alarms etc can be monitored.


