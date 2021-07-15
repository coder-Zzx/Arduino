//导入需要用到的液晶显示库、dht11库、json库与串口通讯库
#include<LiquidCrystal.h>
#include<dht11.h>
#include<ArduinoJson.h>
#include<SoftwareSerial.h>

//预编译各个引脚
#define DHTPIN 9
#define INFOUT 7
#define MQPIN 6

//初始化各种对象
/*
   lcd的各接口：
   rs:12
   e:11
   d4:5
   d5:4
   d6:3
   d7:2
*/
LiquidCrystal lcd(12, 13, 5, 4, 3, 2);
SoftwareSerial ArduinoSerial(11, 10);
dht11 DHT11;

//PM2.5传感器处理函数
float PM()
{
  //设置PM2.5传感器相关引脚与变量
  int measurePin = A0;
  int ledPower = 8;

  unsigned int samplingTime = 280; //3脚直接由单片机给脉冲波10ms周期，高电平0.32ms
  unsigned int deltaTime = 40;
  unsigned int sleepTime = 9680;

  float voMeasured = 0;
  float calcVoltage = 0;
  float dustDensity = 0;

  digitalWrite(ledPower, LOW);
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin);

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH);
  delayMicroseconds(sleepTime);

  calcVoltage = voMeasured * 3.0 / 1023.0;
  dustDensity = 0.172 * calcVoltage - 0.1;

  if (dustDensity < 0)
  {
    dustDensity = 0.00;
  }

  Serial.println();
  Serial.print("Raw Signal Value (0-1023):");
  Serial.println(voMeasured);

  Serial.print(" - Voltage: ");
  Serial.println(calcVoltage);

  Serial.print(" - Dust Density: ");
  Serial.print(dustDensity * 1000);
  Serial.println(" ug/m3");

  delay(1000);

  //返回实际的浓度值，单位ug/m3
  return dustDensity * 1000;
}

//初始化各种设置
void setup() {
  // put your setup code here, to run once:
  //启动串口通讯
  ArduinoSerial.begin(9600);
  Serial.begin(115200);
  //初始化液晶屏各种设置
  lcd.begin(16, 2);
  lcd.print("Welcome!");
  delay(1000);
  lcd.clear();
  //设置小灯泡的引脚
  pinMode(INFOUT, OUTPUT);
}
/********************与DHT11传感器相关的数据获取********************/
float getTemp()
{
  int chk = DHT11.read(DHTPIN);
  return (float)DHT11.temperature;
}

float getHumi()
{
  int chk = DHT11.read(DHTPIN);
  return (float)DHT11.humidity;
}
/****************************************************************/

/******************分别在液晶屏和串口显示数据的函数*****************/
void printOnLCD(float t, float h, int val, float d)
{
  lcd.print("tempareture:");
  lcd.setCursor(0, 1);
  lcd.print(t);
  delay(2000);
  lcd.clear();
  lcd.print("humidity:");
  lcd.setCursor(0, 1);
  lcd.print(h);
  delay(2000);
  lcd.clear();
  lcd.print("mq9:");
  lcd.setCursor(0, 1);
  lcd.print(val);
  delay(2000);
  lcd.clear();
  lcd.print("Dust Density:");
  lcd.setCursor(0, 1);
  lcd.print(d);
  delay(2000);
  lcd.clear();
}

void printOnSerial(float t, float h, int val)
{
  Serial.println("current temperature(`C):");
  Serial.println(t);
  Serial.println("current humidity(%):");
  Serial.println(h);
  Serial.println("data from MQ-9:");
  Serial.println(val);
}
/********************************************************/

/***********************警报函数*************************/
void warning(float d)
{
  if (d > 700)//如果PM2.5浓度大于700ug/m3就输出高电平，使蜂鸣器振动，LED灯亮
  {
    int i;
    for (i = 0; i < 20; i++)
    {
      digitalWrite(INFOUT, HIGH);
      delay(200);
      digitalWrite(INFOUT, LOW);
      delay(200);
    }
  }
}
/*******************************************************/

// void infoTrans(float t,float h,float v,float d)
// {
//    StaticJsonDocument<500> doc;
//  JsonObject root = doc.to<JsonObject>();

//  root["temp"] = t;
//  root["humi"] = h;
//  root["val"] = v;
//  root["dens"] = d;

//  serializeJson(doc,ArduinoSerial);
// }
//测试中去掉的数据传输函数

void loop() {
  // put your main code here, to run repeatedly:

/************将数据转换成json格式并发送出去**************/
  const size_t CAPACITY = JSON_OBJECT_SIZE(20);
/*
 *t:温度
 *h:湿度
 *d:PM2.5浓度
 *val:CO浓度
 */
  float t = getTemp();
  float h = getHumi();
  float d = PM();
  float val = analogRead(MQPIN);

  //将数据通过串口通信传输给nodeMCU.
  StaticJsonDocument<CAPACITY> doc;
  JsonObject root = doc.to<JsonObject>();

  root["temp"] = t;
  root["humi"] = h;
  root["val"] = val;
  root["dens"] = d;

  serializeJson(doc, ArduinoSerial);
/*****************************************************/

  printOnLCD(t, h, val, d);
  printOnSerial(t, h, val);
  warning(d);
}