// OTA Library
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
// WIFI Libraries
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
// http request libraries
//#include <ESP8266HTTPClient.h>
// mqtt library
#include <PubSubClient.h>

/* Set these to your desired credentials. */
const char *ssid = "YourSSID"; //Enter your WIFI ssid
const char *password = "YourPassword"; //Enter your WIFI password
const char* mqttServer = "Ip.Of.Your.Mqtt.Server";
const int mqttPort = Port_Number;
const char* mqttUser = "MQTT_USERNAME";
const char* mqttPassword = "MQTT_PASSWORD";
const float saturationvalue = 460; // raw value in pure water for calibration
const float dryvalue = 843; // raw value dry for calibration

WiFiClient espClient;  // declare wifi client
PubSubClient client(espClient); // seclare mqtt client

char pathcreation [100]; // create a char to store alot of data for sprintf string creation

float value; // make a float to store raw analog read value
float valueperpercent = ((dryvalue - saturationvalue) / 100); // float that helps change Raw values into a percentage of soil saturation
float soilMoisture; // final percentage of saturation

unsigned long timenow = millis(); // time for periodic updates
unsigned long timelast = timenow; // last send time for periodic updates

void setup() {
  // put your setup code here, to run once:  
  pinMode(A0, INPUT); // analog pin as an input for soil mositure sensor data cable
  pinMode(D7, OUTPUT); // D7 as output to power sensor so you can turn it off to avoid leaving it on they shouldnt be left on it wears out otherwise
  pinMode(D6, OUTPUT); // D6 as output to connect sensors ground cable
  WiFi.begin(ssid, password); // connect to wifi
   while (WiFi.status() != WL_CONNECTED) { // do nothing until wifi is connected
   delay(500);
  }
  client.setServer(mqttServer, mqttPort); // setup mqtt server
  client.setCallback(callback); // setup the callback function mqtt not used yet
  while (!client.connected()) { // not connected yet then connect as "potsoil"
    Serial.println("Connecting to MQTT...");
     if (client.connect("potsoil", mqttUser, mqttPassword )) { // change potsoil to whatever you want and something different for each client if you have multiple sensors
       Serial.println("connected");  
     } else {
       Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000); // delay Im sure not needed I think i did it cause i was printing to a small screen when testing and i wanted to see the debug messages one at a time
     }
  }
  // sethostname for OTA
  ArduinoOTA.setHostname("potsoil"); // setup as an ota device to be able to send OTA firmaware upgrades
  // No authentication by default but set password for OTA
  ArduinoOTA.setPassword("yourOTApassword"); // OTA Pawword optional comment out for nbo password
  ArduinoOTA.begin(); // start OTA Server
  readsensor(); // Take a sensor reading on first boot
  senddata(); // send sensor mqtt message on first boot
}

void callback(char* topic, byte* payload, unsigned int length) {
 // Not yet used this is for handling recieved mqtt messages
}

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle(); // check for OTA request
  client.loop(); // Check for MQTT incoming messages and connect if disconnected etc
  timenow = millis(); // check the time
  if(((timenow - timelast) >= 300000) || ((timenow - timelast) < 0)){ // if time has either gone back to zero or been 5 minutes take a reading
  readsensor();
  senddata();
  timelast = timenow; // set the last send time to now so the timer can run its magic
 }
  
}

void readsensor(){ // main sensor read function
  digitalWrite(D7, HIGH); // power the sensor
  digitalWrite(D6, LOW); // ground the sensor
  delay(1000); // wait for it to stabilize
  value = analogRead(A0); // read the raw analog value from A0
  digitalWrite(D7, LOW); // Turn off the sensor to avoid wearing it out and drift etc
  if (value <= saturationvalue){ // if readings wetter than the wet value just display it as 100 percen saturated to avoid weird percentages
    soilMoisture = 100;
  }
  else if (value >= dryvalue){ // if reading is drier than the dry level display as 0 to avoid weird percentages
    soilMoisture = 0;
  }
  else{
    soilMoisture = ((dryvalue - value) / valueperpercent); // Calculate soil moisture as percentage if its in range
  }  
}

void senddata(){ // main mqtt data send to home assistant
  if (!client.connected()){ // if for some reason we are not connected to mqtt connect rememebr to change your client name if needed
    client.connect("potsoil", mqttUser, mqttPassword );    
  }
   
  sprintf(pathcreation, "{\"moisture\": %.0f,\"raw\": %f}", soilMoisture, value); // create the string to send as json with the soilMOisture percentage and the raw analog value of the pin which is useful for periodic calibration
  client.publish("potsoil", pathcreation, true); // publish the mqtt message with the topic of potsoil change if you need to have multiple snesnors in home assistant the payload of the message is the string we created above and the true false booleon sets it as a retained message or not
}
