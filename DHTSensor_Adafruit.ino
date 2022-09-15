/*********************************************************************
  This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

  This example is for a 128x64 size display using I2C to communicate
  3 pins are required to interface (2 I2C and one reset)

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada  for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above, and the splash screen must be included in any redistribution

  Libraries needed:
  Time.h & TimeLib.h:  https://github.com/PaulStoffregen/Time
  Timezone.h: https://github.com/JChristensen/Timezone
  SSD1306.h & SSD1306Wire.h:  https://github.com/squix78/esp8266-oled-ssd1306
  NTPClient.h: https://github.com/arduino-libraries/NTPClient
  ESP8266WiFi.h & WifiUDP.h: https://github.com/ekstrand/ESP8266wifi

*********************************************************************/
#define DISPLAY_ATTACHED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EepromUtil.h>
#include <ESPConfig.h>
#include <ESP8266Controller.h>
#include "DHTSensor_Adafruit.h"
#include <String.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
//#include <Timezone.h>

// Uncomment the type of sensor in use:
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
#define DHTPIN 3 // Digital pin connected to the DHT sensor

DHT dht(DHTPIN, DHTTYPE);

WiFiUDP serverUdp;
ESPConfig configuration(/*name*/ "DHT11", /*location*/ "Unknown", /*firmware ver*/ "dht11c.13082022.bin", /*SSID*/ "onion", /*SSID key*/ "242374666");
DHTSensor_Adafruit sensor(/*name*/ "Temperature", /*pin DHTPIN*/ LED_BUILTIN, /*no. of capabilities*/ 4, configuration.sizeOfEEPROM());

// Define NTP properties
#define NTP_OFFSET 60 * 60 * 5.5      // In seconds IST = 5.5 + UTC
#define NTP_INTERVAL 60 * 1000        // In miliseconds
#define NTP_ADDRESS "ca.pool.ntp.org" // change this to whatever pool is closest (see ntp.org)

// Set up the NTP UDP client
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

String date = "Sunday, 10 Aug 2022";
String tyme = "10:00";
String postmeridian = "pm";
const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *ampm[] = {"am", "pm"};

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//#define OLED_RESET   -1     // define SSD1306 OLED reset (TX pin)
// Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

// maximum bytes for sensor = 201 bytes. UDP_TX_PACKET_MAX_SIZE max size of UDP packet
const int SIZE_OF_PACKET_BUFFER = 255 * 3;
byte packetBuffer[SIZE_OF_PACKET_BUFFER];
byte replyBuffer[SIZE_OF_PACKET_BUFFER];
short replyBufferSize = 0;

void setup()
{
  //delay(2000);
  Serial.begin(115200);
  DEBUG_PRINTLN("setup begin..");

  configuration.init(-1);
  /*WiFi.begin("onion","242374666");
    DEBUG_PRINTLN("Connecting.");
    while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    DEBUG_PRINT(".");
    }
    DEBUG_PRINTLN("connected");*/
  dht.begin();
  DEBUG_PRINTLN("dht.begin");
  Wire.begin(2, 0); // set I2C pins (SDA = GPIO2, SCL = GPIO0), default clock is 100kHz
  // Wire.setClock(400000L);   // uncomment this to set I2C clock to 400kHz
  DEBUG_PRINTLN("Wire.begin");

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
#ifdef DISPLAY_ATTACHED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
  DEBUG_PRINTLN("display.begin");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  DEBUG_PRINTLN("display.display");
  //delay(2000);

  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("connecting...");
  display.display();
#endif

  timeClient.begin(); // Start the NTP UDP client
  DEBUG_PRINTLN("timeClient.begin");

  // load sensor values from EEPROM
  sensor.loadCapabilities();

  // drawStuff();
  DEBUG_PRINTLN("setup complete!");
#ifdef DISPLAY_ATTACHED
  display.clearDisplay();
#endif

  serverUdp.begin(port);
}

unsigned long last = 0;
int packetSize = 0;
_udp_packet udpPacket;
int heap_print_interval = 10000;
byte _pin = 10;
int count = 0;
float temperature = 0;
float humidity = 0;
unsigned long currentMillis = millis();
//int dht11ReadInterval = 5000;
unsigned long lastDHT11Read = 0;
unsigned long lastNTPRead = 0;
unsigned long localTime = 0;

void loop()
{

  currentMillis = millis();

  // read temperature and humidity
  if (currentMillis - lastDHT11Read > sensor.getInterval())
  {

    lastDHT11Read = currentMillis;
    DEBUG_PRINT("read temperature and humidity..");

    // read temperature and humidity
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (isnan(humidity) || isnan(temperature))
    {
      DEBUG_PRINTLN("Failed to read from DHT sensor!");
    } else {
    }

      sensor.setTemperature(temperature);
      sensor.setHumidity(humidity);
    // read the time
    if (WiFi.status() == WL_CONNECTED) // Check WiFi connection status
    {
      //DEBUG_PRINTLN("read time and date");
      date = ""; // clear the variables
      tyme = "";
      postmeridian = "";

      // update the NTP client and get the UNIX UTC timestamp
      timeClient.update();
      localTime = timeClient.getEpochTime();

      // convert received time stamp to time_t object
      time_t local = localTime;

      // now format the Time variables into strings with proper names for month, day etc
      date += days[weekday(local) - 1];
      date += ", ";
      date += day(local);
      date += " ";
      date += months[month(local) - 1];
      date += " ";
      date += year(local);

      // format the time to 12-hour format with AM/PM and no seconds
      tyme += hourFormat12(local);
      tyme += ":";
      if (minute(local) < 10) // add a zero if minute is under 10
        tyme += "0";
      tyme += minute(local);
      //tyme += " ";
      postmeridian += ampm[isPM(local)];

      // Display the date and time
      DEBUG_PRINTLN("");
      DEBUG_PRINT("Local date/time: "); DEBUG_PRINT(date); DEBUG_PRINT(", "); DEBUG_PRINT(tyme); DEBUG_PRINT(" "); DEBUG_PRINTLN(postmeridian);

      lastNTPRead = currentMillis;

#ifdef DISPLAY_ATTACHED
      display.clearDisplay();
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print(tyme);
      display.setTextSize(1);
      display.print(postmeridian);
      display.println();display.println();display.println();display.println();
      display.println(date);

      // display temperature
      DEBUG_PRINT("Temperature: "); DEBUG_PRINT(temperature); DEBUG_PRINT("C  ");
      display.println();
      display.setTextSize(2);
      display.print(temperature);
      display.setTextSize(1);
      display.cp437(true);
      display.write(167);
      display.print("C");

      // display humidity
      DEBUG_PRINT("Humidity: "); DEBUG_PRINT(humidity); DEBUG_PRINTLN(" %");
      display.setTextSize(2);
      display.print(humidity);
      display.setTextSize(1);
      display.println("%");

      display.display();
      // if (millis() - last > heap_print_interval) {
      // DEBUG_PRINTLN();
      // last = millis();
      // DEBUG_PRINT("[MAIN] Free heap: ");
      // DEBUG_PRINTLN(ESP.getFreeHeap(), DEC);
#endif
    }
    else // attempt to connect to wifi again if disconnected
    {
      DEBUG_PRINT("Wifi disconnected");
      lastNTPRead = 0;
#ifdef DISPLAY_ATTACHED
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("Wifi disconnected");
      display.display();
      // WiFi.begin(ssid, password);
      // display.drawString(0, 24, "Connected.");
      // display.display();
      // delay(1000);
#endif
    }

  }

  packetSize = serverUdp.parsePacket();

  if (packetSize)
  {

    // read the packet into packetBuffer
    int packetLength = serverUdp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = 0;

    // DEBUG_PRINTLN(++count);
    DEBUG_PRINTLN();
    DEBUG_PRINT("Received packet of packetSize ");
    DEBUG_PRINT(packetSize);
    DEBUG_PRINT(" packetLength ");
    DEBUG_PRINT(packetLength);
    DEBUG_PRINT(" from ");
    DEBUG_PRINT(serverUdp.remoteIP());
    DEBUG_PRINT(", port ");
    DEBUG_PRINTLN(serverUdp.remotePort());

    // printArray(packetBuffer, packetSize, false);

    // initialize the replyBuffer and replyBufferSize
    memset(replyBuffer, 0, SIZE_OF_PACKET_BUFFER);
    memcpy(replyBuffer, packetBuffer, 3);
    replyBufferSize = 3;

    // prepare the UDP header from buffer
    udpPacket._size = packetBuffer[1] << 8 | packetBuffer[0];
    udpPacket._command = packetBuffer[2];
    udpPacket._payload = (char *)packetBuffer + 3;

    _pin = udpPacket._payload[0];

    if (udpPacket._command == DEVICE_COMMAND_DISCOVER)
    {
      DEBUG_PRINTLN("Command: DEVICE_COMMAND_DISCOVER");

      // replyBufferSize += configuration.discover(replyBuffer+3);
      replyBufferSize += configuration.toByteArray(replyBuffer + replyBufferSize);
    }
    else if (udpPacket._command == DEVICE_COMMAND_SET_CONFIGURATION)
    {
      DEBUG_PRINTLN("Command: DEVICE_COMMAND_SET_CONFIGURATION");

      replyBufferSize += configuration.set(replyBuffer + 3, (byte *)udpPacket._payload);
    }
    else if (udpPacket._command == DEVICE_COMMAND_GET_CONTROLLER)
    {
      DEBUG_PRINTLN("Command: DEVICE_COMMAND_GET_CONTROLLER");

      if (_pin == sensor.pin)
      {
        replyBufferSize += sensor.toByteArray(replyBuffer + replyBufferSize);
      }
    }
    else if (udpPacket._command == DEVICE_COMMAND_SET_CONTROLLER)
    {
      DEBUG_PRINTLN("Command: DEVICE_COMMAND_SET_CONTROLLER");

      if (_pin == sensor.pin)
      {
        sensor.fromByteArray((byte *)udpPacket._payload);
      }
    }
    else if (udpPacket._command == DEVICE_COMMAND_GETALL_CONTROLLER)
    {
      DEBUG_PRINTLN("Command: DEVICE_COMMAND_GETALL_CONTROLLER");

      replyBufferSize += sensor.toByteArray(replyBuffer + replyBufferSize);
    }
    else if (udpPacket._command == DEVICE_COMMAND_SETALL_CONTROLLER)
    {
      DEBUG_PRINTLN("Command: DEVICE_COMMAND_SETALL_CONTROLLER");

      int i = 0;

      // update the sensor variables with new values
      for (int count = 0; count < 1; count++)
      {

        if (udpPacket._payload[i] == sensor.pin)
        {
          sensor.fromByteArray((byte *)udpPacket._payload + i);

          i = i + sensor.sizeOfEEPROM();
        }
      }

      // (OVERRIDE) send 3 bytes (size, command) as reply to client
      // replyBufferSize = 3;
    }

    // update the size of replyBuffer in packet bytes
    replyBuffer[0] = lowByte(replyBufferSize);
    replyBuffer[1] = highByte(replyBufferSize);

    // send a reply, to the IP address and port that sent us the packet we received
    serverUdp.beginPacket(serverUdp.remoteIP(), serverUdp.remotePort());
    serverUdp.write(replyBuffer, replyBufferSize);
    serverUdp.endPacket();
  }

  sensor.loop();

  yield();
}
/*
  void refreshDisplay()
  {

  // display time and date
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(tyme);
  display.setTextSize(1);
  display.println();
  display.println(date);

  // display temperature and humidity

  // display.setTextSize(1);
  // display.setTextColor(WHITE);
  // display.setCursor(0, 0);
  // display.println("Hello, Guunu & Star!");
  //  display.setTextColor(BLACK, WHITE); // 'inverted' text
  //  display.println(3.141592);
  //  display.setTextSize(2);
  // display.setTextColor(WHITE);
  //  display.print("0x"); display.println(0xDEADBEEF, HEX);
  // display.println();
  // display.println("...wassup");
  // display.println();
  // display.println("Wolfu & TSf !");
  // display.setTextSize(1);
  // display.println();
  // display.println();
  // display.println("creator: jovistar");
  display.display();
  delay(5000);
  display.clearDisplay();

  // display temperature

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  //display.print("Temperature: ");
  //display.setTextSize(1);
  //display.setCursor(0, 10);
  display.print(temperature);
  display.print(" ");
  //display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  //display.setTextSize(1);
  display.print("C  ");
  display.println();

  // display humidity
  //display.setTextSize(1);
  //display.setCursor(0, 35);
  //display.print("Humidity: ");
  //display.setTextSize(1);
  //display.setCursor(0, 45);
  display.print(humidity);
  display.println(" %");
  display.println();

  display.display();
  delay(5000);
  }

  void drawStuff()
  {

    // draw a single pixel
    display.drawPixel(10, 10, WHITE);
    // Show the display buffer on the hardware.
    // NOTE: You _must_ call display after making any drawing commands
    // to make them visible on the display hardware!
    display.display();
    delay(2000);
    display.clearDisplay();

    // draw many lines
    testdrawline();
    display.display();
    delay(2000);
    display.clearDisplay();

    // draw rectangles
    testdrawrect();
    display.display();
    delay(2000);
    display.clearDisplay();

    // draw multiple rectangles
    testfillrect();
    display.display();
    delay(2000);
    display.clearDisplay();

    // draw mulitple circles
    testdrawcircle();
    display.display();
    delay(2000);
    display.clearDisplay();

    // draw a white circle, 10 pixel radius
    display.fillCircle(display.width()/2, display.height()/2, 10, WHITE);
    display.display();
    delay(2000);
    display.clearDisplay();

    testdrawroundrect();
    delay(2000);
    display.clearDisplay();

    testfillroundrect();
    delay(2000);
    display.clearDisplay();

    testdrawtriangle();
    delay(2000);
    display.clearDisplay();

    testfilltriangle();
    delay(2000);
    display.clearDisplay();

    // draw the first ~12 characters in the font
    testdrawchar();
    display.display();
    delay(2000);
    display.clearDisplay();

    // draw scrolling text
    testscrolltext();
    delay(2000);
    display.clearDisplay();

    // text display tests
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Hello, world!");
    display.setTextColor(BLACK, WHITE); // 'inverted' text
    display.println(3.141592);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.print("0x"); display.println(0xDEADBEEF, HEX);
    display.display();
    delay(2000);
    display.clearDisplay();

    // miniature bitmap display
    display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
    display.display();
    delay(1);

    // invert the display
    display.invertDisplay(true);
    delay(1000);
    display.invertDisplay(false);
    delay(1000);
    display.clearDisplay();

    // draw a bitmap icon and 'animate' movement
    testdrawbitmap(logo16_glcd_bmp, LOGO16_GLCD_HEIGHT, LOGO16_GLCD_WIDTH);
  }

  void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h)
  {
  uint8_t icons[NUMFLAKES][3];

  // initialize
  for (uint8_t f = 0; f < NUMFLAKES; f++)
  {
    icons[f][XPOS] = random(display.width());
    icons[f][YPOS] = 0;
    icons[f][DELTAY] = random(5) + 1;

    DEBUG_PRINT("x: ");
    DEBUG_PRINTXY(icons[f][XPOS], DEC);
    DEBUG_PRINT(" y: ");
    DEBUG_PRINTXY(icons[f][YPOS], DEC);
    DEBUG_PRINT(" dy: ");
    DEBUG_PRINTXY(icons[f][DELTAY], DEC);
    DEBUG_PRINTLN();
  }

  while (1)
  {
    // draw each icon
    for (uint8_t f = 0; f < NUMFLAKES; f++)
    {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }
    display.display();
    delay(200);

    // then erase it + move it
    for (uint8_t f = 0; f < NUMFLAKES; f++)
    {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, BLACK);
      // move it
      icons[f][YPOS] += icons[f][DELTAY];
      // if its gone, reinit
      if (icons[f][YPOS] > display.height())
      {
        icons[f][XPOS] = random(display.width());
        icons[f][YPOS] = 0;
        icons[f][DELTAY] = random(5) + 1;
      }
    }
  }
  }

  void testdrawchar(void)
  {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  for (uint8_t i = 0; i < 168; i++)
  {
    if (i == '\n')
      continue;
    display.write(i);
    if ((i > 0) && (i % 21 == 0))
      display.println();
  }
  display.display();
  delay(1);
  }

  void testdrawcircle(void)
  {
  for (int16_t i = 0; i < display.height(); i += 2)
  {
    display.drawCircle(display.width() / 2, display.height() / 2, i, WHITE);
    display.display();
    delay(1);
  }
  }

  void testfillrect(void)
  {
  uint8_t color = 1;
  for (int16_t i = 0; i < display.height() / 2; i += 3)
  {
    // alternate colors
    display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, color % 2);
    display.display();
    delay(1);
    color++;
  }
  }

  void testdrawtriangle(void)
  {
  for (int16_t i = 0; i < min(display.width(), display.height()) / 2; i += 5)
  {
    display.drawTriangle(display.width() / 2, display.height() / 2 - i,
                         display.width() / 2 - i, display.height() / 2 + i,
                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
    display.display();
    delay(1);
  }
  }

  void testfilltriangle(void)
  {
  uint8_t color = WHITE;
  for (int16_t i = min(display.width(), display.height()) / 2; i > 0; i -= 5)
  {
    display.fillTriangle(display.width() / 2, display.height() / 2 - i,
                         display.width() / 2 - i, display.height() / 2 + i,
                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
    if (color == WHITE)
      color = BLACK;
    else
      color = WHITE;
    display.display();
    delay(1);
  }
  }

  void testdrawroundrect(void)
  {
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2)
  {
    display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, WHITE);
    display.display();
    delay(1);
  }
  }

  void testfillroundrect(void)
  {
  uint8_t color = WHITE;
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2)
  {
    display.fillRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, color);
    if (color == WHITE)
      color = BLACK;
    else
      color = WHITE;
    display.display();
    delay(1);
  }
  }

  void testdrawrect(void)
  {
  for (int16_t i = 0; i < display.height() / 2; i += 2)
  {
    display.drawRect(i, i, display.width() - 2 * i, display.height() - 2 * i, WHITE);
    display.display();
    delay(1);
  }
  }

  void testdrawline()
  {
  for (int16_t i = 0; i < display.width(); i += 4)
  {
    display.drawLine(0, 0, i, display.height() - 1, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = 0; i < display.height(); i += 4)
  {
    display.drawLine(0, 0, display.width() - 1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i = 0; i < display.width(); i += 4)
  {
    display.drawLine(0, display.height() - 1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = display.height() - 1; i >= 0; i -= 4)
  {
    display.drawLine(0, display.height() - 1, display.width() - 1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i = display.width() - 1; i >= 0; i -= 4)
  {
    display.drawLine(display.width() - 1, display.height() - 1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = display.height() - 1; i >= 0; i -= 4)
  {
    display.drawLine(display.width() - 1, display.height() - 1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i = 0; i < display.height(); i += 4)
  {
    display.drawLine(display.width() - 1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = 0; i < display.width(); i += 4)
  {
    display.drawLine(display.width() - 1, 0, i, display.height() - 1, WHITE);
    display.display();
    delay(1);
  }
  delay(250);
  }

  void testscrolltext(void)
  {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.clearDisplay();
  display.println("scroll");
  display.display();
  delay(1);

  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  }*/
