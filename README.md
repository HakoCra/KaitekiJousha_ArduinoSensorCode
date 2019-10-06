# KaitekiJousha_ArduinoSensorCode
This is Arduino code that read Temp/hum from Wio 3G 

## detail
It use great device whitch is "Wio 3G"  
and use BME280 Temp/Hum/Pre Sensor, Serial LED tape ~~and MN5010HS GPS Mini Module~~

It require library,
- Wire.h
- ArduinoJson.h
- SSCI_BME280.h
- WioCellLibforArduino.h
- Adafruit_NeoPixel.h

Connect BME280 with I2C to Wio 3G I2C Groove port  
Connect Serial LEB Tape to Wio 3G D38 Groove port  
~~Connect MN5010HS GPS Mini Module to Wio 3G UART Groove port~~  
