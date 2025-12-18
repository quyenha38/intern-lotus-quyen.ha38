#include "global.h"

#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "tinyml.h"


//#define WLAN_SSID "RD-SEAI_2.4G" //uncomment to run ohstem and adafruit
#define WLAN_SSID "ESP32" //uncomment to run webserver
#define WLAN_PASS ""

//uncomment to run adafruit
// #define AIO_SERVER      "io.adafruit.com"
// #define AIO_SERVERPORT  1883
// #define AIO_USERNAME    "quyenha38"
// #define AIO_KEY         ""

//uncomment to run coreiot
const char* mqtt_server = "app.coreiot.io";
const int mqtt_port = 1883;
const char* access_token = "ESP322";

//uncomment to run ohstem
// #define OHS_SERVER      "mqtt.ohstem.vn"
// #define OHS_SERVERPORT  1883
// #define OHS_USERNAME    "ohstem"
// #define OHS_KEY         ""

// Define ports
#define D5 GPIO_NUM_8
#define D3 GPIO_NUM_6
#define A1 GPIO_NUM_2
#define A0 GPIO_NUM_1

 uint16_t soilid = 0;

 //MQTT Feeds for adafruit
// WiFiClient client;
// Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
// Adafruit_MQTT_Subscribe timefeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");
// Adafruit_MQTT_Subscribe slider = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/slider", MQTT_QOS_1); //slider need more research
// Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/V1", MQTT_QOS_1);
// Adafruit_MQTT_Publish sensory = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/V20");
// Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
// Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
// Adafruit_MQTT_Publish lightFeed= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/light");
// Adafruit_MQTT_Publish soilMoistureFeed= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/soilmoisture");

// MQTT Feeds for ohstem 
// Adafruit_MQTT_Client mqtt(&client, OHS_SERVER, OHS_SERVERPORT, OHS_USERNAME, OHS_USERNAME, OHS_KEY); //for Ohstem
// Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V2");
// Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V3");
// Adafruit_MQTT_Publish soilMoistureFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V4");
// Adafruit_MQTT_Publish lightFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V5");
// Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, OHS_USERNAME "/feeds/V1", MQTT_QOS_1);

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void TaskWebServer(void *pvParameters); //uncomment to run webserver

//uncomment to run coreiot

// Define your components here
Adafruit_NeoPixel pixels3(4, D5, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);
//uncomment to run coreiot
WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastTelemetry = 0;
const unsigned long telemetryInterval = 10000;
const float LATITUDE  = 10.772175109674038;   
const float LONGITUDE = 106.65789107082472;  

// HTTP Server
AsyncWebServer server(80);

// void slidercallback(double x) {
//   Serial.print("Slider value: ");
//   Serial.println(x);

//   int brightness = map(x, 0, 100, 0, 255);
//   ledcWrite(0, brightness); 
// }
bool ledState = false;  // LED status
#define LIGHT_PIN GPIO_NUM_48

//uncomment to run ohstem and adafruit
// void onoffcallback(char *data, uint16_t len) {
//   Serial.print("Button value: ");
//   Serial.println(data);

//   if (strcmp(data, "ON") == 0) {
//       ledState = true;
//       digitalWrite(LIGHT_PIN, HIGH);  // Bật đèn
//   } else if (strcmp(data, "OFF") == 0) {
//       ledState = false;
//       digitalWrite(LIGHT_PIN, LOW);  // Tắt đèn
//   }
// }

//uncomment to run adafruit and ohstem
// void MQTT_connect() {
//   int8_t ret;
//   if (mqtt.connected()) return;
  
//   Serial.print("Connecting to MQTT... ");
//   uint8_t retries = 3;
//   while ((ret = mqtt.connect()) != 0) {
//     Serial.println(mqtt.connectErrorString(ret));
//     Serial.println("Retrying MQTT connection in 10 seconds...");
//     mqtt.disconnect();
//     delay(10000);
//     retries--;
//     if (retries == 0) while (1);
//   }
//   Serial.println("MQTT Connected!");
// }
void onRpcMessage(char* topic, byte* payload, unsigned int length) {
  Serial.println("RPC message received!");
  Serial.print("Topic: "); 
  Serial.println(topic);
  Serial.print("Payload: "); 
  Serial.write(payload, length); 
  Serial.println();
  Serial.flush();

  payload[length] = '\0';
  String msg = String((char*)payload);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.f_str());
    return;
  }

  String method = doc["method"];
  JsonVariant params = doc["params"];  
  Serial.println("Method: " + method);
  Serial.println("Params: " + String(params.as<String>()));

  String request_id = String(topic).substring(String(topic).lastIndexOf("/") + 1);

  if (method == "setLED") {
    ledState = params.as<bool>();
    digitalWrite(LIGHT_PIN, ledState ? HIGH : LOW);
    Serial.println(ledState ? "LED ON" : "LED OFF");
  } 
  else if (method == "getLED") {
    mqttClient.publish(
      ("v1/devices/me/rpc/response/" + request_id).c_str(),
      ledState ? "true" : "false"
    );
  }
}

//uncomment to run coreiot
void MQTT_connect() {
  if (mqttClient.connected()) return;

  Serial.print("Connecting to MQTT...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect("YOLO_UNO", "YOLO_UNO", "YOLO_UNO")) {
      Serial.println("Connected!");
      mqttClient.setCallback(onRpcMessage);                
      mqttClient.subscribe("v1/devices/me/rpc/request/+"); 
    } else {
      Serial.print("Connection failed, error code: ");
      delay(2000);
    }
  }
}



void sendTelemetry() {
    if(!mqttClient.connected()) return;

    dht20.read();
    float temp = dht20.getTemperature();
    float hum = dht20.getHumidity();
    int lightRaw = analogRead(A1);    // giá trị thô
    int soilValue = analogRead(A0);

    if(!isnan(temp) && !isnan(hum)){
        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"temperature\":%.2f,"
                 "\"humidity\":%.2f,"
                 "\"light\":%d,"
                 "\"soilMoisture\":%d,"
                 "\"latitude\":%.6f,"
                 "\"longitude\":%.6f}",
                 temp, hum, lightRaw, soilValue,
                 LATITUDE, LONGITUDE);
        
        mqttClient.publish("v1/devices/me/telemetry", payload);
        Serial.println("Telemetry sent: " + String(payload));
    } else {
        Serial.println("Failed to read DHT20 sensor!");
    }
}



void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  Serial.begin(115200); 
  Wire.begin(GPIO_NUM_11, GPIO_NUM_12);
  dht20.begin();
  lcd.begin();
  pixels3.begin();
  ledcSetup(0, 5000, 8); 
  ledcAttachPin(GPIO_NUM_48, 0);
WiFi.softAP(WLAN_SSID); //uncomment to run webserver
//pio run -t uploadfs to flash  data to esp32

//uncomment to run coreiot
  mqttClient.setServer(mqtt_server, mqtt_port);


//uncomment to run on other io platform
  // WiFi.begin(WLAN_SSID, WLAN_PASS);
  // delay(2000);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println();
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
// mqtt end here

//uncomment to run adafruit and ohstem
  //slider.setCallback(slidercallback);
 //onoffbutton.setCallback(onoffcallback);
  //mqtt.subscribe(&slider);
  //mqtt.subscribe(&onoffbutton);


  // Create tasks
  xTaskCreate(TaskBlink, "Task Blink", 2048, NULL, 2, NULL);
  xTaskCreate(TaskTemperatureHumidity, "Task Temperature", 2048, NULL, 2, NULL);
  xTaskCreate(TaskSoilMoistureAndRelay, "Task Soil Relay", 2048, NULL, 2, NULL);
  xTaskCreate(TaskLightAndLED, "Task Light LED", 2048, NULL, 2, NULL);

  //uncomment to run webserver
  xTaskCreate(TaskWebServer, "Task WebServer", 8192, NULL, 1, NULL);
  
  //xTaskCreate( tiny_ml_task, "Tiny ML Task" ,2048  ,NULL  ,2 , NULL);






  Serial.printf("Basic Multi-Threading Arduino Example\n");
}

int pubCount = 0;
void loop() {
  //uncomment to run adafruit and ohstem
  // MQTT_connect();
  // mqtt.processPackets(10000);
  // if(! mqtt.ping()) mqtt.disconnect();

  //uncomment to run coreiot
  // MQTT_connect();
  // mqttClient.loop();
  // unsigned long now = millis();
  //   if(now - lastTelemetry > telemetryInterval){
  //       lastTelemetry = now;
  //       sendTelemetry();
  //   }
}

// Task Definitions
void TaskBlink(void *pvParameters) {
  pinMode(GPIO_NUM_48, OUTPUT);
  uint32_t x=0;
//uncomment to control by button on adafruit and ohstem
//   while (1) {
//     if (ledState) {
//         digitalWrite(LIGHT_PIN, HIGH);  // Bật đèn LED
//     } else {
//         digitalWrite(LIGHT_PIN, LOW);  // Tắt đèn LED
//     }
//     delay(5000);
// }
  //uncomment to blink the LED
  // while(1) {
  //   digitalWrite(GPIO_NUM_48, HIGH);
  //   Serial.println("LED_on");
  //   delay(5000);
  //   digitalWrite(GPIO_NUM_48, LOW);
  //   Serial.println("LED_off");
  //   delay(5000);
  //   //uncomment to run adafruit
  //   if (sensory.publish(x++)) {
  //     Serial.println(F("Published successfully!!"));
  //   }
  // }
}

void TaskTemperatureHumidity(void *pvParameters) {
  while(1) {
    dht20.read();
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();
    Serial.print("Temperature: ");
    Serial.println(dht20.getTemperature());
    glob_temperature = temperature;
    glob_humidity = humidity;

    // if (temperatureFeed.publish(dht20.getTemperature())) {
    //   Serial.println(F("Published successfully!!"));
    // }

    //uncomment to run ohstem 
    // if (temperatureFeed.publish(temperature)) {
    //   Serial.println("Temperature Published Successfully!");
    // } //for ohstem pubblish

        //uncomment to run ohstem 
    // if (humidityFeed.publish(humidity)) {
    //   Serial.println("Humidity Published Successfully!");
    // }//for ohstem pubblish


    Serial.print("Humidity: ");
    Serial.println(dht20.getHumidity());
    // if (humidityFeed.publish(dht20.getHumidity())) {
    //   Serial.println(F("Published successfully!!"));
    // }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20.getTemperature());
    lcd.setCursor(0, 1);
    lcd.print(dht20.getHumidity());

    delay(6000);
  }
}

void TaskSoilMoistureAndRelay(void *pvParameters) {
  pinMode(D3, OUTPUT);
  while(1) {
    soilid = analogRead(A0);
        //uncomment to run ohstem 
    // if (soilMoistureFeed.publish(soilid)) {
    //   Serial.println("Soil Moisture Published Successfully!");
    // }//for ohstem pubblish
    Serial.println("Task Soil and Relay");
    Serial.println(analogRead(A0));

    Serial.println("IP: ");
    Serial.println(WiFi.softAPIP());
    
    if(analogRead(A0) > 500){
      digitalWrite(D3, LOW);
    }
    if(analogRead(A0) < 50){
      digitalWrite(D3, HIGH);
    }
    delay(8000);
  }
}

void TaskLightAndLED(void *pvParameters) {
  while(1) {
    Serial.println("Task Light and LED");
    Serial.println(analogRead(A1));
     float lightLevel = analogRead(A1);   

    //uncomment to run ohstem
    // if (lightFeed.publish(lightLevel)) {
    //   Serial.println("Light Level Published Successfully!");
    // }//for ohstem pubblish
    if(analogRead(A1) < 1000){
      pixels3.setPixelColor(0, pixels3.Color(255,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,255));
      pixels3.setPixelColor(2, pixels3.Color(255,0,255));
      pixels3.setPixelColor(3, pixels3.Color(0,255,0));
      pixels3.show();
    } 
    else {
      pixels3.clear();
      pixels3.show();
    }
    delay(10000);
  }
}

void TaskWebServer(void *pvParameters) {
// Define HTTP endpoints
 if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      //return;
  }
if(SPIFFS.exists("/web.html")){
    Serial.println("web.html  found in SPIFFS!");
}
  // Serve the HTML file
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/web.html", "text/html");
  });

server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float temperature = dht20.getTemperature();
    request->send(200, "text/plain", String(temperature));
  });

server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
  float humidity = dht20.getHumidity();
  request->send(200, "text/plain", String(humidity));
});

server.on("/soilmoisture", HTTP_GET, [](AsyncWebServerRequest *request){
  float soilMoisture = soilid; 
  request->send(200, "text/plain", String(soilMoisture));
});

server.on("/light", HTTP_GET, [](AsyncWebServerRequest *request){
  float lightIntensity = analogRead(A1);
  request->send(200, "text/plain", String(lightIntensity));
});

server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
  digitalWrite(LIGHT_PIN, HIGH);
  request->send(200, "text/plain", "LED is On");
});

server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
  digitalWrite(LIGHT_PIN, LOW);
  request->send(200, "text/plain", "LED is Off");
});

server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/logo.png", "image/png");
});


server.begin();

}





