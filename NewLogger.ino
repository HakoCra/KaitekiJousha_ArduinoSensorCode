/*
Hardware - Middle ware - Component
3.3V - CCS811 3.3V
3.3V - BME280 Vio
3.3V - BME280 Vcore
3.3V - BME280 SD0
GND - CCS811 GND
GND - BME280 GND
IO21 - BME280 SDI
IO21 - CCS811 SDA
IO22 - BME280 SCK
IO22 - CCS811 SCK
IO16 - GP-20Up TX
5V - GP-20Up RED wire POW
GND - GP-20Up BLACK wire GND
*/
#include <WiFi.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <SparkFunCCS811.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define CCS811_ADDR 0x5B
#define PIN_NOT_WAKE 5
#define UID "52"

const char* ssid = "YiPhone";
const char* password = "Yuzuka457350";
const char* host="https://fukai.mybluemix.net/post-data";

//Global sensor objects
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;
TinyGPSPlus gps;
const size_t capacity = JSON_OBJECT_SIZE(9);

/*main variables*/
float tempareture, humidity, rh, pressure, lngi, lati;
int co2, tvoc;

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600);
  Serial.println();
  Serial.println("Apply BME280 data to CCS811 for compensation.");

  Wire.begin();

  //This begins the CCS811 sensor and prints error status of .begin()
  CCS811Core::status returnCode = myCCS811.begin();
  Serial.print("CCS811 begin exited with: ");
  //Pass the error code to a function to print the results
  printDriverError( returnCode );
  Serial.println();

  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;

  //Initialize BME280
  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  //Calling .begin() causes the settings to be loaded
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  myBME280.begin();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}
//---------------------------------------------------------------
void loop()
{
  //Check to see if data is available
  if (myCCS811.dataAvailable())
  {
    //Calling this function updates the global tVOC and eCO2 variables
    myCCS811.readAlgorithmResults();
    //printInfoSerial fetches the values of tVOC and eCO2
    printInfoSerial();

    float BMEtempC = myBME280.readTempC();
    float BMEhumid = myBME280.readFloatHumidity();

    Serial.print("Applying new values (deg C, %): ");
    Serial.print(BMEtempC);
    Serial.print(",");
    Serial.println(BMEhumid);
    Serial.println();

    //This sends the temperature data to the CCS811
    myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);
  }
  else if (myCCS811.checkForStatusError())
  {
    //If the CCS811 found an internal error, print it.
    printSensorError();
  }

  delay(2000); //Wait for next reading
}

//---------------------------------------------------------------
void printInfoSerial()
{
  //getCO2() gets the previously read data from the library
  Serial.println("CCS811 data:");
  Serial.print(" CO2 concentration : ");
  co2 = myCCS811.getCO2();
  Serial.print(co2);
  Serial.println(" ppm");

  //getTVOC() gets the previously read data from the library
  Serial.print(" TVOC concentration : ");
  tvoc = myCCS811.getTVOC();
  Serial.print(tvoc);
  Serial.println(" ppb");

  Serial.println("BME280 data:");
  Serial.print(" Temperature: ");
  tempareture = myBME280.readTempC();
  Serial.print(tempareture, 2);
  Serial.println(" degrees C");

  Serial.print(" Temperature: ");
  Serial.print(myBME280.readTempF(), 2);
  Serial.println(" degrees F");

  Serial.print(" Pressure: ");
  pressure = myBME280.readFloatPressure();
  Serial.print(pressure, 2);
  Serial.println(" Pa");

  Serial.print(" Pressure: ");
  Serial.print((myBME280.readFloatPressure() * 0.0002953), 2);
  Serial.println(" InHg");

  Serial.print(" Altitude: ");
  Serial.print(myBME280.readFloatAltitudeMeters(), 2);
  Serial.println("m");

  Serial.print(" Altitude: ");
  Serial.print(myBME280.readFloatAltitudeFeet(), 2);
  Serial.println("ft");

  Serial.print(" %RH: ");
  rh = myBME280.readFloatHumidity();
  Serial.print(rh, 2);
  Serial.println(" %");

  Serial.println();
  while (Serial2.available()) {
    char c = Serial2.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      Serial.print("LAT:  "); Serial.println(gps.location.lat(), 9);
      Serial.print("LONG: "); Serial.println(gps.location.lng(), 9);
      sendDataToServer(tempareture, humidity, rh, pressure, gps.location.lng(), gps.location.lat(), co2, tvoc);
    }else{
      Serial.print(c);
      }
  }
  Serial.println("");
  //sendDataToServer(tempareture, humidity, rh, pressure, 0.0000, 0.0000, co2, tvoc);


}

//printDriverError decodes the CCS811Core::status type and prints the
//type of error to the serial terminal.
//
//Save the return value of any function of type CCS811Core::status, then pass
//to this function to see what the output was.
void printDriverError( CCS811Core::status errorCode )
{
  switch ( errorCode )
  {
    case CCS811Core::SENSOR_SUCCESS:
      Serial.print("SUCCESS");
      break;
    case CCS811Core::SENSOR_ID_ERROR:
      Serial.print("ID_ERROR");
      break;
    case CCS811Core::SENSOR_I2C_ERROR:
      Serial.print("I2C_ERROR");
      break;
    case CCS811Core::SENSOR_INTERNAL_ERROR:
      Serial.print("INTERNAL_ERROR");
      break;
    case CCS811Core::SENSOR_GENERIC_ERROR:
      Serial.print("GENERIC_ERROR");
      break;
    default:
      Serial.print("Unspecified error.");
  }
}

//printSensorError gets, clears, then prints the errors
//saved within the error register.
void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();

  if ( error == 0xFF ) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5) Serial.print("HeaterSupply");
    if (error & 1 << 4) Serial.print("HeaterFault");
    if (error & 1 << 3) Serial.print("MaxResistance");
    if (error & 1 << 2) Serial.print("MeasModeInvalid");
    if (error & 1 << 1) Serial.print("ReadRegInvalid");
    if (error & 1 << 0) Serial.print("MsgInvalid");
    Serial.println();
  }
}





void sendDataToServer(float temp, float hum, float rh, float pre, float longi, float lati, int co2, int tvoc) {

  StaticJsonDocument<capacity> doc;

  JsonObject root = doc.to<JsonObject>();
  root["line"] = UID;
  root["temperature"] = temp;
  root["humidity"] = hum;
  root["pressure"] = pre;
  root["longitude"] = longi;
  root["latitude"] = lati;
  root["co2"] = co2;
  root["tvoc"] = tvoc;

  char data[1024];
  serializeJson(doc, data);
  Serial.println(data);
  
  // post request
  HTTPClient http;
  http.begin(host);
  http.addHeader("Content-Type", "application/json");
  int status_code = http.POST((uint8_t*)data, strlen(data));
  Serial.print("status Code : ");
  Serial.println(status_code);
  if( status_code == 200 ){
    Stream* resp = http.getStreamPtr();

    DynamicJsonDocument json_response(255);
    deserializeJson(json_response, *resp);

    serializeJson(json_response, Serial);
    Serial.println("");
  }
  http.end();
}
