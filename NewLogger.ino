/*
Circuit structure
ESP32 - (Middle layer) - Hardware
5V - - GPS RED wire (+5V)
GND - - GPS BLACK wire(0V)
IO16 - - GPC White wire(RX)
IO21 - 10Kohm - CCS811 SDA
IO21 - 10Kohm - BM280 SDA
IO22 - 10Kohm - CCS811 SLK
IO22 - 10Kohm - BM280 SLK
3.3V - -BM280 Vio
3.3V - -BM280 Vcore
3.3V - -BM280 CS8
3.3V - -CCS811 3.3V
GND - -BM280 GND
GND - -BM280 SD0
GND - -CCS811 GND
IO4 - - CCS811 Wake
IO5 - - CCS811 Reset
*/



#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SSCI_BME280.h>
#include <SparkFunCCS811.h>

#define BME280_ADDRESS 0x76
#define CCS811_ADDR 0x5B
#define UUID "Test0002"

TinyGPSPlus gps;
unsigned long int hum_raw, temp_raw, pres_raw;
SSCI_BME280 bme280;
CCS811 mySensor(CCS811_ADDR);


void setup() {
    Wire.begin();
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);

  uint8_t osrs_t = 1;             //Temperature oversampling x 1
  uint8_t osrs_p = 1;             //Pressure oversampling x 1
  uint8_t osrs_h = 1;             //Humidity oversampling x 1
  uint8_t bme280mode = 3;         //Normal mode
  uint8_t t_sb = 5;               //Tstandby 1000ms
  uint8_t filter = 0;             //Filter off
  uint8_t spi3w_en = 0;           //3-wire SPI Disable

  bme280.setMode(BME280_ADDRESS, osrs_t, osrs_p, osrs_h, bme280mode, t_sb, filter, spi3w_en);

  bme280.readTrim();
  
    CCS811Core::status returnCode = mySensor.begin();
  if (returnCode != CCS811Core::SENSOR_SUCCESS)
  {
    Serial.println(".begin() returned with an error.");
    while (1); //Hang if there was a problem.
  }

  
}

void loop() {
  // put your main code here, to run repeatedly:
  /*
    if(Serial2.available()){
    int inByte = Serial2.read();
    Serial.write(inByte);
    }*/

  while (Serial2.available()) {
    char c = Serial2.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      Serial.print("LAT:  "); Serial.println(gps.location.lat(), 9);
      Serial.print("LONG: "); Serial.println(gps.location.lng(), 9);
    }
  }

   double temp_act, press_act, hum_act; //最終的に表示される値を入れる変数

  bme280.readData(&temp_act, &press_act, &hum_act);

  Serial.print("TEMP : ");
  Serial.print(temp_act);
  Serial.print(" DegC  PRESS : ");
  Serial.print(press_act);
  Serial.print(" hPa  HUM : ");
  Serial.print(hum_act);
  Serial.println(" ");

  if (mySensor.dataAvailable())
  {
    //If so, have the sensor read and calculate the results.
    //Get them later
    mySensor.readAlgorithmResults();

    Serial.print("CO2[");
    //Returns calculated CO2 reading
    Serial.print(mySensor.getCO2());
    Serial.print("] tVOC[");
    //Returns calculated TVOC reading
    Serial.print(mySensor.getTVOC());
    Serial.print("] millis[");
    //Simply the time since program start
    Serial.print(millis());
    Serial.print("]");
    Serial.println();
  }
  /*
    if (Serial.available()) {
    int inByte = Serial.read();
    Serial2.write(inByte);
    }*/
}
