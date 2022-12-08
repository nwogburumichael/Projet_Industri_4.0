#include <Arduino.h>
#include <WifiClient.h>
#include <PubSubClient.h>
#include <Wifi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_NeoPixel.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "LAPTOP_T"
#define WIFI_PASSWORD "TIMON123"
const char* ssid = "LAPTOP_T";
const char* password = "TIMON123";

// Insert Firebase project API Key
#define API_KEY "AIzaSyBAq3wQ0PUtwS3-O1IRf3Lg5aErtic1xFo"
#define DATABASE_URL "https://tp8firebase-default-rtdb.europe-west1.firebasedatabase.app/" 

// Variable PIN
#define PINLED 2
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

//MQTT server
const char* mqtt_server = "10.22.2.42";
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

/*-------------------------BEGIN MQTT---------------------------*/
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
/*-------------------------END MQTT---------------------------*/

void actualisation(String chemin,String valeur){

  // if(String(chemin) == "/json/led1"){Serial.print("premiere condition verifier");}
  // if(String(valeur.c_str()) == "on"){Serial.println("Deuxieme condition verifier");}
  if(String(chemin) == "/json/led1" && String(valeur) == "on"){
    Serial.printf("LED1 allumé %s",valeur);
    digitalWrite(led1,HIGH);
  }
  else if(String(chemin) == "/json/led1" && String(valeur) == "off"){
     Serial.printf("LED1 eteint%s",valeur);
    digitalWrite(led1,LOW);
  }

  if(String(chemin) == "/json/led2" && String(valeur) == "on"){
    Serial.printf("LED2 allumé %s",valeur);
    digitalWrite(led2,HIGH);
  }
  else if(String(chemin) == "/json/led2" && String(valeur) == "off"){
     Serial.printf("LED2 eteint%s",valeur);
    digitalWrite(led2,LOW);
  }
}

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

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());

  // Due to limited of stack memory, do not perform any task that used large memory here especially starting connect to server.
  // Just set this flag and check it status later.
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

  /*Initialisation des PIN*/
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(bp,INPUT);
  
  /*Connexion Wifi*/
  setup_wifi();
  dht.begin();

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
  // test
  count++;
  FirebaseJson json;
  json.add("led1", "Off");
  json.add("led2", "Off");
  Serial.printf("Set json... %s\n\n", Firebase.RTDB.setJSON(&fbdo, "/Signalisation/stream/data/json", &json) ? "ok" : fbdo.errorReason().c_str());
  // test  
  /*---------------END IREBASE SETUP----------------*/
}


void loop(){

  if (!client.connected()) { // faire une taches pour la reconnexion
    reconnect();
  }
  client.loop();
}
