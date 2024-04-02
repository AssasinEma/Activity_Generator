//----------------------------------------------------------------------------------------------------//
//                                                                                                    //
//    Project: I2C Signals - Arduino                                                                  //
//                                                                                                    //
//    1234567890123456                                                                                //
//         Done!                                                                                        //
//                                                                                                    //
//----------------------------------------------------------------------------------------------------//

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C Bus adresses
#define   ESP32         10          // 
#define   LCDisplay     0x3F        // 
#define   EEPROM        80          // 1010 000

#define   Button          12
#define   ESP_Sleep       9
#define   Wakeup          5
#define   Arduino_Sel     8 
#define   Potentiometer   A0

#define   MESSIZE 32

LiquidCrystal_I2C lcd(LCDisplay, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

const unsigned int      cellAddress     =   10;

//This value takes care of the cooldown function of the button
const unsigned int      cooldown        =   1000;

//Used for code decisions
byte              decision        =   0;
byte              ready           =   0;

//Delay decision value
byte              delaybutton     =   0;

//State that handle the machine
byte              CURRENT_STATE   =   0;
byte              BEFORE_STATE    =   0;

//Reading values and digital pins
double            percent         =   0;
float             CostUpdate      =   0;
int               Pot             =   0;
volatile int      button          =   1;

//Time shenanigans
unsigned long     time1           =   0;
unsigned long     time2           =   0;
unsigned long     time3           =   0;
unsigned long     time4           =   0;

//Data
String    data      =   "";

//Error codes
// Timeout    - 408
// NULL       - 000
// Other      - 404
String     error    =   "000";

//This function takes care of LCD update
void UpdateLCD() {
  //Serial.println(CURRENT_STATE);

  //  For some weird reason state 7 is never displayed correctly in the switch.
  // I havent got a clue why at the moment, leave it as TO-DO
  if(CURRENT_STATE == 7)
  {
    BEFORE_STATE = CURRENT_STATE;
    lcd.setCursor(0, 0);
    lcd.print("     Again?     ");
    lcd.setCursor(0, 1);
    if (decision == 0)
      lcd.print(" >Yes<      No  ");
    else
      lcd.print("  Yes      >No< ");
  }


  switch (CURRENT_STATE) {
    case 0:
      BEFORE_STATE = CURRENT_STATE;
      lcd.setCursor(0, 0);
      lcd.print("    Welcome!    ");
      lcd.setCursor(0, 1);
      lcd.print(" Press for next ");
      break;

    case 1:
      BEFORE_STATE = CURRENT_STATE;
      lcd.setCursor(0, 0);
      lcd.print("     Select:    ");
      lcd.setCursor(0, 1);
      if (decision == 0)
        lcd.print(">Update<  Ideea ");
      else
        lcd.print(" Update  >Ideea<");
      break;

    case 2:
      BEFORE_STATE = CURRENT_STATE;
      lcd.setCursor(0, 0);
      lcd.print("  Please wait   ");
      lcd.setCursor(0, 1);
      lcd.print("Retrieving data.");
      break;

    case 3:
      BEFORE_STATE = CURRENT_STATE;
      lcd.setCursor(0, 0);
      lcd.print("The new cost is:");
      lcd.setCursor(0, 1);
      lcd.print("     ");
      lcd.print(percent);
      lcd.print("%      ");
      break;

    case 4:
      BEFORE_STATE = CURRENT_STATE;
      lcd.setCursor(0, 0);
      lcd.print("  Please wait   ");
      lcd.setCursor(0, 1);
      if(ready == 1)
        lcd.print("     Done!      ");
      else
        lcd.print("                 ");
      break;

    case 5:
      BEFORE_STATE = CURRENT_STATE;
      String first  = data.substring(0, 16);
      String second = data.substring(16);
      lcd.setCursor(0, 0);
      lcd.print(first);
      lcd.setCursor(0, 1);
      lcd.print(second);
      break;

    case 6:
      BEFORE_STATE = CURRENT_STATE;
      lcd.setCursor(0, 0);
      lcd.print("  Error! Code:  ");
      lcd.setCursor(5, 1);
      lcd.print(error);
      break;

    case 7:
      BEFORE_STATE = CURRENT_STATE;
      //Serial.println("S7");
      lcd.setCursor(0, 0);
      lcd.print("     Again?     ");
      lcd.setCursor(0, 1);
      if (decision == 0)
        lcd.print(" >Yes<      No  ");
      else
        lcd.print("  Yes      >No< ");
      break;

    default:
      CURRENT_STATE = 0;
      BEFORE_STATE = CURRENT_STATE;
      break;
  }
}

//This function handles the data getting sequence from the ESP32
void GetData() {
  UpdateLCD();
  digitalWrite(Arduino_Sel, HIGH);
  digitalWrite(Wakeup, LOW);

  delay(500);

  while(digitalRead(ESP_Sleep) == 1)
  {
    millis();
  }

  Serial.println("ESP Finished");

  delay(1000);

  Wire.requestFrom(ESP32, MESSIZE, true);
  String response = "";
  while(Wire.available())
  {
    char b = Wire.read();
    //Serial.print(b);
    response += b;
  }

  //Serial.println("");


  digitalWrite(Wakeup, HIGH);

  data = response;

  if(data[0] == 'w')
  {
    data = "Try programming!";
  }

  //Serial.println(response);

  CURRENT_STATE = 5;
}

//This function handle the cost update sequece with the EEPROM as well as the ESP32
void UpdateCost() {
  // Temp value to send to the ESP32
  char tempbuffer[4];
  ready = 0;
  // Temp value to store in the EEPROM
  int temp = CostUpdate * 100;

  UpdateLCD();
  digitalWrite(Arduino_Sel, LOW);
  dtostrf(CostUpdate, 3, 2, tempbuffer);
  //Serial.println(tempbuffer);

  digitalWrite(Wakeup, LOW);
  delay(500);
  //Serial.print(digitalRead(9));
  while(digitalRead(ESP_Sleep) == 1)
  {
    Wire.beginTransmission(ESP32);
    Wire.write(tempbuffer);
    Wire.endTransmission();

    delay(2000);
  }

  delay(1000);

  digitalWrite(Wakeup, HIGH);

  writeTo(EEPROM, cellAddress, temp);
  ready = 1;
}

//This function handles the EEPROM writing
void writeTo(int chAddress, unsigned int ceAddress, byte wData){
  Wire.beginTransmission(chAddress);
  Wire.write(ceAddress);
  Wire.write(wData);
  Wire.endTransmission();

  delay(5);
}

//This function handles the EEPROM reading and returns the read value
int readFrom(int chAddress, unsigned int ceAddress) {
  Wire.beginTransmission(chAddress);

  Wire.write((int)(ceAddress));
  Wire.endTransmission();

  Wire.requestFrom(chAddress,1);

  byte rData = 0;

  if(Wire.available()){
    rData = Wire.read();
  }

  return rData;
}

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);

  Wire.begin();

  lcd.init();         // initialize the lcd
  lcd.backlight();    // Turn on the LCD screen backlight

  pinMode(Button,         INPUT_PULLUP);   
  pinMode(ESP_Sleep,      INPUT_PULLUP); 
  pinMode(Wakeup,         OUTPUT);        
  pinMode(Arduino_Sel,    OUTPUT);        
  pinMode(Potentiometer,  INPUT);  

  digitalWrite(Wakeup , HIGH);       
}

void loop() {
  //The potentiomenter is only use/read in certain STATES
  if(CURRENT_STATE == 1 || CURRENT_STATE == 7) {
    Pot = analogRead(A0);
    if(Pot <= 512)
    {
      decision = 0;
    }
    else
    {
      decision = 1;
    }
  }

  //The percentage is only used for a single state
  if(CURRENT_STATE == 3) {
    Pot = analogRead(A0);
    //Serial.println(Pot);
    percent = (double)Pot / 970; // Calculate percentage
    if(percent > 1.00)
      percent = 1.00;
  }

  //The button is only read if it doesnt have a cooldown
  if(delaybutton == 0) {
    button = digitalRead(12);
    if(button == 0)
    {
      time1 = time2;
    }
  }

  //This function handles the cooldown
  time2 = millis();
  if(time1 + cooldown <= time2 ){
    delaybutton = 0;
  } else {
    delaybutton = 1;
  }

  //This part takes care of STATE transitions
  if (button == 0) {
    //Serial.print("---------------");
    //Serial.println(CURRENT_STATE + 1);
    switch (CURRENT_STATE) {
      case 0:
        //Serial.println(readFrom(EEPROM, cellAddress), DEC);
        CURRENT_STATE = 1;
        break;

      case 1:
        CURRENT_STATE = (decision == 0) ? 3 : 2;
        if(CURRENT_STATE == 2)
          time3 = millis();
        break;

      case 2:
        break;

      case 3:
        CostUpdate = percent;
        CURRENT_STATE = 4;
        UpdateCost();
        break;

      case 4:
        CURRENT_STATE = 0;
        break;

      case 5:
        lcd.clear();
        CURRENT_STATE = 7;
        break;

      case 6:
        CURRENT_STATE = 0;
        break;

      case 7:
        CURRENT_STATE = (decision == 0) ? 2 : 0;
        break;

      default:
        CURRENT_STATE = 0;
        break;
    }
  }

  //This ensures the states dont skip twice during the button cooldown
  button = 1;

  //Serial.println("beforeUpdate");

  UpdateLCD();

  if(CURRENT_STATE == 2) {
    GetData();
    lcd.clear();
  }

  delay(10);
}

