#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SSCI_BME280.h>
#include <WioCellLibforArduino.h>



#define BME280_ADDRESS 0x76
#define UUID "Test0001"
#define APN               "soracom.io"
#define USERNAME          "sora"
#define PASSWORD          "sora"
#define INTERVAL 5000
#define READGPS WIO_UART_D23
#define GPS_OVERFLOW_STRING "OVERFLOW"
#define LEDPIN WIO_D38
#define LEDNUM 30

WioCellular Wio;
char buff[128] = {'\0'};
int counter = 0;
unsigned long int hum_raw, temp_raw, pres_raw;
signed long int t_fine;
String input;
char GpsData[100];
int GpsDataLength=0;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDNUM, LEDPIN, NEO_GRB + NEO_KHZ800);
int led_num=0;

uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;
uint16_t dig_P1;
int16_t dig_P2;
int16_t dig_P3;
int16_t dig_P4;
int16_t dig_P5;
int16_t dig_P6;
int16_t dig_P7;
int16_t dig_P8;
int16_t dig_P9;
int8_t  dig_H1;
int16_t dig_H2;
int8_t  dig_H3;
int16_t dig_H4;
int16_t dig_H5;
int8_t  dig_H6;

void setup() {
  uint8_t osrs_t = 1;             //Temperature oversampling x 1
  uint8_t osrs_p = 1;             //Pressure oversampling x 1
  uint8_t osrs_h = 1;             //Humidity oversampling x 1
  uint8_t mode = 3;               //Normal mode
  uint8_t t_sb = 5;               //Tstandby 1000ms
  uint8_t filter = 0;             //Filter off
  uint8_t spi3w_en = 0;           //3-wire SPI Disable

  uint8_t ctrl_meas_reg = (osrs_t << 5) | (osrs_p << 2) | mode;
  uint8_t config_reg    = (t_sb << 5) | (filter << 2) | spi3w_en;
  uint8_t ctrl_hum_reg  = osrs_h;

  SerialUSB.begin(115200);
  //SerialUART.begin(4800);
 //SerialUART.print("$PUNV,CONFIG,00,00,0,1000,115200,01*0C\r\n");
  Wire.begin();
  writeReg(0xF2, ctrl_hum_reg);
  writeReg(0xF4, ctrl_meas_reg);
  writeReg(0xF5, config_reg);
  readTrim();
  Wio.Init();
  pixels.begin();
  pixels.clear();
  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyCellular(true);
  Wio.PowerSupplyGrove(true);
  delay(500);

  SerialUSB.println("### Turn on or reset.");
#ifdef ARDUINO_WIO_LTE_M1NB1_BG96
  Wio.SetAccessTechnology(WioCellular::ACCESS_TECHNOLOGY_LTE_M1);
  Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_MANUAL_IMSI);
#endif
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Connecting to \"" APN "\".");
  if (!Wio.Activate(APN, USERNAME, PASSWORD)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }
  SerialUSB.println("### Setup completed.");
}

void loop()
{
  input ="";
  double temp_act = 0.0, press_act = 0.0, hum_act = 0.0;
  signed long int temp_cal;
  unsigned long int press_cal, hum_cal;

  readData();

  temp_cal = calibration_T(temp_raw);
  press_cal = calibration_P(pres_raw);
  hum_cal = calibration_H(hum_raw);
  temp_act = (double)temp_cal / 100.0;
  press_act = (double)press_cal / 100.0;
  hum_act = (double)hum_cal / 1024.0;
  SerialUSB.print("TEMP : ");
  SerialUSB.print(temp_act);
  SerialUSB.print(" DegC  PRESS : ");
  SerialUSB.print(press_act);
  SerialUSB.print(" hPa  HUM : ");
  SerialUSB.print(hum_act);
  SerialUSB.println(" %");

  // allocate the memory for the document
  const size_t CAPACITY = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<CAPACITY> doc;

  // create an object
  JsonObject root = doc.to<JsonObject>();

  root["uuid"] = UUID;
  root["temperature"] = temp_act;
  root["humidity"] = hum_act;
  char data[200];
  serializeJson(doc, data);
  int statusCode = 0;
  if (!Wio.HttpPost("http://fukai.mybluemix.net/post-data", data, &statusCode)) {
    SerialUSB.println("ERROR in sending data!!");

  }
  SerialUSB.print("Status code : ");
  SerialUSB.println(statusCode);
  float Res = calcDI(temp_act,hum_act);
  SerialUSB.print("DI : ");
  SerialUSB.println(Res);
  LEDControl(Res);
  delay(INTERVAL);
}
void readTrim()
{
  uint8_t data[32], i = 0;
  Wire.beginTransmission(BME280_ADDRESS);
  Wire.write(0x88);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDRESS, 24);
  while (Wire.available()) {
    data[i] = Wire.read();
    i++;
  }

  Wire.beginTransmission(BME280_ADDRESS);
  Wire.write(0xA1);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDRESS, 1);
  data[i] = Wire.read();
  i++;

  Wire.beginTransmission(BME280_ADDRESS);
  Wire.write(0xE1);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDRESS, 7);
  while (Wire.available()) {
    data[i] = Wire.read();
    i++;
  }
  dig_T1 = (data[1] << 8) | data[0];
  dig_T2 = (data[3] << 8) | data[2];
  dig_T3 = (data[5] << 8) | data[4];
  dig_P1 = (data[7] << 8) | data[6];
  dig_P2 = (data[9] << 8) | data[8];
  dig_P3 = (data[11] << 8) | data[10];
  dig_P4 = (data[13] << 8) | data[12];
  dig_P5 = (data[15] << 8) | data[14];
  dig_P6 = (data[17] << 8) | data[16];
  dig_P7 = (data[19] << 8) | data[18];
  dig_P8 = (data[21] << 8) | data[20];
  dig_P9 = (data[23] << 8) | data[22];
  dig_H1 = data[24];
  dig_H2 = (data[26] << 8) | data[25];
  dig_H3 = data[27];
  dig_H4 = (data[28] << 4) | (0x0F & data[29]);
  dig_H5 = (data[30] << 4) | ((data[29] >> 4) & 0x0F);
  dig_H6 = data[31];
}
void writeReg(uint8_t reg_address, uint8_t data)
{
  Wire.beginTransmission(BME280_ADDRESS);
  Wire.write(reg_address);
  Wire.write(data);
  Wire.endTransmission();
}


void readData()
{
  int i = 0;
  uint32_t data[8];
  Wire.beginTransmission(BME280_ADDRESS);
  Wire.write(0xF7);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDRESS, 8);
  while (Wire.available()) {
    data[i] = Wire.read();
    i++;
  }
  pres_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
  temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
  hum_raw  = (data[6] << 8) | data[7];
}


signed long int calibration_T(signed long int adc_T)
{

  signed long int var1, var2, T;
  var1 = ((((adc_T >> 3) - ((signed long int)dig_T1 << 1))) * ((signed long int)dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((signed long int)dig_T1)) * ((adc_T >> 4) - ((signed long int)dig_T1))) >> 12) * ((signed long int)dig_T3)) >> 14;

  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

unsigned long int calibration_P(signed long int adc_P)
{
  signed long int var1, var2;
  unsigned long int P;
  var1 = (((signed long int)t_fine) >> 1) - (signed long int)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((signed long int)dig_P6);
  var2 = var2 + ((var1 * ((signed long int)dig_P5)) << 1);
  var2 = (var2 >> 2) + (((signed long int)dig_P4) << 16);
  var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((signed long int)dig_P2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((signed long int)dig_P1)) >> 15);
  if (var1 == 0)
  {
    return 0;
  }
  P = (((unsigned long int)(((signed long int)1048576) - adc_P) - (var2 >> 12))) * 3125;
  if (P < 0x80000000)
  {
    P = (P << 1) / ((unsigned long int) var1);
  }
  else
  {
    P = (P / (unsigned long int)var1) * 2;
  }
  var1 = (((signed long int)dig_P9) * ((signed long int)(((P >> 3) * (P >> 3)) >> 13))) >> 12;
  var2 = (((signed long int)(P >> 2)) * ((signed long int)dig_P8)) >> 13;
  P = (unsigned long int)((signed long int)P + ((var1 + var2 + dig_P7) >> 4));
  return P;
}

unsigned long int calibration_H(signed long int adc_H)
{
  signed long int v_x1;

  v_x1 = (t_fine - ((signed long int)76800));
  v_x1 = (((((adc_H << 14) - (((signed long int)dig_H4) << 20) - (((signed long int)dig_H5) * v_x1)) +
            ((signed long int)16384)) >> 15) * (((((((v_x1 * ((signed long int)dig_H6)) >> 10) *
                (((v_x1 * ((signed long int)dig_H3)) >> 11) + ((signed long int) 32768))) >> 10) + (( signed long int)2097152)) *
                ((signed long int) dig_H2) + 8192) >> 14));
  v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((signed long int)dig_H1)) >> 4));
  v_x1 = (v_x1 < 0 ? 0 : v_x1);
  v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);
  return (unsigned long int)(v_x1 >> 12);
}
const char* GpsRead() {
  while (SerialUART.available()) {
    char data = SerialUART.read();
    if (data == '\r') continue;
    if (data == '\n') {
      GpsData[GpsDataLength] = '\0';
      GpsDataLength = 0;
      return GpsData;
    }
    
    if (GpsDataLength > (int)sizeof(GpsData) - 1) { // Overflow
      GpsDataLength = 0;
      return GPS_OVERFLOW_STRING;
    }
    GpsData[GpsDataLength++] = data;
  }

  return NULL;
}

float calcDI(float temp,float hum){
  return 0.81*temp + 0.01*hum * (0.99 * temp - 14.3) + 46.3;
}
void LEDControl(float di){
  if(di<65){
    pixels.setPixelColor(led_num,pixels.Color(0,map(di,65,60,255,0),map(di,65,0,0,255)));
  }else if(di>=65){
      pixels.setPixelColor(led_num,pixels.Color(map(di,65,100,0,255),map(di,65,70,255,0),0));
  }

  pixels.show();
  led_num++;
  if(led_num>30) led_num =0;
  }
