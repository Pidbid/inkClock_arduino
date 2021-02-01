#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
//ezTime 时间模块
#include <ezTime.h>
#include "Solar2Lunar.h"
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"
#include "DHTesp.h"
GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(/*CS=D8*/ SS, /*DC=D3*/ 4, /*RST=D4*/ 10, /*BUSY=D2*/ 5)); // GDEH029A1
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
Timezone myTZ;
Solar2Lunar S2L;
DHTesp dht;
HTTPClient http;

ESP8266WebServer server(80); //创建一个webserver
const char *ap_ssid = "inkPaper";
const char *ap_password = ""; //开放式网络
boolean FirstOpen = false;
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
struct wifi_config
{
  char wifissid[32];
  char wifipsd[64];
};
struct weather_now
{
  char nowtemp[32];
  char text[32];
  char mintemp[32];
  char maxtemp[32];
};
wifi_config wifiConfig;
weather_now weatherData;
void setup()
{
  //开始联网授时
  Serial.begin(115200);
  initWifi(); //授时部分
  display.init();
  display.setRotation(3);
  u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
  leftLunarPaint();         //左侧黑色区域绘制
  timeInit();               //初始化时间获取
  displayLunarData();       //显示左侧阴历
  displaySolarData();       //显示顶部阳历
  displayWenshiIcon();      //显示温湿度图标
  showtime();
  displaySecond(myTZ.dateTime("s").toInt(), myTZ.dateTime("H").toInt());
  displayWenshiInit(); //初始化dht11
  displayWenshi();
  //ESP.deepSleep(1e6);
}
void loop()
{
  //timeLoop();
  //判断是否跟新农历以及阳历数据
  timeLoop();
  if (myTZ.dateTime("i").toInt() % 15 == 0 && myTZ.dateTime("s").toInt() <= 2)
  {
    displayLunarData();
    displaySolarData();
    Serial.println("fresh lunar data");
  }
  if (myTZ.dateTime("s").toInt() == 58)
  {
    Serial.println("fresh lunar data");
  }
  //循环显示秒
  if (myTZ.dateTime("s").toInt() % 10 == 0)
  {
    displaySecond(myTZ.dateTime("s").toInt(), myTZ.dateTime("H").toInt());
    //循环获得温湿度数据显示
    displayWenshi();
  }
  delay(900);
}

void showtime()
{
  //Serial.println("helloWorld");
  u8g2Fonts.setFontMode(1);                        // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                   // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);       // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);       // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_7Segments_26x42_mn); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.setPartialWindow(85, 39, 160, 42);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setCursor(85, 85); // start writing at this position
    String nowTime = myTZ.dateTime("H:i");
    u8g2Fonts.print(nowTime);
  } while (display.nextPage());
}

void initWifi()
{
  loadConfig();
  WiFi.begin(wifiConfig.wifissid, wifiConfig.wifipsd);
  delay(5000);
  if (WiFi.isConnected() != true)
  {
    initApConfig();
    serverInit();
    while (WiFi.status() != WL_CONNECTED)
    {
      server.handleClient();
      delay(300);
      Serial.print(".");
    }
    WiFi.softAPdisconnect(); //连接成功后关闭AP
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiConfig.wifissid, wifiConfig.wifipsd);
    saveConfig();
  }
}

/**
 * 初始化AP配置
 */
void initApConfig()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);
}

void serverInit()
{
  server.on("/", HTTP_GET, handleRootPost);
  server.onNotFound(handleNotFound);
  server.begin();
}

void handleNotFound()
{
  String wrongs = "no";
  server.send(200, "text/html", wrongs);
}

/**
 * 处理web post请求
 */
void handleRootPost()
{
  if (server.hasArg("ssid"))
  {
    strcpy(wifiConfig.wifissid, server.arg("ssid").c_str());
  }
  else
  {
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");
    return;
  }
  if (server.hasArg("password"))
  {
    strcpy(wifiConfig.wifipsd, server.arg("password").c_str());
  }
  else
  {
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }
  WiFi.begin(wifiConfig.wifissid, wifiConfig.wifipsd);
  Serial.println(wifiConfig.wifissid);
  Serial.println(wifiConfig.wifipsd);
}
/*
 * 保存参数到EEPROM
*/
void saveConfig()
{
  Serial.println("Save config!");
  Serial.print("wifissid:");
  Serial.println(wifiConfig.wifissid);
  Serial.print("wifipsd:");
  Serial.println(wifiConfig.wifipsd);

  EEPROM.begin(1024);
  uint8_t *p = (uint8_t *)(&wifiConfig);
  for (int i = 0; i < sizeof(wifiConfig); i++)
  {
    EEPROM.write(i, *(p + i));
  }
  EEPROM.commit();
}

/*
 * 从EEPROM加载参数
*/
void loadConfig()
{
  EEPROM.begin(1024);
  uint8_t *p = (uint8_t *)(&wifiConfig);
  for (int i = 0; i < sizeof(wifiConfig); i++)
  {
    *(p + i) = EEPROM.read(i);
  }
  EEPROM.commit();
  Serial.println("-----Read config-----");
  Serial.print("wifissid:");
  Serial.println(wifiConfig.wifissid);
  Serial.print("wifipsd:");
  Serial.println(wifiConfig.wifipsd);
}

void showTime()
{
  //Serial.println("helloWorld");
  u8g2Fonts.setFontMode(1);                        // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                   // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);       // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);       // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_7Segments_26x42_mn); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.setPartialWindow(85, 39, 218, 81);
  display.firstPage();
  for (int i = 0; i <= 100; i++)
  {
    do
    {
      display.fillScreen(GxEPD_WHITE);
      u8g2Fonts.setCursor(90, 85); // start writing at this position
      String nowTime = myTZ.dateTime("H:i");
      u8g2Fonts.print(nowTime);
    } while (display.nextPage());
  }
}

void timeInit()
{
  //setServer("ntp1.aliyun.com");
  //updateNTP();
  setDebug(INFO);
  //myTZ.setLocation(F("Asia/Shanghai"));
  Serial.println("set utc +8");
  myTZ.setPosix("UTC -8");
  Serial.println("set utc +8 finished");
  Serial.println(myTZ.dateTime());
  waitForSync();
  Serial.println(myTZ.dateTime());
  String timeString = myTZ.dateTime("YmdHisvN");
  Serial.println(timeString);
  Serial.println("get time finished");
}

void leftLunarPaint()
{
  //display.setRotation(3);
  display.setPartialWindow(0, 0, 80, 128);
  display.firstPage();
  do
  {
    //display.fillScreen(0xFFFF);
    display.fillRect(0, 0, 80, 128, 0x0000);
  } while (display.nextPage());
}

void displayLunarData()
{
  //获取数据
  String solarData = myTZ.dateTime("Ymd");
  char *tiangan[] = {"甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸"};
  char *dizhi[] = {"子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥"};
  char *ri[] = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十"};
  char *yueBig[] = {"壹", "贰", "叁", "肆", "伍", "陆", "柒", "捌", "玖", "拾"};
  unsigned short int lunarYear = S2L.LunarYear(solarData.substring(0, 4).toInt(), solarData.substring(4, 6).toInt(), solarData.substring(6, 8).toInt());
  unsigned short int tiangan_num = lunarYear % 10;
  unsigned short int dizhi_num = lunarYear % 12;
  Serial.println(tiangan[tiangan_num]);
  String out_print = String(tiangan[tiangan_num]) + String(dizhi[dizhi_num]) + "年";

  //out_print.concat(dizhi.substring(dizhi_num,dizhi_num+1));
  Serial.println(out_print);
  //u8g2_font_ink_epaper
  u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);             // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_WHITE); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_ink_epaper);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  //显示年
  display.setPartialWindow(0, 0, 80, 25);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_BLACK);
    u8g2Fonts.setCursor(0, 20); // start writing at this position
    u8g2Fonts.print(out_print);
  } while (display.nextPage());
  //显示月
  unsigned short int lunarMonth = S2L.LunarMonth(solarData.substring(0, 4).toInt(), solarData.substring(4, 6).toInt(), solarData.substring(6, 8).toInt());
  String lunarYue;
  if (lunarMonth > 10)
  {
    lunarYue = "拾" + String(yueBig[lunarMonth - 10 - 1]) + "月";
  }
  else
  {
    lunarYue = String(yueBig[lunarMonth - 1]) + "月";
  }
  display.setPartialWindow(0, 25, 80, 50);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_BLACK);
    u8g2Fonts.setCursor(0, 45); // start writing at this position
    u8g2Fonts.print(lunarYue);
  } while (display.nextPage());
  //显示日
  unsigned short int lunarDay = S2L.LunarDay(solarData.substring(0, 4).toInt(), solarData.substring(4, 6).toInt(), solarData.substring(6, 8).toInt());
  String lunarRi;
  if (lunarDay <= 10)
  {
    lunarRi = String(ri[lunarDay - 1]) + "日";
  }
  else if (lunarDay > 10 && lunarDay < 20)
  {
    lunarRi = "拾" + String(ri[lunarDay % 10 - 1]) + "日";
  }
  else if (lunarDay == 20)
  {
    lunarRi = "贰拾日";
  }
  else if (lunarDay > 20 && lunarDay < 30)
  {
    lunarRi = "贰拾" + String(ri[lunarDay % 10 - 1]) + "日";
  }
  else if (lunarDay == 30)
  {
    lunarRi = "叁拾日";
  }
  else
  {
    lunarRi = "叁拾" + String(ri[lunarDay % 10 - 1]) + "日";
  }
  display.setPartialWindow(0, 50, 80, 75);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_BLACK);
    u8g2Fonts.setCursor(0, 70); // start writing at this position
    u8g2Fonts.print(lunarRi);
  } while (display.nextPage());
  //显示时辰
  int shi_now = myTZ.dateTime("H").toInt();
  int jisuan = String(shi_now / 2).toInt() + shi_now % 2;
  String shichen = String(dizhi[jisuan % 12]) + "时";
  int ke_biaozhi = shi_now % 2;
  int fen_now = myTZ.dateTime("i").toInt() / 15;
  String bake;
  if (ke_biaozhi == 1)
  {
    bake = String(ri[fen_now]) + "刻";
  }
  else
  {
    bake = String(ri[fen_now + 4]) + "刻";
  }
  display.setPartialWindow(0, 75, 80, 100);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_BLACK);
    u8g2Fonts.setCursor(0, 95); // start writing at this position
    u8g2Fonts.print(shichen + bake);
  } while (display.nextPage());
  //显示八刻
}

void displaySolarData()
{
  u8g2Fonts.setFontMode(1);                     // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);    // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);    // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_ink_epaper_yuan); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.setPartialWindow(85, 0, 190, 25);
  String timeString = myTZ.dateTime("YmdN");
  char *xingqi[] = {"一", "二", "三", "四", "五", "六", "日"};
  String show_now = timeString.substring(0, 4) + "年" + timeString.substring(4, 6) + "月" + timeString.substring(6, 8) + "日周" + String(xingqi[myTZ.dateTime("N").toInt() - 1]);
  Serial.println(timeString);
  Serial.println(timeString.substring(9).toInt());
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setCursor(85, 25); // start writing at this position
    u8g2Fonts.print(show_now);
  } while (display.nextPage());
}

void displaySecond(int now_second, int hour)
{
  //fonts  u8g2_font_open_iconic_weather_2x_t
  display.setPartialWindow(280, 0, 16, 128);
  int display_part = now_second / 10 + 1;
  u8g2Fonts.setFont(u8g2_font_open_iconic_weather_2x_t);
  //Serial.println(display_part);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    if (hour > 0 && hour <= 8)
    {
      u8g2Fonts.drawGlyph(280, display_part * 25 - 5 * (display_part - 1), 0x0042);
    }
    else
    {
      u8g2Fonts.drawGlyph(280, display_part * 25 - 5 * (display_part - 1), 0x0045);
    }

  } while (display.nextPage());
}

void displayWenshiInit()
{
  dht.setup(2, DHTesp::DHT11); //dht11初始化
}

void displayWenshi()
{
  //delay(dht.getMinimumSamplingPeriod());
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  String display_humidity = String(humidity).substring(0, 4);
  String display_temperature = String(temperature).substring(0, 4);
  //String display_computeHeatIndex = String(dht.computeHeatIndex(temperature, humidity, false));
  //设置显示区域
  u8g2Fonts.setFontMode(1); // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  u8g2Fonts.setFont(u8g2_font_ink_epaper_number);
  display.setPartialWindow(115, 95, 140, 43);
  display.firstPage();
  String final_display = display_temperature + "℃ " + display_humidity + "%";
  //Serial.println(final_display);
  do
  {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setCursor(115, 125);
    u8g2Fonts.print(final_display);
  } while (display.nextPage());
}

void displayWenshiIcon()
{
  //显示湿度图标的函数
  display.setPartialWindow(90, 100, 28, 28);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  u8g2Fonts.setFont(u8g2_font_open_iconic_all_2x_t);
  display.firstPage();
  do
  {
    //display.fillScreen(GxEPD_WHITE);
    //u8g2Fonts.setCursor(101, 101);
    u8g2Fonts.drawGlyph(90, 125, 0x0098);
  } while (display.nextPage());
}

void timeLoop()
{
  //Serial.println("helloWorld");
  u8g2Fonts.setFontMode(1);                        // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                   // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);       // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);       // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_7Segments_26x42_mn); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.setPartialWindow(85, 39, 160, 42);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setCursor(85, 85); // start writing at this position
    String nowTime = myTZ.dateTime("H:i");
    u8g2Fonts.print(nowTime);
  } while (display.nextPage());
}
