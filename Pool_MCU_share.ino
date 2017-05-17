
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID       "put your wireless SSID here"
#define WLAN_PASS       "put your wireless password here"
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "put your adafruit io username here"
#define AIO_KEY         "put your adafruit AIO key here"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup feeds called 'Current_Temperature' and 'Pool_Mode' for publishing (you will need to create these feed on adafruit io).
Adafruit_MQTT_Publish Current_Temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Current_Temperature");
Adafruit_MQTT_Publish Pool_Mode = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Pool_Mode");

// Setup feeds called 'onoff' and 'lights' for subscribing to changes (you will need to create these feeds on adafruit io).
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");
Adafruit_MQTT_Subscribe lightsbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lights");

// Variables
const int spaLightPin = 13;  //marked as D7 on the board
const int poolLightPin = 12; //marked as D6 on the board
const int tempPin = A0;//marked as A0 on the board
const int modePin = 15;  //marked as D8 on the board
const int modeSensePin = 14; //marked as D5 on the board
unsigned long currentState = 0;
String currentStatus = "POOL";
String newStatus = "POOL";
String lightStatus = "0";



//Functions
void MQTT_connect();

int getTemperature() {
  uint8_t i;
  float average;
  int samples[50];
  float numerator = 1;
  float denominator = 1;
 
// take 50 samples in a row, with a slight delay
  for (i=0; i< 50; i++) {
   samples[i] = analogRead(tempPin);
   delay(10);
  }
 
// average all the samples out
  average = 0;
  for (i=0; i< 50; i++) {
     average += samples[i];
  }
  average /= 50;
 
 
// convert the value to resistance
  numerator = -33000 * average;
  denominator = (3.3 * average) -4900;  // might need to change -5155 to 5*max ADC value
  average = numerator / denominator;


// convert to temperature in F
  float steinhart;
  steinhart = average / 10000;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= 3915;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (25 + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  return steinhart * 9 / 5 + 32;           // convert to F

}

void getPoolMode()
{
  if (pulseIn(modeSensePin, HIGH, 3000000) > 100)
  {
    currentStatus = "SPA";
  }
  else
  {
    currentStatus = "POOL";
  }
}


//Run once setup
void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  mqtt.subscribe(&onoffbutton);
  mqtt.subscribe(&lightsbutton);
  
// GPIO Pin Setup
  pinMode(tempPin, INPUT);
  pinMode(modeSensePin, INPUT);
  pinMode(modePin, OUTPUT);
  pinMode(spaLightPin, OUTPUT);
  pinMode(poolLightPin, OUTPUT);

  
}

void loop() {

  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(9000))) 
  {
    if (subscription == &onoffbutton) 
    {
      newStatus=(char *)onoffbutton.lastread;
      if (currentStatus == "SPA" && newStatus == "POOL")
      {
      digitalWrite(modePin, HIGH);
      delay(800);
      digitalWrite(modePin, LOW);
      delay(2000);
      currentStatus = "POOL";
      }
      if (currentStatus == "POOL" && newStatus == "SPA")
      {
      digitalWrite(modePin, HIGH);
      delay(800);
      digitalWrite(modePin, LOW);
      delay(2000);
      currentStatus = "SPA";
      }
    }
    if (subscription == &lightsbutton) 
    {
      lightStatus=(char *)lightsbutton.lastread;
      if(lightStatus == "POOL")
        {
         digitalWrite(poolLightPin, HIGH);
         delay(800);
         digitalWrite(poolLightPin, LOW);
        }

        if(lightStatus == "SPA")
        {
        digitalWrite(spaLightPin, HIGH);
        delay(800);
        digitalWrite(spaLightPin, LOW);
        }
    }
  }


  
  
  // Calculate the Temperature of the pool

  Serial.print("Temperature "); 
  Serial.print(getTemperature());
  Serial.println(" *F");
  
  // Publish Current Temperature
  Current_Temperature.publish(getTemperature())) 

  // Publish Current Pool Mode
  getPoolMode();
  if(currentStatus == "SPA")
  {
    Pool_Mode.publish("SPA");
  }
  if(currentStatus == "POOL")
  {
    Pool_Mode.publish("POOL");
  }
  
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
