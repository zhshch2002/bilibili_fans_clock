#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "SH1106Wire.h"

//----------------------------------------------------
const char* ssid     = "";     //WiFi名
const char* password = ""; //WiFi密码
String biliuid       = "";           //bilibili UID
//----------------------------------------------------

const long utcOffsetInSeconds = 3600 * 8;
String daysOfTheWeek[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
String Time, Date;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

DynamicJsonDocument jsonBuffer(400);
WiFiClient client;

SH1106Wire  display(0x3c, D3, D5);
// D3 -> SDA
// D5 -> SCL

long fans = 0;
int times = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("bilibili fans clock,init...");

  display.init();
  display.flipScreenVertically();

  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 16, "Loading...");
  display.display();

  Serial.print("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
}

void loop() {
  if (times == 0) {
    getTime();
    display.clear();
    display.setFont(ArialMT_Plain_24);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 12, Time);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 40, Date);
    display.display();
  }

  if (times == 4) {
    getBilibiliFans();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 16, "FANS:" + String(fans));
    display.display();
  }

  times += 1;
  if (times >= 7) {
    times = 0;
  }
  delay(1000);
}

void getTime() {
  timeClient.update();//获取时间
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int secounds = timeClient.getSeconds();
  int week = timeClient.getDay();
  long Epoch = timeClient.getEpochTime();


  Time = hours < 10 ? "0" + String(hours) : String(hours);//格式化时间字符串
  Time += ":";
  Time += minutes < 10 ? "0" + String(minutes) : String(minutes);
  Date = daysOfTheWeek[week];
  Date += " ";

  int MON1[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};  //平年
  int MON2[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //闰年
  int FOURYEARS = (366 + 365 + 365 + 365); //每个四年的总天数
  int DAYMS = 24 * 3600;

  int nDays = Epoch / DAYMS + 1; //time函数获取的是从1970年以来的毫秒数，因此需要先得到天数
  int nYear4 = nDays / FOURYEARS; //得到从1970年以来的周期（4年）的次数
  int nRemain = nDays % FOURYEARS; //得到不足一个周期的天数
  int nDesYear = 1970 + nYear4 * 4;
  int nDesMonth = 0, nDesDay = 0;
  bool bLeapYear = false;

  if ( nRemain < (365 + 365) ) //一个周期内，第二年
  { //平年
    nDesYear += 1;
    nRemain -= 365;
  }
  else if ( nRemain < (365 + 365 + 365) ) //一个周期内，第三年
  { //平年
    nDesYear += 2;
    nRemain -= (365 + 365);
  }
  else//一个周期内，第四年，这一年是闰年
  { //润年
    nDesYear += 3;
    nRemain -= (365 + 365 + 365);
    bLeapYear = true;
  }

  int *pMonths = bLeapYear ? MON2 : MON1;
  //循环减去12个月中每个月的天数，直到剩余天数小于等于0，就找到了对应的月份
  for ( int i = 0; i < 12; ++i )
  {
    int nTemp = nRemain - pMonths[i];
    if ( nTemp <= 0 )
    {
      nDesMonth = i + 1;
      if ( nTemp == 0 )//表示刚好是这个月的最后一天，那么天数就是这个月的总天数了
        nDesDay = pMonths[i];
      else
        nDesDay = nRemain;
      break;
    }
    nRemain = nTemp;
  }

  Date += String(nDesMonth);
  Date += "-";
  Date += String(nDesDay);
  /*Serial.print("Time");
    Serial.print(nDesYear);
    Serial.print(nDesMonth);
    Serial.println(nDesDay);*/
}

void getBilibiliFans() {
  if (WiFi.status() == WL_CONNECTED) //此处感谢av30129522视频作者的开源代码
  {
    HTTPClient http;
    http.begin("http://api.bilibili.com/x/relation/stat?vmid=" + biliuid);
    auto httpCode = http.GET();
    Serial.print("Bilibili fans HttpCode:");
    Serial.print(httpCode);
    if (httpCode > 0) {
      String resBuff = http.getString();
      DeserializationError error = deserializeJson(jsonBuffer, resBuff);
      if (error) {
        Serial.println("json error");
        while (1);
      }
      JsonObject root = jsonBuffer.as<JsonObject>();
      long code = root["code"];
      Serial.print("     Code:");
      Serial.print(code);
      fans = root["data"]["follower"];
      Serial.print("     Fans:");
      Serial.println(fans);
    }
  }
}
