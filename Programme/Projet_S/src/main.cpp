#include <Arduino.h>
#include <WifiClient.h>
#include <PubSubClient.h>
#include <Wifi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_NeoPixel.h>

// JSON
//String output;
DynamicJsonDocument doc2(200);

//FREE RTOS VAR 
TaskHandle_t TaskAuto;
TaskHandle_t TaskManu;
TaskHandle_t TaskTransmission;
SemaphoreHandle_t semaphore;

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WiFi_TIMEOUT_MS 20000
#define WIFI_SSID "mike"
#define WIFI_PASSWORD "Nwogburu234"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBrppHxmgeqeBasciYEABjP5C67FLtyRbE"
#define DATABASE_URL "https://projet-serre-e7a00-default-rtdb.europe-west1.firebasedatabase.app/"

// Variable PIN

#define PINLEDWIFI 2
#define PINLEDVOLETSHAUT 12
#define PINLEDVOLETSBAS 32
#define PINLEDLDR 25
#define PINLDR 34 
#define DHTpin 26
#define LedAuto 14
#define LedManu 33
#define LedClient 0
//pin rgb
#define RED 13
#define Blue 27

//pinhumi
#define LEDHUMI 4
#define LEDSEC 15

// Define Firebase Data object
FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//Define DHT object
DHT dht(DHTpin, DHT22);

//point de consignes

int tempconsigne;
int humiconsigne;
int lumiconsigne;
int nbrarrosage;
int modemanu = 0;

//variable actuel

float tempactu = 19;
float humiactu = 55;
long lumiactu = 40;
int volethaut; 
int voletbas ;
int arrosage ;

//variable pour mode auto
int tempManu;
int humiManu;
int lumiManu;
int volethautManu; 
int voletbasManu ;
int arrosageManu = 3 ;
int lumivolet = 80;
int u = 0;


//flag
int positionVolethaut = 0; 
bool taskGlobal;
bool flagarr;
//MQTT server
const char* mqtt_server = "192.168.43.128";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String valeurLed ="";

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
volatile bool dataChanged = false;
int compteur = 0;
/*---------------------BEGIN FREE RTOS TACHES-------------------*/
void actualisation(String topic,String messageTemp){

  Serial.print("JE SUIS DANS laCTU");
  if(String(topic) == "FruitTopic"){
    flagarr = true;
    Serial.print("JE SUIS DANS FRUITOPIC");
    Serial.print(messageTemp);
    //fraise
    if (String(messageTemp) == "1"){
      
      Serial.print("RENTRE DANS CONSINGE");
      tempconsigne = 20 ;
      humiconsigne = 55 ;
      lumiconsigne = 100 ;
      nbrarrosage = 2 ;
      Serial.println(tempconsigne);
      
    }
      // kiwi
    else if (String(messageTemp) == "2")  
      {
        tempconsigne = 27 ;
      humiconsigne = 75 ;
      lumiconsigne = 80 ;
      nbrarrosage = 1 ;
      Serial.println(tempconsigne);
      }
      // cerise
    else if (String(messageTemp) == "3")  
      {
      tempconsigne = 15 ;
      humiconsigne = 30 ;
      lumiconsigne = 35 ;
      nbrarrosage = 1 ;
      Serial.println(tempconsigne);
      }

    }
  
  //reception mode 
  if(String(topic) == "ModeActuel" && String(messageTemp)== "true"){ 
    modemanu = 1;
    Serial.println("mode manu : "); 
    Serial.println(String(modemanu));
  }
  else if (String(topic) == "ModeActuel" && String(messageTemp)== "false"){
    modemanu = 0;
    Serial.println("mode manu : ");
    Serial.println(String(modemanu)); 
  }

  //prise des valeurs en mode manu

  if (String(topic) == "ManuPotTemp") {
    tempManu = String(messageTemp).toInt() ;
    Serial.println(tempManu);
  } 

  if (String(topic) == "ManuPotHumi") { 
    humiManu = String(messageTemp).toInt();
  } 

  if (String(topic) == "ManuPotLumi") {
    lumiManu = String(messageTemp).toInt();
  }

  if (String(topic) == "ManuBpVoletHaut" && String(messageTemp)== "1") {
    volethautManu = 1;
    
  }
  else if (String(topic) == "ManuBpVoletHaut" && String(messageTemp)== "0"){
    volethautManu = 0;
    
  }

  if (String(topic) == "ManuBpVoletBas" && String(messageTemp)== "1"){
    voletbasManu =1 ;
  }
  else if (String(topic) == "ManuBpVoletBas" && String(messageTemp)== "0"){
    voletbasManu =0 ;
  }

  if (String(topic) == "ManuBpArrosage" && String(messageTemp)== "1"){
    arrosageManu = 1 ;
  }

  else if  (String(topic) == "ManuBpArrosage" && String(messageTemp)== "0"){
    arrosageManu = 0;
  }
}

/*----------------TACHE ModeAuto-----------------*/
void ModeAuto(void *argp) {
  for (;;) {
   
    //lumiactu = map(analogRead(PINLDR),0,4095,0,100);
//fonction luminosité
    unsigned int temp1 = uxTaskGetStackHighWaterMark(NULL);
    Serial.print("task wifi="); Serial.println(temp1);
  //condition ouvrir volet
  if (-10 < lumiactu - lumiconsigne < 10  && positionVolethaut != 1 ) {
    Serial.println("VOLETOUVERT");
    digitalWrite(PINLEDVOLETSHAUT,HIGH);
    client.publish("etatvolethaut","1");
    vTaskDelay(12000/ portTICK_PERIOD_MS);
    digitalWrite(PINLEDVOLETSHAUT,LOW);
    client.publish("etatvolethaut","0");
    analogWrite(PINLEDLDR,map(lumivolet,0,100,0,255));
    positionVolethaut = 1;

  }
  else if (-10 < lumiactu - lumiconsigne < 10 && positionVolethaut == 1 ) {
    analogWrite(PINLEDLDR,map(lumivolet,0,100,0,255));
  }

  //condition lampe interieur
  else if (-10 > lumiactu - lumiconsigne > 10 && positionVolethaut != 1) {
    Serial.println("LAMPEINTERIEUR");

    analogWrite(PINLEDLDR,map(lumiconsigne,0,100,0,255));
  }

  else if (-10 > lumiactu - lumiconsigne > 10 && positionVolethaut == 1) {
    Serial.println("LAMPEINTERIEUR");
    digitalWrite(PINLEDVOLETSBAS,HIGH);
    client.publish("etatvoletbas","1");
    vTaskDelay(12000/ portTICK_PERIOD_MS);
    digitalWrite(PINLEDVOLETSBAS,LOW);
    client.publish("etatvoletbas","0");
    analogWrite(PINLEDLDR,map(lumiconsigne,0,100,0,255));
    positionVolethaut = 0;
  }

  //fonction température
  //chauffer&& tempconsigne > 0
  if (tempactu < tempconsigne && tempconsigne > 0 ){
    Serial.println("CHAUFFER");
    int difftempchaud = tempconsigne - tempactu;
    analogWrite(RED,map(difftempchaud,0,30,0,255));
    tempactu++;
    delayMicroseconds(200);

    
  }
  

  //refroidir&& tempconsigne > 0
  else if (tempactu > tempconsigne && tempconsigne > 0 ){
    Serial.println("REFROIDIR");
    int difftempfroid = tempactu - tempconsigne;
    analogWrite(Blue,map(difftempfroid,0,30,0,255));

    tempactu--;
    delayMicroseconds(200);
    
  }

  //fonction humidificateur
  if (humiactu < humiconsigne && humiconsigne > 0){
    Serial.println("HUMIDIFIE");
    int diffhumi = humiconsigne - humiactu;
    analogWrite(LEDHUMI,map(diffhumi,0,100,0,255));
    humiactu++;
    delayMicroseconds(350);
  }
  //secher
  else if (humiactu > humiconsigne && humiconsigne > 0){
    Serial.println("SECHER");
    int diffhumisec = humiactu - humiconsigne;
    analogWrite(LEDSEC,map(diffhumisec,0,100,0,255));
    humiactu--;
    delayMicroseconds(350);
  }
  //arrosage
  int i;
  
  if (flagarr == true){
    for (i = 0 ;i == nbrarrosage;i++){
    analogWrite(LEDHUMI,240);
    client.publish("etatarrosage","1");
    //allumer led arrosage pour 2 sec 
    vTaskDelay(8000/ portTICK_PERIOD_MS);
    client.publish("etatarrosage","0");
    analogWrite(LEDHUMI,0);
    if ( i == nbrarrosage){
      flagarr = false;
    }
  }
  }
  
   
    Serial.print("Auto");
    
    Serial.println("consigne" + tempconsigne);
    delay(1000);
    } 
 }

/*----------------TACHE ModeManu-----------------*/
void ModeManu(void *argp) {
  for (;;) {  
    //gestion lumi
    analogWrite(PINLEDLDR,map(lumiManu,0,100,0,255));
    //gestion temp
    //chauffer
    analogWrite(RED,map(tempManu,15,30,0,245));
    //refroidir
    analogWrite(Blue,map(tempManu,14,0,0,245));

    //gestion humi
    //humidife
    analogWrite(LEDHUMI,map(humiManu,50,100,0,245));
    //secher
    analogWrite(LEDSEC,map(humiManu,0,49,0,245));
    
    if (arrosageManu == 1){
      analogWrite(LEDHUMI,240);
      client.publish("etatarrosage","1");
      arrosageManu = 3;
    }
    else if (arrosageManu ==0) {
      analogWrite(LEDHUMI,0);
      client.publish("etatarrosage","0");
      arrosageManu = 3;
    }

    //
    if (volethautManu == 1 && positionVolethaut !=1){
      analogWrite(PINLEDVOLETSHAUT,HIGH);
      client.publish("etatvolethaut","1");
      vTaskDelay(12000/ portTICK_PERIOD_MS);
      analogWrite(PINLEDVOLETSHAUT,LOW);
      client.publish("etatvolethaut","0");
      analogWrite(PINLEDLDR,map(lumivolet,0,100,0,255));
      positionVolethaut = 1 ;
    }
    else if (volethautManu == 1 && positionVolethaut !=0){
      //ecrire que le volet est deja en position haut
    }

    if (voletbasManu == 1 && positionVolethaut != 1){
      //ecrire que le volet est deja en position bas
    }
  //ferme le volet
    else if (voletbasManu == 1 && positionVolethaut != 0){
      analogWrite(PINLEDVOLETSBAS,HIGH);
      client.publish("etatvoletbas","1");
      vTaskDelay(12000/ portTICK_PERIOD_MS);
      analogWrite(PINLEDVOLETSBAS,LOW);
      client.publish("etatvoletbas",0);
      analogWrite(PINLEDLDR,0);
      positionVolethaut = 0;
    }
    //delay(500);
    //Serial.print("Manu");
  } 
 }
/*----------------TACHE Transmission-----------------*/
void Transmission(void *argp) {
  
  for (;;) { 
  doc2["HUM"] = String(humiactu);
  doc2["TEMP"] = String(tempactu);
  doc2["HAUTEUR"] = "56";
  doc2["LUM"] = String(lumiactu);
  String output;
  serializeJson(doc2, output);
  client.publish("donnees",output.c_str());
  //transmission toute les secondes, une fois connecter
  vTaskDelay(4000/ portTICK_PERIOD_MS);
  }

  } // TEST
/*---------------------TASK CORE 1-------------------*/
// void mqttConnect(void * parameter) {
//   for (;;) {

//     while(!client.connected() && WiFi.status() == WL_CONNECTED){
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
//         client.subscribe("ManuPotHumi");
//         client.subscribe("ManuPotLumi");
//         client.subscribe("ManuPotTemp");
//         client.subscribe("ManuBpVoletHaut");
//         client.subscribe("ManuBpVoletBas");
//         client.subscribe("ManuBpArossage");
//         client.subscribe("ModeActuel");
//         client.subscribe("TempActuel");
//         client.subscribe("HumiActuel");
//         client.subscribe("LumiActuel");
//         client.subscribe("FruitTopic");
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
      digitalWrite(PINLEDWIFI,HIGH);
      Serial.println("wifi still connected :");
      Serial.print(String(WiFi.localIP()));
      vTaskDelay(10000/ portTICK_PERIOD_MS);
      continue;
    }

  Serial.println("wifi connection");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() -startAttemptTime < WiFi_TIMEOUT_MS){}

  if(WiFi.status() != WL_CONNECTED){
    digitalWrite(PINLEDWIFI,LOW);
    Serial.println("[wifi] FAILED");
    vTaskDelay(20000/ portTICK_PERIOD_MS);
    continue;
  }
    }
}

/*---------------------END FREE RTOS TACHES-------------------*/

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

void setup(){
  Serial.begin(115200);
  // Initialisation of JSON Derulo Object


  Serial.println(xPortGetCoreID());

  /*Initialisation des PIN*/
  pinMode(PINLEDVOLETSBAS,OUTPUT);
  pinMode(PINLEDVOLETSHAUT,OUTPUT);
  pinMode(PINLEDLDR,OUTPUT);
  pinMode(RED,OUTPUT);
  pinMode(Blue,OUTPUT);
  pinMode(LEDHUMI,OUTPUT);
  pinMode(LEDSEC,OUTPUT);
  pinMode(PINLDR,INPUT);
  pinMode(PINLEDWIFI,OUTPUT);
  pinMode(LedClient,OUTPUT);
  pinMode(LedAuto,OUTPUT);
  pinMode(LedManu,OUTPUT);



  dht.begin();

  // Wifi connnect Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi In the SETUP");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }

  /*-------------Begin setup RTOS--------------*/

	xTaskCreatePinnedToCore(
		ModeManu, /* Function to implement the task */
		"TaskManu", /* Name of the task */
		50000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		&TaskManu, /* Task handle. */
		0); /* Core where the task should run */

	xTaskCreatePinnedToCore(
		ModeAuto, /* Function to implement the task */
		"TaskAuto", /* Name of the task */
		50000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		&TaskAuto, /* Task handle. */
		0); /* Core where the task should run */
	
//TASK CORE 1

	xTaskCreatePinnedToCore(
        WifiConnect, /* Function to implement the task */
        "Taskwifi", /* Name of the task */
        80000, /* Stack size in words */
        NULL, /* Task input parameter */
        0, /* Priority of the task */
        NULL, /* Task handle. */
        ARDUINO_RUNNING_CORE); /* Core where the task should run */

// transmission
    xTaskCreatePinnedToCore(
		Transmission, /* Function to implement the task */
		"TaskTransmission", /* Name of the task */
		50000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		&TaskTransmission, /* Task handle. */
		0); /* Core where the task should run */

  vTaskSuspend(TaskManu);
  vTaskSuspend(TaskAuto);

	Serial.println("Setup completed.");
  /*------------------End setup RTOS--------------*/

  /*------------BEGIN MQTT SETUP--------------*/
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  
}


void loop(){
  client.loop();
  //tempactu = dht.readTemperature();
  //humiactu = dht.readHumidity();

  if (modemanu == 0 && taskGlobal != true ){
      
      digitalWrite(LedAuto,HIGH);
      digitalWrite(LedManu,LOW);
       vTaskSuspend(TaskManu);
       vTaskResume(TaskAuto);
       taskGlobal = true;
       
    }
    else if (modemanu == 1 && taskGlobal != false){
      digitalWrite(LedAuto,LOW);
      digitalWrite(LedManu,HIGH);
      vTaskSuspend(TaskAuto);
      vTaskResume(TaskManu);
      taskGlobal = false ;
    }

  if(!client.connected()){
      unsigned int temp2 = uxTaskGetStackHighWaterMark(NULL);
      //Serial.print("task mqtt="); Serial.println(temp2);
      Serial.print("Attempting MQTT connection...");
      

      // Attempt to connect
      if (client.connect("Client")) {
        Serial.println("connected");
        digitalWrite(LedClient,HIGH);
        // Once connected, publish an announcement...
        client.publish("outTopic", "hello world"); // ATTENTION 
        // ... and resubscribe
        
        client.subscribe("ManuPotHumi");
        client.subscribe("ManuPotLumi");
        client.subscribe("ManuPotTemp");
        client.subscribe("ManuBpVoletHaut");
        client.subscribe("ManuBpVoletBas");
        client.subscribe("ManuBpArossage");
        client.subscribe("ModeActuel");
        client.subscribe("TempActuel");
        client.subscribe("HumiActuel");
        client.subscribe("LumiActuel");
        client.subscribe("FruitTopic");
        vTaskResume(TaskTransmission);

      } 
      else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        vTaskSuspend(TaskTransmission);
        digitalWrite(LedClient,LOW);
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        vTaskDelay(20000/ portTICK_PERIOD_MS);
      }
  }
  /*doc2["HUM"] = String(humiactu);
  doc2["TEMP"] = String(tempactu);
  doc2["HAUTEUR"] = "56";
  doc2["LUM"] = String(lumiactu);
  String output;
  serializeJson(doc2, output);
  client.publish("donnees",output.c_str()); // TEST*/
  
 

}
