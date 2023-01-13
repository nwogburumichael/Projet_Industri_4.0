#include <TFT_eSPI.h>
#include <SPI.h>
#include <LittleFS.h>

#include <Arduino.h>
#include <WifiClient.h>
#include <PubSubClient.h>
#include <Wifi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>

// #include "Final_Frontier_28.h"
#define AA_FONT_SMALL "NotoSans-Bold.ttf"
#define AA_FONT_LARGE "NotoSans-Regular.ttf"
TFT_eSPI tft = TFT_eSPI();  // Invoke library

// JSON
String output;
DynamicJsonDocument doc2(500);

// Insert your network credentials
#define WiFi_TIMEOUT_MS 10000
#define WIFI_SSID "mike"
#define WIFI_PASSWORD "Nwogburu234"

//Define DHT object
#define DHTpin 26
DHT dht(DHTpin, DHT11);

//MQTT server
const char* mqtt_server = "192.168.43.128";

WiFiClient Michael;
PubSubClient client(Michael);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

//pin bouton
#define bpMode 21
#define bpArossage 22
#define bpVoletH 25
#define bpVoletL 27
#define bpLum 26 

#define potArossage 34
#define potTemp 35
#define potLumi 36

//pin Led
#define statutWifi 15
#define ledAuto 16
#define ledManu 17
#define ledVoletL 19
#define ledVoletH 32
#define ledLum 12
#define ledArossage 13
#define ledTemp 14

bool togMode = false;
bool togArossage = false;
bool togVoletH = false;
bool togVoletL = false;
bool togLum = false;
bool changement = false;
String modeActuel = "None";
String etatmode = "false";
String fruitChoisi = "None";
String  tempActuel = "0";
String humiActuel = "0";
String lumiActuel = "0";

/*---------------------BEGIN FREE RTOS TACHES-------------------*/
/*---------------------TASK CORE 1-------------------*/
// void mqttConnect(void * parameter) {
//   for (;;) {

//     while(!client.connected()){
//       unsigned int temp2 = uxTaskGetStackHighWaterMark(NULL);
//       Serial.print("task mqtt="); Serial.println(temp2);
//       Serial.print("Attempting MQTT connection...");

//       // Attempt to connect
//       if (client.connect("Client")) {
//         Serial.println("connected");
//         // Once connected, publish an announcement...
//         client.publish("outTopic", "hello world"); // ATTENTION 
//         // ... and resubscribe
//         client.subscribe("inTopic");
//       } 
//       else {
//         Serial.print("failed, rc=");
//         Serial.print(client.state());
//         Serial.println(" try again in 5 seconds");
//         // Wait 5 seconds before retrying
//         vTaskDelay(20000/ portTICK_PERIOD_MS);
//       }
//   }
// }
// }

void WifiConnect(void * parameter) {
	for (;;) {
    unsigned int temp1 = uxTaskGetStackHighWaterMark(NULL);
    Serial.print("task wifi="); Serial.println(temp1);

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("wifi still connected :");
      digitalWrite(statutWifi,HIGH);
      vTaskDelay(10000/ portTICK_PERIOD_MS);
      continue;
    }

  Serial.println("wifi connection");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() -startAttemptTime < WiFi_TIMEOUT_MS){}

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("[wifi] FAILED");
    digitalWrite(statutWifi,LOW);
    vTaskDelay(20000/ portTICK_PERIOD_MS);
    continue;
  }
	}
}
/*---------------------END FREE RTOS TACHES-------------------*/
void TFToled(){
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE,179197230,true);

  tft.setCursor(0, 100);
  tft.print("Mode : ");
  tft.print(String(modeActuel));

  tft.setCursor(0, 20);
  tft.setTextSize(1);
  tft.print("T Serre : ");
  tft.print(String(tempActuel));
  tft.print(" *C");

  tft.setCursor(0, 30);
  tft.print("H Serre : ");
  tft.print(String(humiActuel));
  tft.print(" %");

  tft.setCursor(0, 40);
  tft.print("Lum Serre : ");
  tft.print(String(lumiActuel));
  tft.print(" %");

  tft.setCursor(0, 50);
  tft.print("Fruit Choisi : ");
  tft.print(String(fruitChoisi));
}

void actualisation(String topic,String messageTemp){
   
  if(String(topic) == "FruitTopic" ){
    if (messageTemp == "1"){fruitChoisi = "fraise";}
    else if (messageTemp == "2"){fruitChoisi = "kiwi";}
    else if (messageTemp == "3"){fruitChoisi = "fraise";}
  }

  //reception mode 
  if(String(topic) == "ModeActuel"){ 
    if(messageTemp == "true"){
      modeActuel = "Manu";
      digitalWrite(ledAuto,LOW);
      digitalWrite(ledManu,HIGH);}
    else if(messageTemp == "false"){
      modeActuel = "Auto";
      digitalWrite(ledAuto,HIGH);
      digitalWrite(ledManu,LOW);}
  }

  //prise des valeurs en mode manu
  if (String(topic) == "TempActuel") { 
    tempActuel = messageTemp;
  } 

  if (String(topic) == "HumiActuel") { 
    humiActuel = messageTemp;
  } 

  if (String(topic) == "LumiActuel") {
    lumiActuel = messageTemp;
  }

  if (String(topic) == "etatarrosage"){
    if(messageTemp == "1") {analogWrite(ledArossage,255);}
    else if(messageTemp == "0"){analogWrite(ledArossage,0);}
  }

  if (String(topic) == "etatvolethaut" ) {
    if(messageTemp == "1"){digitalWrite(ledVoletH,HIGH);}
    else if(messageTemp == "0"){digitalWrite(ledVoletH,LOW);}
  }
    
  if (String(topic) == "etatvoletbas"){
    if(messageTemp == "1"){digitalWrite(ledVoletL,HIGH);}
    else if(messageTemp == "0"){digitalWrite(ledVoletL,LOW);}
  }
  TFToled();
}

/*-------------------------BEGIN MQTT---------------------------*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  
 actualisation(String(topic),String(messageTemp));
}

/*-------------------------END MQTT---------------------------*/
void transmission(){
  serializeJson(doc2, output);
  Serial.println(output);
  client.publish("donneesSignalisation",output.c_str());
}

void Commande(){
if(digitalRead(bpMode) == LOW){
  togMode = !togMode;
  delay(250);}

if (togMode == true && etatmode == "false"){ 
  etatmode = "true";
  doc2["Mode"] = "true";
  output = "";
  transmission();}

else if (togMode == false && etatmode == "true"){ 
  etatmode = "false";
  doc2["Mode"] = "false";
  output = "";
  transmission();}

if (modeActuel == "Manu"){
  output = "";

  if(digitalRead(bpArossage) == LOW){
    togArossage = !togArossage;
    doc2["BpHumi"] = String(togArossage);
    doc2["PotHumi"] = analogRead(potArossage);
    analogWrite(ledArossage,map(analogRead(potArossage),0,4095,0,255));
    transmission();
    }
  
  if(digitalRead(bpLum) == LOW){
    togLum = !togLum;
    doc2["PotLum"] = map(analogRead(potLumi),0,4095,0,100);
    doc2["PotTemp"] = map(analogRead(potTemp),0,4095,0,30);
    analogWrite(ledLum,map(analogRead(potLumi),0,4095,0,255));
    analogWrite(ledTemp,map(analogRead(potTemp),0,4095,0,255));
    Serial.print("je suis dans la cond");
    transmission();}

  if(digitalRead(bpVoletH) == LOW){
    togVoletH = !togVoletH;
    doc2["BpVoletHaut"] = String(togVoletH);
    transmission();}

  if(digitalRead(bpVoletL) == LOW){
    togVoletL = !togVoletL;
    doc2["BpVoletBas"] = String(togVoletL);
    transmission();}
  delay(250);
}
}

void setup(void) {
  Serial.begin(115200); 
  /*-----Setup LITLLE FS-----*/
    if(!LITTLEFS.begin(true)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }
  /*----Setup Pins----*/
  pinMode(bpMode,INPUT_PULLUP);
  pinMode(bpArossage,INPUT_PULLUP);
  pinMode(bpVoletH,INPUT_PULLUP);
  pinMode(bpVoletL,INPUT_PULLUP);
  pinMode(bpLum,INPUT_PULLUP);

  pinMode(potArossage,INPUT);
  pinMode(potLumi,INPUT);
  pinMode(potTemp,INPUT);

  pinMode(ledArossage,OUTPUT);
  pinMode(ledAuto,OUTPUT);
  pinMode(ledManu,OUTPUT);
  pinMode(ledVoletH,OUTPUT);
  pinMode(ledVoletL,OUTPUT);
  pinMode(ledLum,OUTPUT);
  pinMode(ledTemp,OUTPUT);
  pinMode(statutWifi,OUTPUT);
 
  /*-----SETUP OLED-----*/
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(179197230);

  // Wifi connnect Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi In the SETUP");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }

  //TASK CORE 1
	xTaskCreatePinnedToCore(
		WifiConnect, /* Function to implement the task */
		"Taskwifi", /* Name of the task */
		40000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		NULL, /* Task handle. */
		ARDUINO_RUNNING_CORE); /* Core where the task should run */

	// xTaskCreatePinnedToCore(
	// 	mqttConnect, /* Function to implement the task */
	// 	"TaskMqtt", /* Name of the task */
	// 	80000, /* Stack size in words */
	// 	NULL, /* Task input parameter */
	// 	0, /* Priority of the task */
	// 	NULL, /* Task handle. */
	// 	ARDUINO_RUNNING_CORE); /* Core where the task should run */
  

    /*------------BEGIN MQTT SETUP--------------*/
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Wrap test at right and bottom of screen
  tft.setTextWrap(true, true);
  // Font and background colour, background colour is used for anti-alias blending
  tft.setTextColor(TFT_BLUE, 179197230,true);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.print("Serre Timon Michael 2022-2023");
  // Load the font
  // tft.loadFont(AA_FONT_LARGE);
  // // Display all characters of the font
  // tft.showFont(2000);
  // Set "cursor" at top left corner of display (0,0)
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  // Unload the font to recover used RAM
  // tft.unloadFont();
  TFToled();
  delay(5000);

}

void loop() { 
  Commande();
  client.loop();
  if(!client.connected()){
      // unsigned int temp2 = uxTaskGetStackHighWaterMark(NULL);
      // Serial.print("task mqtt="); Serial.println(temp2);
      Serial.print("Attempting MQTT connection...");

      // Attempt to connect
      if (client.connect("Client30000")) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        client.publish("outTopic", "hello world"); // ATTENTION 
        // ... and resubscribe
        client.subscribe("ManuPotHumi");
        client.subscribe("ManuPotLumi");
        client.subscribe("ManuPotTemp");
        client.subscribe("etatvolethaut");
        client.subscribe("etatvoletbas");
        client.subscribe("etatarrosage");
        client.subscribe("ModeActuel");
        client.subscribe("TempActuel");
        client.subscribe("HumiActuel");
        client.subscribe("LumiActuel");
        client.subscribe("FruitTopic");
      } 
      else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        // vTaskDelay(20000/ portTICK_PERIOD_MS);
      }
      }
}