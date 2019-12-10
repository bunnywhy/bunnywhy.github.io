#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;
Adafruit_ADS1115 ads1115;     // Construct an instance of an ADS1015 with the default address (0x48)  
HTTPClient http;
/* Pins in ESP8266 */
int MotorIn1Pin = 12;
int MotorIn2Pin =  15;
int AutoPin = 2;   // D4
int FakeDataPin = 13;   // D7
int UpPin = 14;    // D5
int DownPin = 16;   // D0
//int SDAPin = 4;
//int SCLPin = 5;

/* Some attributes */
bool Auto = true;
bool FakeData = false;
bool UP = false;
bool DOWN = false;
bool BadWeather = false;
bool needAC = false;
bool dontCare = false;
int mytime = 0;
int speed = 1000;
int motorStartTime = 0;
int prev_up = 0;    // previous UpPin value
int prev_down = 0;  // previous DownPin value
int level = 0;      // level: 0 - lowest, 3 - highest
int desiredLevel = 0;
int temperature = 15;
int desiredTemperature = 20;
int outsideTemperature = 20;

int prev_auto = 1;
int prev_fake = 1;
int PERIOD = 10000;

void setup() {
  // put your setup code here, to run once:
  pinMode(MotorIn1Pin, OUTPUT);
  pinMode(MotorIn2Pin, OUTPUT);
  pinMode(AutoPin, INPUT);     
  pinMode(FakeDataPin, INPUT); 
  pinMode(UpPin, INPUT); 
  pinMode(DownPin, INPUT);  
  Serial.begin(9600);  
  ads1115.begin();
  USE_SERIAL.begin(9600);
  for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
   }
   WiFiMulti.addAP("IllinoisNet_Guest", "");  
   setupDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:

  //monitor buttons and update desiredLevel, run motor
  //read value from sensor, and update temperature
  //if the current temperature is different from previous one, update the SSD
  // getADC(ads1115);
  displayValue(temperature);
  getSwithes();
  if (Auto) Serial.println("auto");
  if (FakeData) Serial.println("fake");
  if(!Auto) {
    driveMotorBySwitch();    
    return;
  }

  //monitor the button, update the desiredTemperature and SSD
 
  if((WiFiMulti.run() == WL_CONNECTED)) {
 
    getData(http);
  }
  else {
    if(FakeData) outsideTemperature = 40;
    else outsideTemperature = 0;
    BadWeather = false;
  }

  int measureProduct = (desiredTemperature-temperature)*(outsideTemperature-temperature);
  Serial.print("desiredTemperature: ");
  Serial.print(desiredTemperature);
  Serial.println();
  Serial.print("temperature: ");
  Serial.print(temperature);
  Serial.println();
  Serial.print("outsideTemperature: ");
  Serial.print(outsideTemperature);
  Serial.println();
  Serial.print("measureProduct: ");
  Serial.print(measureProduct);
  Serial.println();
  if(dontCare && !BadWeather)  { //meaning that we don't care
    if(measureProduct <= 4) desiredLevel = 0;
    else if(measureProduct <= 9) desiredLevel = 1;
    else if(measureProduct <= 16) desiredLevel = 2;
    else desiredLevel = 3;
  }
  if(BadWeather)  desiredLevel = 0;
  needAC = (desiredLevel == 0 && desiredTemperature-temperature > 2);


  adjustWindow();
  Serial.println(level);
  Serial.println(desiredLevel);
  //compare level and desiredLevel, and output AC
  //update desiredLevel, desiredTemperature, BadWeather   
    mytime++;
    //Serial.print(mytime);
    //int speed = Serial.parseInt();
    // Serial.print("time: "); Serial.println(mytime);
 

    
    delay(20);
}

void getSwithes() {
    int curr_auto = digitalRead(AutoPin);
    int curr_fake = digitalRead(FakeDataPin);
    
    if (curr_auto == 0 && prev_auto == 1) Auto = !Auto;
    if (curr_fake == 0 && prev_fake == 1) FakeData = !FakeData;

    prev_auto = curr_auto;
    prev_fake = curr_fake;
}


void adjustWindow() {
  int diff = level - desiredLevel;
  if(diff > 0 && diff < 4) {
     setMotor(speed, true);
     //motorStartTime = mytime;
     delay(PERIOD*diff);
     // while(mytime - motorStartTime > 50*(level - desiredLevel));
  }
  else if (diff < 0 && diff > -4) {
     //motorStartTime = mytime;
     setMotor(speed, false);
     delay(PERIOD*(0-diff));
     // while (mytime - motorStartTime > 50*(desiredLevel - level)); 

  }
  setMotor(0, true); 
  level = desiredLevel;
}
/* Detect Up and Down switches and drive motor accordingly */
void driveMotorBySwitch() {
    updateUpDownStatus();

    if (UP) {
    //if (UP) {
      if (level > 0){
        Serial.println("down");
        setMotor(speed, true);
        level--;
        delay(PERIOD);
      }
        setMotor(0, true); 
        UP = false; 
    } else if (DOWN) {
    //} else if (DOWN) {
      if (level < 3) {
        Serial.println("up");
        setMotor(speed, false);
        level++;
        delay(PERIOD);
      }
        DOWN = false;
        setMotor(0, true);  
    }
  Serial.println(level);
  Serial.println(desiredLevel);  
}

/* Update switches Up and Down status */
void updateUpDownStatus() {
    int curr_up = digitalRead(UpPin);
    int curr_down = digitalRead(DownPin);
    
    // detect if motor gets triggered for up or down
    if (prev_up == 1 && curr_up == 0 && !DOWN) {
        UP = true;
        motorStartTime = mytime;
    } 
    if (prev_down == 1 && curr_down == 0 && !UP) {
        DOWN = true;
        motorStartTime = mytime;
    } 
    
    prev_up = curr_up;
    prev_down = curr_down;
}

/* Get ADC: temp from temp sensor (adc0), obj distance from obj sensor (adc1) */
void getADC(Adafruit_ADS1115 ads1115) {
    int16_t adc0, adc1, adc2, adc3;
    
    adc0 = ads1115.readADC_SingleEnded(0);
    adc1 = ads1115.readADC_SingleEnded(1);
    adc2 = ads1115.readADC_SingleEnded(2);
    adc3 = ads1115.readADC_SingleEnded(3);

    // for temp sensor
    float temp = ((adc0 * 0.004882814) - 3.9) * 10;  
    if (mytime%20 == 0) {
        Serial.print("AIN0: "); Serial.println(temp);
        Serial.print("AIN1: "); Serial.println(adc1);
        Serial.print("AIN2: "); Serial.println(adc2);
        Serial.print("AIN3: "); Serial.println(adc3);
        Serial.println(" "); 
    }
}

/* Set motor speed and direction */
void setMotor(int speed, bool up) {   // if up, go up; else down
    if (up) {
      analogWrite(MotorIn1Pin, 0);
      analogWrite(MotorIn2Pin, speed*0.85);        
    } else {  // going down
      analogWrite(MotorIn1Pin, speed);
      analogWrite(MotorIn2Pin, 0);  
    }
} 


void sendData(HTTPClient &http, int desiredLevel, int desiredTemp) {
  String tmpURL = "http://ece445group84.web.engr.illinois.edu/backend.php?q=c" + String(desiredLevel) + String(desiredTemp);
  http.begin(tmpURL); //HTTP
  USE_SERIAL.print("[HTTP] GET (SEND DATA)...\n");
  http.GET();
  http.end();
}

void getData(HTTPClient &http) {
  http.begin("http://ece445group84.web.engr.illinois.edu/backend.php?q=cc"); //HTTP
  //USE_SERIAL.print("[HTTP] GET (GET DATA)...\n");
  http.GET();
  String payload = http.getString();
  http.end();
  //update desiredLevel, desiredTemperature, BadWeather, temperature
  //payload should have such format: levelDesire + space + TempDesired + space + isWeatherBad + space + currentTemp
  payload = payload.substring(payload.indexOf('\n')+1);
  if(payload.equals("error")) return;
  int newDesiredLevel = payload[0] - '0';
  if(newDesiredLevel < 0 || newDesiredLevel > 4) {
    Serial.println("error appear");
  } else {
    dontCare = newDesiredLevel == 4;
    if(!dontCare)
      desiredLevel = newDesiredLevel;
  }
  Serial.print("desiredLevel: ");
  Serial.print(desiredLevel);
  Serial.println();
  int secondSpaceIdx = payload.indexOf(' ', 2);
  desiredTemperature = payload.substring(2, secondSpaceIdx).toInt();
  BadWeather = payload[secondSpaceIdx+1] == '1';
  int newLinePos = payload.indexOf('\n');
  if(newLinePos != -1) outsideTemperature = payload.substring(secondSpaceIdx+2, newLinePos).toInt();
  else outsideTemperature = payload.substring(secondSpaceIdx+2).toInt();
  /*
  if (mytime % 10 == 0) {
    //Serial.println(payload);

    Serial.print("desired Level");
    Serial.println(desiredLevel);
    Serial.print("desiredTemperature");
    Serial.println(desiredTemperature);
    Serial.print("badWeather");
    Serial.println(payload[secondSpaceIdx+1]);
    Serial.print("outsideTemperature");
    Serial.println(outsideTemperature);    

  }
  */
}
