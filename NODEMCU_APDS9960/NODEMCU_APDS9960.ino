
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SEC_TO_MILISEC(x) ((x)*1000) 



// Pins en wemos D1 mini
#define APDS9960_INT      D6  //AKA GPIO12 -- Interrupt pin
#define APDS9960_SDA    D3  //AKA GPIO0
#define APDS9960_SCL     D1  //AKA GPIO5


// Global Variables
SparkFun_APDS9960 apds = SparkFun_APDS9960();
volatile bool isr_flag = 0;


char * ssid ="WIFI Pier";
char * pass ="pagle736pagle";
unsigned int mqttPort=1883;

const char MqttUser[]="matiBed";
const char MqttPassword[]="1234";
const char MqttClientID[]="mBed";

IPAddress mqttServer(192,168,0,116);

WiFiClient wclient;
PubSubClient mqtt_client(wclient);

void callback(char* topic, byte* payload, unsigned int length);
void  debug_message (char * string, bool newLine)
{
#ifdef DEBUGG
  if(string !=NULL)
  {
    if (!newLine)
    {
      Serial.print(string);
    }else
    {
      Serial.println(string);
    }
  }
  #endif
}

void setUpWifi(char * ssid, char * pass)
{
  String ip;
  debug_message(" ",true);
  debug_message(" ",true);
  debug_message("Conectandose a: ",false);
  debug_message(ssid,true);

  WiFi.begin(ssid,pass);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    debug_message(".",false);
  }
  debug_message(" ",true);
  debug_message("Coneccion realizada",true);
  debug_message("La ip es: ",false);
  ip=WiFi.localIP().toString();
  debug_message((char *)ip.c_str(),true);
}

void setUpMqtt(void)
{
  mqtt_client.setServer(mqttServer,mqttPort);
  mqtt_client.setCallback(callback);
}


void callback(char* topic, byte* payload, unsigned int length)
{
  int tiempo=0;
  payload[length]='\n';
  String message((char *)payload);
  debug_message("Llego un mensage, topic:",false);
  debug_message(topic,false);
  debug_message(", payload : ",false);
  debug_message((char *)payload,true);

  if(!strcmp(topic,"timer/stop"))
  {
   
    debug_message("LLEGO MENSAJE PARA DETENER LA BOMBA",true);
  }
 
}


void setup() {

  setUpWifi(ssid,pass);
  setUpMqtt();

  //Start I2C with pins defined above
  Wire.begin(APDS9960_SDA,APDS9960_SCL);

  // Set interrupt pin as input
  pinMode(APDS9960_INT, INPUT);

  // Initialize Serial port
  Serial.begin(9600);
 
  // Initialize interrupt service routine
  attachInterrupt(APDS9960_INT, interruptRoutine, FALLING);

  // Initialize APDS-9960 (configure I2C and initial values)
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
 
  // Start running the APDS-9960 gesture sensor engine
  if ( apds.enableGestureSensor(true) ) {
    Serial.println(F("Gesture sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during gesture sensor init!"));
  }
}

void reconnect()
{
  while(!mqtt_client.connected())
  {
    debug_message("Intentando conectar al servidor MQTT",true);
    if (mqtt_client.connect(MqttClientID,MqttUser,MqttPassword))
      {
            debug_message("conectado",true);
  
  
            // ...suscrivirse a topicos
            //mqtt_client.subscribe("timer/setTime");
           // mqtt_client.subscribe("timer/start");
           // mqtt_client.subscribe("timer/stop");
           // mqtt_client.subscribe("timbre/on");
           // mqtt_client.subscribe("timbre/off");
            


      }
      else
      {
        debug_message("intentando conetarse al broker",true);
        delay(3000);
      }
  }
}

void loop() {

  if (!mqtt_client.connected()) 
  {
      reconnect();
 }
 mqtt_client.loop(); 
  
  
  if( isr_flag == 1 ) {
   detachInterrupt(APDS9960_INT);
    
   handleGesture();
    isr_flag = 0;
    attachInterrupt(APDS9960_INT, interruptRoutine, FALLING);
  }
 // delay(100);
}

void interruptRoutine() {
  isr_flag = 1;
  Serial.print("SE llama a la irq"); 
   
}

void handleGesture() {
    if ( apds.isGestureAvailable() ) {
    switch ( apds.readGesture() ) {
      case DIR_LEFT:
      mqtt_client.publish("bed/gesture","left");
        Serial.println("LEFT");   //LEFT Pasa página
        break;
      case DIR_RIGHT:
        Serial.println("RIGHT");   //RIGHT retrocede pagina
        mqtt_client.publish("bed/gesture","right");
        break;
      case DIR_NEAR:
        Serial.println("NEAR");
        mqtt_client.publish("bed/gesture","near");
        break;
      case DIR_FAR:
        Serial.println("FAR");
        mqtt_client.publish("bed/gesture","far");
        break;
      case DIR_UP:
        Serial.println("UP");
        mqtt_client.publish("bed/gesture","up");
        break;
      case DIR_DOWN:
        default:
        Serial.println("DOWN");
        mqtt_client.publish("bed/gesture","down");
        break;
        Serial.println("NONE");
    }
  }
}
