#include <Wire.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define   Slave_addr    10
#define   ESP_Sleep     32
#define   Arduino_Sel   33
#define   Arduino_Wake  GPIO_NUM_15

#define   EEPROM        80          // 1010 000 - bytes needed at the start as per the EEPROM data sheet

// Wi-Fi
const char* ssid = "GetYourOwnWi-Fi";
const char* password = "Srsly.Get.It.";


//API URL
const char* IapiUrl = "http://www.boredapi.com/api/activity?minprice=0&maxprice=";
char UapiUrl[80] = "NULL";

//The Message Size that needs to be sent through I2C
byte            MesSize     = 0;

//New Price variable
float           n_Price     = 0;

//The data that must be sent through I2C
String          w_data      = "0";



// EEPROM stuff
const unsigned int      cellAddress     =   10;

//Time shenanigans
unsigned long   time1       = 0;
unsigned long   time2       = 0;


void WiFiConnectSetup() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(2000);
    Serial.print(".");
  }

  Serial.println("Done!");
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void UpdatePrice(float newPrice) {
  char priceString[10];
  dtostrf(newPrice, 5, 4, priceString);

  Serial.println(priceString);

  // Merging strings
  strcpy(UapiUrl, IapiUrl);
  strcat(UapiUrl, priceString);

  Serial.println(UapiUrl);
}

String APInterrogation() {
  
  HTTPClient http;
  http.begin(UapiUrl);

  String payload = "NULL";

  int httpResponseCode = http.GET();
  //Serial.println(httpResponseCode);
  if(httpResponseCode == HTTP_CODE_OK) 
  {
    payload = http.getString();
    //Serial.println("Response:");
    //Serial.println(payload);

    http.end();
    return payload;
  } 
  else 
  {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpResponseCode);

    http.end();
    return payload;
  }
  
  http.end();
  return payload;
}

String JSONParseActivity(String payload) {

  String activity = "NULL";

  StaticJsonDocument<256> doc; // Adjust the capacity according to your JSON response size
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
  } else {
    // Extract the value of the "activity" field
    activity = String(doc["activity"].as<String>());
    /*
    if (!activity.isEmpty()) {
      Serial.print("Activity: ");
      Serial.println(activity);
    } else {
      Serial.println("Activity not found in JSON response");
    }
    */
  }
  return activity;
}

void InternetRetrievePhase() {

  byte      Retry       = 0;
  byte      Fail        = 0;
  String    Message     = "0";
  String    Activity    = "0";

  do {

  Retry = 0;

  Message = APInterrogation();
  Activity = JSONParseActivity(Message);

  if(Activity.equals("NULL")) {
    Fail++;
    Retry = 1;
  }

  if(Activity.length() > 32)
  {
    Retry = 1;
  }

  Serial.println(Activity);
  //Serial.print("Retry is: ");
  //Serial.println(Retry);

  delay(500);
  }
  while( Fail < 5 && Retry == 1 );

  //if(Activity.equals("NULL")) {
  //  Activity = "Go outside";
  //}
  
  MesSize = Activity.length();
  //Serial.println(MesSize);
  while (MesSize < 32) {
    Activity += ' '; // Add a space character
    MesSize++;
    //Serial.println(Activity);
  }

  w_data = Activity;
  Serial.println(w_data);
  Serial.println(MesSize);
}

void recieveEvent(int numBytes) {
  Serial.println("Reading!");
  String response = "";
  while ( numBytes > 0 && Wire.available())
  {
    char x = Wire.read();
    response += x;
  }
  if(response[0] == '0' || response[0] == '1')
  {
    n_Price = std::stof(response.c_str());
  }
  else
  {
    n_Price = 0;
  }
  UpdatePrice(n_Price);
  digitalWrite(ESP_Sleep, LOW);
}

void requestEvent() {

  byte response[32];

  Serial.print("The sent data should be: ");
  Serial.println(w_data);

  for(byte i = 0; i < w_data.length(); i++)
  {
    response[i] = (byte)w_data.charAt(i);
  }

  Wire.write(response, sizeof(response));
}

void Updating() {
  //time1 = millis();
  while( digitalRead(15) == 0 )
  {
    //time2 = millis();

    millis();

    // Time constraint for it to not go into an infinite loop ( 12s )
    //if(time2 - time1 >= 12000) {
    //  goto sleep;
    //}
  }
}

void Sending() {
  InternetRetrievePhase();
  delay(500);
  digitalWrite(ESP_Sleep, LOW);
  while( digitalRead(15) == 0 )
  {
    millis();
  }
}

void setup() {
  delay(1000); // Delay for Serial Monitor initialization

  // Initialize the Serial Monitor
  Serial.begin(115200);

  //Define Pin modes
  pinMode(ESP_Sleep, OUTPUT);
  pinMode(Arduino_Sel, INPUT);
  pinMode(15, INPUT_PULLUP);

  // Wi-Fi Connection
  WiFiConnectSetup();

  // Wire Library
  Wire.begin(Slave_addr);

  Wire.onReceive(recieveEvent);
  Wire.onRequest(requestEvent);

  //delay(1000);
  //InternetRetrievePhase();

  digitalWrite(ESP_Sleep, HIGH);
  
  // Enable wake-up from External Pin
  esp_sleep_enable_ext0_wakeup(Arduino_Wake, LOW);

  //EEPROMRead();
}

void loop() {
sleep:

  Serial.println("Entering sleep");

  delay(1000);

  // Enter light sleep mode
  esp_light_sleep_start();

  Serial.println("Waking up!");

  if (WiFi.status() != WL_CONNECTED) 
  {
    WiFiConnectSetup();
  }

  //Decision between Cost Update or Sending Information
  //Decision = digitalRead(Arduino_Sel) == 1 ? 111 : 100;

  if(digitalRead(Arduino_Sel) == 1)
    Sending();
  else
    Updating();


  // Resetting values
  MesSize   = 0;
  w_data  = "0";

  //Signals the ESP is going to sleep
  digitalWrite(ESP_Sleep, HIGH);
}

