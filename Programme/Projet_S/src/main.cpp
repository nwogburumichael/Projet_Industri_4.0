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
String output;
DynamicJsonDocument doc2(96);

//FREE RTOS VAR 
TaskHandle_t TaskAuto;
TaskHandle_t TaskManu;
SemaphoreHandle_t semaphore;

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WiFi_TIMEOUT_MS 20000
const char* ssid = "mike";
const char* password = "Nwogburu234";

// Insert Firebase project API Key
#define API_KEY "AIzaSyBrppHxmgeqeBasciYEABjP5C67FLtyRbE"
#define DATABASE_URL "https://projet-serre-e7a00-default-rtdb.europe-west1.firebasedatabase.app/"

// Variable PIN
#define PINLED 2
#define PINLEDVOLETSHAUT 13
#define PINLEDVOLETSBAS 12
#define PINLEDLDR 27
#define PINLDR 14 
#define DHTpin 26
#define led1 14
#define led2 15
#define bp 12

// Define Firebase Data object
FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//Define DHT object
DHT dht(DHTpin, DHT11);

//point de consignes

int tempconsigne;
int humiconsigne;
int lumiconsigne;
int nbrarrosage;
int modemanu;

//variable actuel

int tempactu;
int humiactu;
int lumiactu;
int volethaut; 
int voletbas ;
int arrosage ;

//variable pour mode auto
int tempManu;
int humiManu;
int lumiManu;
int volethautManu; 
int voletbasManu ;
int arrosageManu ;

//flag
int positionVolethaut = 0; 

//MQTT server
const char* mqtt_server = "10.22.1.252";
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

/*----------------TACHE Manager-----------------*/
void TaskManager(void *argp) {
  
  for (;;) {

    // delay(2000);
    // vTaskSuspend(TaskAuto);
    // vTaskResume(TaskManu);
    Serial.print("MANAGER");
    delay(1000);
    // vTaskSuspend(TaskManu);
    // vTaskResume(TaskAuto);
    }
 }

/*----------------TACHE ModeAuto-----------------*/
void ModeAuto(void *argp) {
  for (;;) {
//fonction luminosité
  if (lumiconsigne >= 70 && positionVolethaut != 1 ) {
    digitalWrite(PINLEDVOLETSHAUT,HIGH);
    delayMicroseconds(3000);
    digitalWrite(PINLEDVOLETSHAUT,LOW);
    analogWrite(PINLEDLDR,255);
    positionVolethaut = 1;

  }
  else if (lumiconsigne >= 70 && positionVolethaut == 1 ) {
    analogWrite(PINLEDLDR,255);
  }


  else if (lumiconsigne < 70 && positionVolethaut != 1) {

    analogWrite(PINLEDLDR,map(lumiconsigne,0,100,0,255));
  }

  else if (lumiconsigne < 70 && positionVolethaut == 1) {
    digitalWrite(PINLEDVOLETSBAS,HIGH);
    delayMicroseconds(3000);
    digitalWrite(PINLEDVOLETSBAS,LOW);
    analogWrite(PINLEDLDR,map(lumiconsigne,0,100,0,255));
    positionVolethaut = 0;
  }

  //fonction température
  

    Serial.print("Auto");
    delay(500);
    } 
 }

/*----------------TACHE ModeManu-----------------*/
void ModeManu(void *argp) {
  for (;;) {  
    Serial.print("Manu");
    delay(500);
  } 
 }

/*---------------------TASK CORE 1-------------------*/
void reconnect() {
 while (!client.connected()) {
  Serial.print("Attempting MQTT connection...");

  // Attempt to connect
  if (client.connect("Client")) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    client.publish("outTopic", "hello world");
    // ... and resubscribe
    client.subscribe("inTopic");
  } 
  else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
 }
}

void WifiConnect(void * parameter) {
	for (;;) {

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("wifi still connexted");
      vTaskDelay(10000/ portTICK_PERIOD_MS);
      continue;
    }

  Serial.println("wifi connection");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && 
        millis() -startAttemptTime < WiFi_TIMEOUT_MS){}
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("[wifi] FAILED");
    vTaskDelay(20000/ portTICK_PERIOD_MS);
    continue;
  }
  Serial.println("[WIFI] Connected: " + WiFi.localIP());
  //METTRE UN SEMAPHORE
  reconnect();
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
  Serial.println();
  Serial.print(topic);

  // Switch on the LED if an 1 was received as first character
  if (String(topic) == "inTopic") {
    Serial.print("ok");
    if(messageTemp == "true"){
      digitalWrite(PINLED, HIGH);
    }
      // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    else if(messageTemp == "false"){
      digitalWrite(PINLED, LOW);  // Turn the LED off by making the voltage HIGH
  }
}
}


/*-------------------------END MQTT---------------------------*/

void actualisation(String chemin,String valeur){


  if(String(chemin) == "/main/fruit"){
    switch (tolower(valeur[0])){
      case 'f' :
      tempconsigne = 20 ;
      humiconsigne = 50 ;
      lumiconsigne = 100 ;
      nbrarrosage = 2 ;
      // point de consignes
      case 'k' :
      tempconsigne = 27 ;
      humiconsigne = 75 ;
      lumiconsigne = 80 ;
      nbrarrosage = 1 ;
      // points de consignes
      case 'c' :
      tempconsigne = 15 ;
      humiconsigne = 30 ;
      lumiconsigne = 35 ;
      nbrarrosage = 1 ;

    }
  }
  //reception mode 
  if(String(chemin) == "/main/parametre/modemanu" && String(valeur)== "true"){ 
    modemanu = 1; 
  }
  else if (String(chemin) == "/main/parametre/modemanu" && String(valeur)== "false"){
    modemanu = 0;
  }

  //prise des valeurs en mode manu

  if (String(chemin) == "/main/parametre/manu/temperature") {
    tempManu = String(valeur).toInt() ;
  } 

  if (String(chemin) == "/main/parametre/manu/humidite") { 
    humiManu = String(valeur).toInt();
  } 

  if (String(chemin) == "/main/parametre/manu/luminosite") {
    lumiManu = String(valeur).toInt();
  }

  if (String(chemin) == "/main/parametre/manu/volets/haut" && String(valeur)== "true") {
    volethautManu = 1;
    
  }
  else if (String(chemin) == "/main/parametre/manu/volets/haut" && String(valeur)== "false"){
    volethautManu = 0;
    
  }

  if (String(chemin) == "/main/parametre/manu/volets/bas" && String(valeur)== "true"){
    voletbasManu =1 ;
  }
  else if (String(chemin) == "/main/parametre/manu/volets/bas" && String(valeur)== "false"){
    voletbasManu =0 ;
  }

  if (String(chemin) == "/main/parametre/manu/arrosage" && String(valeur)== "true"){
    arrosageManu = 1 ;
  }

  else if  (String(chemin) == "/main/parametre/manu/arrosage" && String(valeur)== "false"){
    arrosageManu = 0;
  }

  //commande manu
 

  


/*----------------------------------- BEGIN FIREBASE--------------------------------*/
// La fonction est appelé a chaque variation de la base de données dans FIREBASE
void streamCallback(FirebaseStream data){
  Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data); // see addons/RTDBHelper.h
  Serial.println();
  actualisation(data.dataPath(),data.to<String>());

  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
  dataChanged = true;

}

void streamTimeoutCallback(bool timeout)
  {
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
  }

/*----------------------------------- END FIREBASE--------------------------------*/

void setup(){
  Serial.begin(115200);
  // Initialisation of JSON Derulo Object
  doc2["HUM"] = "90";
  doc2["TEMP"] = "22";
  doc2["HAUTEUR"] = "56";
  doc2["LUM"] = "23";

  Serial.println(xPortGetCoreID());

  /*Initialisation des PIN*/
  pinMode(PINLEDVOLETSBAS,OUTPUT);
  pinMode(PINLEDVOLETSHAUT,OUTPUT);
  pinMode(PINLEDLDR,OUTPUT);
  pinMode(PINLDR,INPUT);

  dht.begin();

  /*-------------Begin setup RTOS--------------*/

	xTaskCreatePinnedToCore(
		ModeManu, /* Function to implement the task */
		"TaskManu", /* Name of the task */
		5000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		&TaskManu, /* Task handle. */
		0); /* Core where the task should run */

	xTaskCreatePinnedToCore(
		ModeAuto, /* Function to implement the task */
		"TaskAuto", /* Name of the task */
		5000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		&TaskAuto, /* Task handle. */
		0); /* Core where the task should run */
	
//TASK CORE 1
  xTaskCreatePinnedToCore(
    TaskManager, // Function
    "taskBp", // Task name
    5000, // Stack size (void*)LED1_GPIO, // arg
    NULL, // arg
    0, // Priority
    NULL, // No handle returned
    1); // CPU

	xTaskCreatePinnedToCore(
		WifiConnect, /* Function to implement the task */
		"Taskwifi", /* Name of the task */
		5000, /* Stack size in words */
		NULL, /* Task input parameter */
		0, /* Priority of the task */
		NULL, /* Task handle. */
		1); /* Core where the task should run */

  vTaskSuspend(TaskManu);
  vTaskSuspend(TaskAuto);

	Serial.println("Setup completed.");
  /*------------------End setup RTOS--------------*/

  /*------------BEGIN MQTT SETUP--------------*/
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  /*------------BEGIN FIREBASE SETUP--------------*/
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback; 
  
  if (!Firebase.RTDB.beginStream(&stream, "/Signalisation/stream/data"))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  // test debut
  // count++;
  // FirebaseJson json;
  // json.add("led1", "Off");
  // json.add("led2", "Off");
  // Serial.printf("Set json... %s\n\n", Firebase.RTDB.setJSON(&fbdo, "/Signalisation/stream/data/json", &json) ? "ok" : fbdo.errorReason().c_str());
  // test fin 

  /*---------------END FIREBASE SETUP----------------*/
}


void loop(){
  client.loop();
  delay(500);
  // serializeJson(doc2, output);
  // client.publish("donnees",output.c_str()); // TEST
 

}
