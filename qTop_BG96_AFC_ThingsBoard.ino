/*****************************************************************************
  This is the Demo of using qTop LTE BG9x shield 
  plugged into Arduino MKR Compatible or Adafruit Feather Compatible board with ThingsBoard IOT Dashboard.

  Hardware Setup:
    Arduino MKR Compatible or Adafruit Feather Compatible board;
    qTop LTE BG95 AFC / AMCor qTop LTE BG96 AFC / AMC shield (IBT-QTC-AMC-BG95 / IBT-QTC-AMC-BG96 / 
        IBT-QTC-AFC-BG95 / IBT-QTC-AFC-BG95) with LTE and GNSS antennas connected;
    qJam BLE280 sensor module (IBT-QJS-BME280-0) : option;
    Nano SIM Card;
    LiPol Battery.

    * For qBoard-A or qBoard-B "ESP32 Dev Module" should be choosen as board in Arduino IDE.

  Project HW Option:
    You need to set these defines:
      - IOT_BOARD_TYPE : CPU Board type
      - QTOP_CELL_SHIELD_TYPE : qTop Modem Shield type
      - DUMP_AT_COMMANDS : Modem AT commands Logging

====================================================================================================
Revision History:

Author                          Date                Revision NUmber          Description of Changes
------------------------------------------------------------------------------------

iotbotscom                02/09/2021               1.0.0                        Initial release
iotbotscom                02/10/2021               1.0.1                        Set "Always On" Mode
iotbotscom                02/11/2021               1.0.2                        Added : Battery reading through GPIO, GSM Network time
iotbotscom                03/17/2021               1.0.3                        Support of Arduino MKR IOT boards and qTop LTE BG95 AMC (Arduino MKR Compatible) shield added
iotbotscom                05/03/2021               1.0.4                        Support of qBoardB (Adafruit Feather Compatible) IOT board and qTop LTE BG96 AFC (Adafruit Feather Compatible) shield added
iotbotscom                05/04/2021               1.0.5                        Support of qBoardA (Arduino MKR Compatible) IOT board and qTop LTE BG95 AMC (Arduino MKR Compatible) shield added
iotbotscom                05/07/2021               1.0.6                        GNSS polling / qBoard-B battery reading issues fixed
iotbotscom                05/20/2021               1.0.7                        GNSS position search order changed, put before GSM registration
iotbotscom                06/21/2021               1.0.8                        qTop-BG95-AMC Arduino MKR ZERO ThingsBoard Demo (tested on Zero, WiFi 1010, WAN 1300)
iotbotscom                06/22/2021               1.0.9                        qTop-BG96-AFC Huzzah ESP32 Adafruit Feather ThingsBoard Demo


*****************************************************************************/

// IOT Board options
#define IOT_BOARD_TYPE_HUZZAH32     1  // ESP32 Adafruit Feather IOT board
#define IOT_BOARD_TYPE_QBOARDA      2  // ESP32 Arduino MKR Compatible (AMC) IOT board
#define IOT_BOARD_TYPE_MKR          3  // Arduino MKR IOT board
#define IOT_BOARD_TYPE_QBOARDB      4  // ESP32 Adafruit Feather Compatible (AFC) IOT board
#define IOT_BOARD_TYPE_QBOARDX      5  // ESP32 Arduino MKR Compatible (AMC) QWARKS IOT board

// qTop Shield options
#define QTOP_CELL_SHIELD_TYPE_BG95  1
#define QTOP_CELL_SHIELD_TYPE_BG96  2


// Project HW option : qBoardA (Arduino MKR Compatible Board) + qTop BG95 AMC Shield
//#define IOT_BOARD_TYPE              IOT_BOARD_TYPE_QBOARDA
//#define QTOP_CELL_SHIELD_TYPE       QTOP_CELL_SHIELD_TYPE_BG95

// Project HW option : Arduino MKR Board + qTop BG95 AMC Shield
//#define IOT_BOARD_TYPE              IOT_BOARD_TYPE_MKR
//#define QTOP_CELL_SHIELD_TYPE       QTOP_CELL_SHIELD_TYPE_BG95

// Project HW option : qBoardB (Feather Compatible Board) + qTop BG96 AFC Shield
//#define IOT_BOARD_TYPE              IOT_BOARD_TYPE_QBOARDB
//#define QTOP_CELL_SHIELD_TYPE       QTOP_CELL_SHIELD_TYPE_BG96

// Project HW option : qBoardB (Feather Compatible Board) + qTop BG95 AMC Shield
//#define IOT_BOARD_TYPE              IOT_BOARD_TYPE_QBOARDB
//#define QTOP_CELL_SHIELD_TYPE       QTOP_CELL_SHIELD_TYPE_BG95

// Project HW option : Adafruit Feather Huzzah ESP32 Board + qTop BG96 AFC Shield
#define IOT_BOARD_TYPE              IOT_BOARD_TYPE_HUZZAH32
#define QTOP_CELL_SHIELD_TYPE       QTOP_CELL_SHIELD_TYPE_BG96


// Modem type
#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>

#include <ArduinoJson.h>
#include <Adafruit_BME280.h>

#if (defined(ESP32))
// ESP32 specific
#else
// AVR specific
#include "ArduinoLowPower.h"
#endif

///////////////////////////////////////////////////////////////////////////////
// Demo modes
#define MODE_ALWAYS_ON      1
#define MODE_ONE_SHOT       2

// Publish delay timeout for "Always On" mode
#define MODE_ALWAYS_ON_DELAY_TIMEOUT  5000 // 5s

// Publish delay (Device Sleep) timeout for "Always On" mode
#define MODE_ONE_SHOT_DELAY_TIMEOUT   10000//60000 // 60s

// Modem HW Pins
#if (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_HUZZAH32)
// Huzzah ESP32
#define MODEM_PWR_ON_PIN    13
#define MODEM_ON_PIN        32
#elif (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDA)
//qBoard-A
#define MODEM_PWR_ON_PIN    2
#define MODEM_ON_PIN        27
#elif (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDB)
//qBoard-B
#define MODEM_PWR_ON_PIN    13
#define MODEM_ON_PIN        32
#elif (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_MKR)
//Arduino MKR
#define MODEM_PWR_ON_PIN    20
#define MODEM_ON_PIN        6
#else
#error IOT_BOARD_TYPE is not defined, please define.
#endif

// Battery Voltage Pin
#if (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_HUZZAH32)
// Huzzah ESP32
#define BATTERY_PIN         35
#define BATTERY_K           ((2) * (3300.0) / 4096.0)
#elif (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDA)
//qBoard-A
#define BATTERY_EN_PIN      2
#define BATTERY_PIN         39
#define BATTERY_K           ((250.0 / 150.0) * (3300.0) / 4096.0)
#elif (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDB)
//qBoard-B
#define BATTERY_EN_PIN      2
#define BATTERY_PIN         39
#define BATTERY_K           ((250.0 / 150.0) * (3300.0) / 4096.0)
#elif (defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_MKR)
//Arduino MKR
#define BATTERY_PIN         ADC_BATTERY
#define BATTERY_K           ((1530.0 / 330.0) * (3300.0) / 4096.0)
#else
#error IOT_BOARD_TYPE is not defined, please define.
#endif

// GNSS Defs
#define GNSS_FIELDLEN_MAX  32

// Modem Serial
#if (defined(ESP32))
// ESP32 specific
HardwareSerial serialGSM(2);
#else
// AVR specific
HardwareSerial &serialGSM = Serial1;
#endif

// Uncomment to see StreamDebugger output in serial monitor
//#define DUMP_AT_COMMANDS 1
#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(serialGSM, Serial);
  TinyGsm modem(debugger);
#else
  // Initialize GSM modem
  TinyGsm modem(serialGSM);
#endif

// Initialize GSM client
TinyGsmClient client(modem);

// BME280 sensor
Adafruit_BME280 bme280;

// GPRS credentials
const char apn[]  = "wholesale";
const char user[] = "";
const char pass[] = "";

// See https://thingsboard.io/docs/getting-started-guides/helloworld/
// to understand how to obtain an access token
#define CLOUD_TOKEN   "pb7Vc2go03M2TmAdvzYe"
#define CLOUD_SERVER  "thingsboard.cloud"
#define CLOUD_PORT    80

// JSON & HTTP
DynamicJsonDocument jsonbuf(1024);
String reqtstr = "";

// Working Mode
int demo_mode = MODE_ALWAYS_ON;
//int demo_mode = MODE_ONE_SHOT;

// Devices status
bool is_modem_on = false;
bool is_sensor_on = false;
bool is_gnss_on = false;
bool is_gnss_ready = false;
bool is_modem_registered = false;
bool is_publish_done = false;

// Device GSM Data
int rssi = 0;

// Device Sensor Data
float temperature = 0;
float humidity = 0;
float pressure = 0;
int int_battery = 0;
float ext_battery = 0;

// Device GNSS Data
char quality[1+1];
char gnss_date[6+1];
char gnss_time[12+1];
char latitude[11+1];
char longitude[12+1];
char altitude[25+1];
char hdop[4+1];
char nsat[2+1];
char speed[25+1];
char heading[3+1];
int gnss_attempts = 0;

// Local Calls
void get_data(void );
bool publish_data(void );
void get_sensor_data(void );
void get_modem_data(void );
bool get_network_time(void );
bool get_gnss_data(void );
bool modem_register(void );
bool modem_check_registered(void );
bool open_connection(void );
bool close_connection(void );
bool publish_data(void );
bool modem_gnss_on(void );
bool modem_gnss_off(void );
bool modem_gnss_resume(void );
bool modem_gnss_suspend(void );
bool getfield(char * pBuf, char * pField, int idx, int len);
bool gnss_search(void );

#if (defined(ESP32))
// ESP32 specific
#else
// AVR specific
void dummy();
void(* restart)(void) = 0;
#endif
///////////////////////////////////////////////////////////////////////////////
void setup() {

  // Set console baud rate
  Serial.begin(115200);

#if (defined(ESP32))
  // ESP32 specific
#else
  // AVR specific
  LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, dummy, CHANGE);
#endif

  Serial.print("\r\n***************************************************************************************************\r\n");
  Serial.print("\r\n **** IOT-BOTS.COM **** \r\n");
  Serial.print("\r\n **** qTop Quectel BG9x Shield ThingsBoard Demo **** \r\n");
  Serial.print("\r\n***************************************************************************************************\r\n");

  Serial.println("\r\n---- Setup Started ----\r\n");

  if (!bme280.begin(BME280_ADDRESS_ALTERNATE)) {
    Serial.println("No BME280 Sensor Found!");
    is_sensor_on = false;
  } else {
/*
    bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                        Adafruit_BME280::SAMPLING_X1,
                        Adafruit_BME280::SAMPLING_X1,
                        Adafruit_BME280::SAMPLING_X1,
                        Adafruit_BME280::FILTER_OFF);
*/
    Serial.println("BME280 Sensor Found!");
    is_sensor_on = true;
  }

  pinMode(MODEM_PWR_ON_PIN, OUTPUT);
  digitalWrite(MODEM_PWR_ON_PIN, LOW);

  pinMode(MODEM_ON_PIN, OUTPUT);
  digitalWrite(MODEM_ON_PIN, LOW);

#if ((defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDB))
  pinMode(BATTERY_EN_PIN, OUTPUT);
  digitalWrite(BATTERY_EN_PIN, LOW);
#endif

  delay(3000);

  // Modem On Begin
  digitalWrite(MODEM_PWR_ON_PIN, HIGH);

  delay(1000);

  digitalWrite(MODEM_ON_PIN, HIGH);

  delay(1000);

  digitalWrite(MODEM_ON_PIN, LOW);
  // Modem On End

  delay(1000);

  // Set GSM module baud rate
  serialGSM.begin(115200);

  /**/
  Serial.println("\r\n Modem Initializing...");
  if (modem.begin()) {

    // Get Modem Info
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    String imei = modem.getIMEI();
    Serial.print("IMEI: ");
    Serial.println(imei);

    String imsi = modem.getIMSI();
    Serial.print("IMSI: ");
    Serial.println(imsi);

    String ccid = modem.getSimCCID();
    Serial.print("CCID: ");
    Serial.println(ccid);

    delay(3000);

    /* GNSS On and Search */
    gnss_search();

    is_modem_on = true;
  } else {
    Serial.println("No Modem Found");
    is_modem_on = false;
  }

  Serial.println("\r\n---- Setup Done ----");
}

///////////////////////////////////////////////////////////////////////////////
void loop() {

  Serial.println("\r\n---- Loop Cycle ----\r\n");

  if (is_modem_on != true) {
    Serial.println("No Modem Found");
    delay(1000);
    return;
  }

  if (is_modem_registered != true) {
    modem_gnss_suspend();

    modem_register();

    if (is_modem_registered == true) {
      modem_gnss_resume();
    }
  }
  else {
    if(is_gnss_ready == true || gnss_attempts++ > 5) {
        gnss_attempts = 0;

      /* Obtain Data to be publised */
      get_data();

      modem_gnss_suspend();

      modem_check_registered();

      if (is_modem_registered == true) {

        /* Connect to Cloud */
        if(open_connection() == true) {
          /* Data publising */
          publish_data();

          /* Disconnect from Cloud */
          //close_connection();
        }
      }

      modem_gnss_resume();

      /* Get GNSS Data, if available */
      get_gnss_data();
    }
    else {
      /* Get GNSS Data, if available */
      get_gnss_data();
    }
  }

  /* Wait or Sleep */
  if (demo_mode == MODE_ALWAYS_ON) {
    /* Always On mode - just wait and start new Data Obtain-Publish cycle again */
    delay(MODE_ALWAYS_ON_DELAY_TIMEOUT);
  } else if ((demo_mode == MODE_ONE_SHOT) && (is_publish_done == true)) {
    /* One Shot mode */

    /* Disconnect from Cloud */
    close_connection();

    /* Disconnect from GPRS */
    modem.gprsDisconnect();
    if (!modem.isGprsConnected()) {
      Serial.println("GPRS disconnected");
    }

    /* GNSS Off */
    modem_gnss_off();

    /* Modem Off */
    modem.poweroff();
    digitalWrite(MODEM_PWR_ON_PIN, LOW);
    is_modem_on = false;
    Serial.println("Modem Off");

    // BME280 : Nothing to be done - sensor works in Forced Mode : Issue found : TBD
    //Serial.println("Turn BME280 Off");

    /* Move Core to Deep Sleep */
    Serial.println("Move core to Deep Sleep");
#if (defined(ESP32))
    // ESP32 specific
    esp_sleep_enable_timer_wakeup(MODE_ONE_SHOT_DELAY_TIMEOUT * 1000);
    esp_deep_sleep_start();
#else
    // AVR specific
    LowPower.sleep(MODE_ONE_SHOT_DELAY_TIMEOUT);

    restart();

    // Never returns from Low Power - Need to Investigate
#endif

    Serial.println("Moved out from Deep Sleep : Something wrong...");
  }
}

bool modem_register(void ) {

  /* Connect to GSM Network */
  if (!modem.isNetworkConnected()) {
    Serial.print("Waiting for GSM network...");
    if (!modem.waitForNetwork()) {
      Serial.println(" failed");
      return false;
    }

    if (modem.isNetworkConnected()) {
      Serial.println(" : Network connected");
    }
  }
  else
  {
    if (modem.isNetworkConnected()) {
      Serial.println("GSM Network : Connected");
    }
  }

  /* Connect to GPRS */
  if (!modem.isGprsConnected()) {
    Serial.print("Connecting to \"");
    Serial.print(apn);
    Serial.print("\"...");
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println(" failed");
      return false;
    }

    if (modem.isGprsConnected()) {
      Serial.println(" : GPRS connected");
    }
  }
  else
  {
    if (modem.isNetworkConnected()) {
      Serial.println("GPRS Network : Connected");
    }
  }

  /* Get Network Time */
  get_network_time();

  is_modem_registered = true;

  return true;
}

bool modem_check_registered(void ) {

  /* Check GSM Network */
  if (!modem.isNetworkConnected()) {
    Serial.print("Not registered in GSM network");
    is_modem_registered = false;
    return false;
  }

  /* Check GPRS Network*/
  if (!modem.isGprsConnected()) {
    Serial.print("Not registered in GPRS network");
    is_modem_registered = false;
    return false;
  }

  return true;
}

void get_data(void ) {

  /* Obtain BME280 reading */
  get_sensor_data();
  
  /* Get RSSI and Battery info */
  get_modem_data();
}

bool open_connection(void ) {
  /* Connect to Cloud */
  if (!client.connected()) {
    Serial.print("Connecting to \"");
    Serial.print(CLOUD_SERVER);
    Serial.print("\"...");
    if (!client.connect(CLOUD_SERVER, CLOUD_PORT)) {
      Serial.println(" failed");
      return false;
    }
    else {
      Serial.println(" : Cloud connected");
    }
  }

  return true;
}

bool close_connection(void ) {
  /* Disconnect from Cloud */
  if (client.connected()) {
    client.stop();
    Serial.println("Cloud disconnected");
  }

  return true;
}

bool publish_data(void ) {

  String cloud_token(CLOUD_TOKEN);
  String cloud_server(CLOUD_SERVER);
  String jsonstr = "";
  int jsonlen = 0;

  jsonlen = 0;

  /* Compose JSON */
  jsonbuf["Bat.voltage"] = int_battery;
  jsonbuf["Ext.voltage"] = ext_battery;
  jsonbuf["RSSI"] = rssi;

  if (is_sensor_on) {
    jsonbuf["Temperature"] = temperature;
    jsonbuf["Humidity"] = humidity;
    jsonbuf["Pressure"] = pressure;
  }

  if (is_gnss_ready) {
    jsonbuf["latitude"] = latitude;
    jsonbuf["longitude"] = longitude;
    jsonbuf["altitude"] = altitude;
    jsonbuf["speed"] = speed;
    jsonbuf["heading"] = heading;
    jsonbuf["nsat"] = nsat;
    jsonbuf["hdop"] = hdop;

    jsonbuf["date"] = gnss_date;
    jsonbuf["time"] = gnss_time;
  }

  /* Get JSON Str*/
  serializeJson(jsonbuf, jsonstr);
  jsonlen = jsonstr.length();

  /* Compose HTTP POST request */
  reqtstr = "POST /api/v1/" + cloud_token + "/telemetry HTTP/1.1\r\nHost: " + cloud_server + "\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: " + jsonlen + "\r\n\r\n" + jsonstr + "\r\n\r\n";

  Serial.println("\r\nHTTP Request: ");
  Serial.println(reqtstr);

  /* Send HTTP POST request */
  client.print(reqtstr);

  Serial.println("\r\nHTTP Reply: ");

  // Wait for data to arrive
  uint32_t start = millis();
  while (client.connected() && !client.available() &&
         millis() - start < 10000L) {
    delay(100);
  };

  if (client.available()) {
    // Read data
    start = millis();
    while (client.connected() && millis() - start < 5000L) {
      while (client.available()) {
        Serial.write(client.read());
        start = millis();
      }
    }
    Serial.println();

    is_publish_done = true;

    return true;
  } else {
    Serial.println("...No Reply received");

    return false;
  }
}

void get_sensor_data(void ) {

  Serial.println("\r\nBattery: ");

  //get and print battery voltage

#if ((defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDB))
  digitalWrite(BATTERY_EN_PIN, HIGH);
  delay(500);
#endif
  Serial.print(" Voltage, mV: ");
  int_battery = (int)(BATTERY_K * (float)analogRead(BATTERY_PIN));
  Serial.println(int_battery);
  Serial.print(" Voltage, %: ");
  int_battery = (int)((((float)int_battery - 3400.0) / (4200.0 - 3400.0)) * 100.0);
  if(int_battery > 100)
  {
    int_battery = 100;
  }
  Serial.println(int_battery);
#if ((defined IOT_BOARD_TYPE) && (IOT_BOARD_TYPE == IOT_BOARD_TYPE_QBOARDB))
  digitalWrite(BATTERY_EN_PIN, LOW);
#endif


  Serial.println("\r\nBME280 Sensor: ");

  if (is_sensor_on) {
    //get and print temperatures
    Serial.print(" Temperature, F: ");
    temperature = bme280.readTemperature();
    temperature = ((temperature * 9 / 5) + 32);
    Serial.println(temperature);

    //get and print atmospheric pressure data
    Serial.print(" Pressure, hPa: ");
    Serial.println(pressure = bme280.readPressure() / 100.0);

    //get and print humidity data
    Serial.print(" Humidity, %: ");
    Serial.println(humidity = bme280.readHumidity());
  } else {
    Serial.println("...No Data Available!");
  }
}

void get_modem_data(void ) {

  uint8_t chargeState = 0;
  int8_t percent     = 0;
  uint16_t milliVolts  = 0;

  // Get Modem Voltage
  Serial.println("\r\nModem Power: ");

  modem.getBattStats(chargeState, percent, milliVolts);
  //Serial.print(" Voltage, %: ");
  //Serial.println((int)percent);
  Serial.print(" Voltage, mV: ");
  Serial.println((int)milliVolts);

  // Get RSSI
  Serial.println("\r\nSignal Quality: ");

  rssi = modem.getSignalQuality();
  Serial.print(" RSSI: ");
  Serial.println(rssi);
}

bool get_network_time(void ) {

  int gsm_year = 0;
  int gsm_month = 0;
  int gsm_day = 0;
  int gsm_hour = 0;
  int gsm_min = 0;
  int gsm_sec = 0;
  float gsm_timezone = 0;

  Serial.println("Get Network Time :");
  if (modem.getNetworkTime(&gsm_year, &gsm_month, &gsm_day, &gsm_hour, &gsm_min, &gsm_sec, &gsm_timezone)) {
    Serial.print(" Year : ");
    Serial.println(gsm_year);

    Serial.print(" Month : ");
    Serial.println(gsm_month);

    Serial.print(" Day : ");
    Serial.println(gsm_day);

    Serial.print(" Hour : ");
    Serial.println(gsm_hour);

    Serial.print(" Minute : ");
    Serial.println(gsm_min);

    Serial.print(" Second : ");
    Serial.println(gsm_sec);

    Serial.print(" Time Zone : ");
    Serial.println(gsm_timezone);

    return true;
  } else {
    Serial.println(" failed");

    return false;
  }
}

bool gnss_search(void ) {

  int timer = 0;

  //Serial.print("GNSS Search ");

  modem_gnss_on();

  while(get_gnss_data() != true && timer++ < 120) {
    //Serial.println(".");
    delay(1000);
  }

  if(is_gnss_ready == true) {
    //Serial.println(": Position Fixed");

    return true;
  } else {
    //Serial.println(": Failed");

    return false;
  }
}

bool get_gnss_data(void ) {

  char pField[32];
  char buf[128];
  unsigned int len = 0;
  unsigned int i;

  // Get GNSS Data
  Serial.println("\r\nGNSS: ");

  String gnss_str = modem.getGPSraw();

  len = gnss_str.length();
  if (len) {
    gnss_str.getBytes((byte *)buf, len + 1);

    //Serial.println(buf);

    if (getfield(buf, pField, 0, GNSS_FIELDLEN_MAX)) {
        strcpy(gnss_time, (char *)&pField);
        gnss_time[6] = 0;
    }

    if (getfield(buf, pField, 1, GNSS_FIELDLEN_MAX)) {
        strcpy(latitude, (char *)pField);
        latitude[9] = 0;
    }

    if (getfield(buf, pField, 2, GNSS_FIELDLEN_MAX)) {
        strcpy(longitude, (char *)pField);
        longitude[10] = 0;
    }

    if (getfield(buf, pField, 3, GNSS_FIELDLEN_MAX)) {
        strcpy(hdop, (char *)pField);
        hdop[4] = 0;
    }

    if (getfield(buf, pField, 4, GNSS_FIELDLEN_MAX)) {
        i = 0;
        len = strlen((char *)pField);
        while(pField[i] != '.' && i < len)
        {
            i++;
        }
        pField[i] = 0;
        strcpy(altitude, (char *)pField);
    }

    if (getfield(buf, pField, 5, GNSS_FIELDLEN_MAX)) {
        strcpy(quality, (char *)pField);
        quality[1] = 0;
    }

    if (getfield(buf, pField, 6, GNSS_FIELDLEN_MAX)) {
        i = 0;
        while(pField[i] != '.' && i < len)
        {
            i++;
        }
        pField[i] = 0;

        strcpy(heading, (char *)pField);
    }

    if (getfield(buf, pField, 7, GNSS_FIELDLEN_MAX)) {
        i = 0;
        len = strlen((char *)pField);
        while(pField[i] != '.' && i < len)
        {
            i++;
        }
        pField[i] = 0;

        strcpy(speed, (char *)pField);
    }

    if (getfield(buf, pField, 9, GNSS_FIELDLEN_MAX)) {
        strcpy(gnss_date, (char *)pField);
        gnss_date[6] = 0;
    }

    if (getfield(buf, pField, 10, GNSS_FIELDLEN_MAX)) {
        pField[2] = 0;
        strcpy(nsat, (char *)pField);
    }

    Serial.print(" Quality : ");
    Serial.println((char *)quality);

    Serial.print(" Date : ");
    Serial.println((char *)gnss_date);

    Serial.print(" Time : ");
    Serial.println((char *)gnss_time);

    Serial.print(" Latitude : ");
    Serial.println((char *)latitude);

    Serial.print(" Longitude : ");
    Serial.println((char *)longitude);

    Serial.print(" Altitude, m : ");
    Serial.println((char *)altitude);

    Serial.print(" Speed, km/h : ");
    Serial.println((char *)speed);

    Serial.print(" Heading : ");
    Serial.println((char *)heading);

    Serial.print(" HDOP : ");
    Serial.println((char *)hdop);

    Serial.print(" Sats In Use : ");
    Serial.println((char *)nsat);

    is_gnss_ready = true;

    return true;
  } else {
    Serial.println(" ...No Data Available!");

    is_gnss_ready = false;

    return false;
  }
}

bool modem_gnss_on(void ) {

  int attempts = 3;

#if ((defined QTOP_CELL_SHIELD_TYPE) && (QTOP_CELL_SHIELD_TYPE == QTOP_CELL_SHIELD_TYPE_BG95))
  modem.sendAT(GF("+QGPSCFG=\"priority\",0,0\r\n"));
  if (modem.waitResponse() == 1) {
    delay(3000);
    modem.sendAT(GF("+QGPS=1\r\n"));
    if (modem.waitResponse() == 1) {
      is_gnss_on = true;
      return true;
    }
    return false;
  } else {
    return false;
  }
#else
    modem.sendAT(GF("+QGPS=1\r\n"));
    if (modem.waitResponse() == 1) {
      is_gnss_on = true;
      return true;
    }
    return false;
#endif
}

bool modem_gnss_off(void ) {

  modem.sendAT(GF("+QGPSEND\r\n"));
  if (modem.waitResponse() == 1) {
    return true;
  } else {
    return false;
  }
}

bool modem_gnss_resume(void ) {

#if ((defined QTOP_CELL_SHIELD_TYPE) && (QTOP_CELL_SHIELD_TYPE == QTOP_CELL_SHIELD_TYPE_BG95))
  modem.sendAT(GF("+QGPSCFG=\"priority\",0,0\r\n"));
  if (modem.waitResponse() == 1) {
    delay(1000);
    if (is_gnss_on != true) {
      modem.sendAT(GF("+QGPS=1\r\n"));
      if (modem.waitResponse() == 1) {
        is_gnss_on = true;
        return true;
      }
      else {
        return false;
      }
    }
    else {
      return true;
    }
  }
  return false;
#else
  return true;
#endif
}

bool modem_gnss_suspend(void ) {

#if ((defined QTOP_CELL_SHIELD_TYPE) && (QTOP_CELL_SHIELD_TYPE == QTOP_CELL_SHIELD_TYPE_BG95))
  modem.sendAT(GF("+QGPSCFG=\"priority\",1,0\r\n"));
  if (modem.waitResponse() == 1) {
    delay(1000);
    return true;
  } else {
    return false;
  }
#else
  return true;
#endif
}

bool getfield(char * pBuf, char * pField, int idx, int len) {
  int i = 0;
  int j = 0;
  int nField = 0;

  if (pBuf == NULL || pField == NULL || len <= 0) {
      return false;
  }

  while (nField != idx && pBuf[i]) {
      if (pBuf[i] == ',') {
          nField++;
      }

      i++;

      if (pBuf[i] == '\0') {
          pField[0] = '\0';
          return false;
      }
  }

  if (pBuf[i] == ',') {
      pField[0] = '\0';
      return false;
  }

  while (pBuf[i] != ',' && pBuf[i] != '*' && pBuf[i]) {
      pField[j] = pBuf[i];
      j++; i++;

      if (j >= len) {
          j = len-1;
          break;
      }
  }
  pField[j] = '\0';

  return true;
}

#if (defined(ESP32))
// ESP32 specific
#else
// AVR specific
void dummy() {
  //alarm_source = 0;
}
#endif

