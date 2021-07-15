//导入相关的库
#include<ArduinoJson.h>
#include<SoftwareSerial.h>
#include<ESP8266WiFi.h>
#include<Adafruit_MQTT.h>
#include<Adafruit_MQTT_Client.h>

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "iQOO 7"                 //你的WI-FI的SSID，注意把你的 WIFI 的 AP 设置成 2.4GHz 频段。
#define WLAN_PASS       "1234567899"          //你的WI-FI的密码

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                                //use 8883 for SSL
#define AIO_USERNAME    "coderzzx"                            //你在io.adafruit.com上注册的用户名
#define AIO_KEY         "aio_FVAz59rsIgM6vRHWNTbX3h6ceAch"  //你在io.adafruit.com所获得的AIO

/*
   定义nodeMCU引脚编号
*/
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13

#define LED_PIN D4

/************************************************实例化串口通信的对象****************************************************/
SoftwareSerial nodemcuSerial(D6, D5);

/**********************************************用于将数据发布到服务器上***************************************************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish t_rel = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature");
Adafruit_MQTT_Publish h_rel = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidity");
Adafruit_MQTT_Publish c_rel = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/CO Density");
Adafruit_MQTT_Publish p_rel = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/PM2.5");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Switch");

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
  int8_t retries = 10;

  // Stop if already connected.
  if ( mqtt.connected() )
  {
    return;
  }

  Serial.println("Connecting to MQTT... ");

  while ( (ret = mqtt.connect()) != 0 )                       // connect will return 0 for connected
  {
    Serial.println( mqtt.connectErrorString(ret) );
    Serial.println("Retrying MQTT connection in 5 seconds...");

    mqtt.disconnect();
    delay(5000);

    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }

  Serial.println("MQTT Connected!");
}

/***************************************************数据发布函数*********************************************************/
void infoRelea(float x[])
{
  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here
  Adafruit_MQTT_Subscribe *subscription;

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  int flag;
  while ((subscription = mqtt.readSubscription(5000)))
  {
    if (subscription == &onoffbutton)
    {
      Serial.print(F("Got: "));
      String value = (char *)onoffbutton.lastread;

      Serial.println(value);
      if (!value.compareTo("1"))
      {
        digitalWrite(LED_PIN, HIGH);
        flag = 1;
      }
      if (!value.compareTo("0"))
      {
        digitalWrite(LED_PIN, LOW);
        flag = 0;
        return;
      }
    }
  }

  // Now we can publish stuff!
  int i;
  for (i = 0; i < 4; i++)
  {
    switch (i)
    {
      case 0:
        if (! t_rel.publish(x[0]))
        {
          Serial.println(F("Failed"));
        }
        else
        {
          Serial.println(F("OK!"));
        }
        break;

      case 1:
        if (! h_rel.publish(x[1]))
        {
          Serial.println(F("Failed"));
        }
        else
        {
          Serial.println(F("OK!"));
        }
        break;

      case 2:
        if (! c_rel.publish(x[2]))
        {
          Serial.println(F("Failed"));
        }
        else
        {
          Serial.println(F("OK!"));
        }
        break;

      case 3:
        if (! p_rel.publish(x[3]))
        {
          Serial.println(F("Failed"));
        }
        else
        {
          Serial.println(F("OK!"));
        }
    }
  }
}

//各个初始化操作
void setup()
{
  Serial.begin(115200);
  while (!Serial) continue;

  nodemcuSerial.begin(9600);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);

  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  /************************************************用于从串口中获取数据****************************************************/
  const size_t CAPACITY = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<CAPACITY> doc;

  DeserializationError error = deserializeJson(doc, nodemcuSerial);

  //    if (error)
  //    {
  //    Serial.print(F("deserializeJson() failed: "));
  //    Serial.println(error.f_str());
  //    return;
  //    }

  JsonObject root = doc.as<JsonObject>();
  //获取数据
  /*
    t:温度
    h:湿度
    val:一氧化碳浓度
    d:PM2.5浓度
  */
  float temp = (float)root["temp"];
  float humi = (float)root["humi"];
  float val = (float)root["val"];
  float dens = (float)root["dens"];

  float data[4] = {temp, humi, val, dens};
  //    for (int i = 0; i < 4; i++)
  //    Serial.println(data[i]);
  /***********************************************************************************************************************/
  infoRelea(data);
}