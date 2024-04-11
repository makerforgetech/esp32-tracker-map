/*******************************************************************
 *  An example of receiving location Data
 *
 *
 *  By Brian Lough
 *  
 *  Add .ZIP library of Universal-Arduino-Telegram-Bot.zip
 *  Add ArduinoJSON library
 *  Add Wifi and BOT TOKEN (See https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/blob/master/README.md)
 *  Compile and deploy
 *  Share your location with the bot, you should get coordinates returned in serial monitor and chat.
 *  
 *******************************************************************/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

#include "secure.h"
#include "config.h"

unsigned long bot_lasttime;          // last time messages' scan has been done
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

#define PANEL_RES_X 96      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 48     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1      // Total number of panels chained one to another
 
//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

void setup()
{
  screenSetup();
  telegramSetup();
}

void loop()
{
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}

void telegramSetup() {
  Serial.begin(115200);
  Serial.println();

  // attempt to connect to Wifi network:
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  // clear screen
  dma_display->drawRect(0, 0, dma_display->width(), dma_display->height(), myWHITE);
  delay(500);

  drawHome();
}

void drawHome()
{
  // clear screen
  dma_display->fillRect(0, 0, dma_display->width(), dma_display->height(), myBLACK);
  delay(500);

  Serial.println("Mapping Home Location to LEDs");
  mapLocationToLEDs(HOME_LAT, HOME_LON, "");
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    Serial.println(bot.messages[i].from_id);
    
    // ignore messages from users not in whitelist
    if (bot.messages[i].from_id != MY_ID){
      continue;
    }
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (bot.messages[i].longitude != 0 || bot.messages[i].latitude != 0)
    {
      Serial.print("From: ");
      Serial.println(from_name);
      Serial.print("Lat: ");
      Serial.println(String(bot.messages[i].latitude, 6));
      Serial.print("Long: ");
      Serial.println(String(bot.messages[i].longitude, 6));
      
      String message = "From: " + from_name + "\n";
      message += "Lat: " + String(bot.messages[i].latitude, 6) + "\n";
      message += "Long: " + String(bot.messages[i].longitude, 6) + "\n";
      bot.sendMessage(chat_id, message, "Markdown");

      drawHome();

      mapLocationToLEDs(bot.messages[i].latitude, bot.messages[i].longitude, chat_id);
    }
    else if (text == "/start")
    {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
      welcome += "Share a location or a live location and the bot will respond with the co-ords\n";

      bot.sendMessage(chat_id, welcome, "Markdown");
    }

    // if text contains latitude and longitude separated by a comma
    else if (text.indexOf(",") > 0)
    {
      int commaIndex = text.indexOf(",");
      String lat = text.substring(0, commaIndex);
      String lon = text.substring(commaIndex + 1);

      Serial.println("Manual Location");
      Serial.print("Lat: ");
      Serial.println(lat);
      Serial.print("Lon: ");
      Serial.println(lon);

      mapLocationToLEDs(lat.toFloat(), lon.toFloat(), chat_id);
    }
    else
    {
      String message = "I'm sorry, I don't understand that command. Please share a location or a live location and I will respond with the co-ords\n";
      bot.sendMessage(chat_id, message, "Markdown");
    }
  }
}
void mapLocationToLEDs(float lat, float lon, String chat_id)
{
  // Convert latitude and longitude to LED row and column, convert to radians as needed
  // I tried a direct mapping initially but it didn't give the right results. Converting to percentage first seems better
  int latPercentage = (lat - START_LAT) / (END_LAT - START_LAT) * 100;
  int lonPercentage = (lon - START_LON) / (END_LON - START_LON) * 100;
  Serial.print("Lat %: ");
  Serial.println(latPercentage);
  Serial.print("Lon %: ");
  Serial.println(lonPercentage);
 
  // Map LED ROW using define LED_ROWS and map against START_LAT and END_LAT
  int row = map(latPercentage, 0, 100, 0, LED_ROWS-1);
  // Map LED COL using define LED_COLS and map against START_LON and END_LON
  int col = map(lonPercentage, 0, 100, 0, LED_COLS-1);

  // Ensure the row and col are within the LED bounds
  row = constrain(row, 0, LED_ROWS-1);
  col = constrain(col, 0, LED_COLS-1);

  Serial.print("Row: ");
  Serial.println(row);
  Serial.print("Col: ");
  Serial.println(col);

  // display on screen
  dma_display->drawPixel(row, col, myWHITE);
  delay(500);

  if (chat_id == "")
    return;

  String message = "LED: " + String(row) + ", " + String(col) + "\n";
  bot.sendMessage(chat_id, message, "Markdown");
}

void screenSetup() {

  // Module configuration
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // module width
    PANEL_RES_Y,   // module height
    PANEL_CHAIN    // Chain length
  );

  mxconfig.gpio.e = 32;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(255); //0-255
  dma_display->clearScreen();
}
