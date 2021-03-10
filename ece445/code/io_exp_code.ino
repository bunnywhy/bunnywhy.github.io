/*

  Arduino demo code for the PCA9535 I2C port expander.
  Compatible with Arduino IDE version 1.0.5
  Last modified: 26 January 2014  

*/

#include <Wire.h>

//  I2C 9535 device address is 0 1 0 0   0 0 0 (0x20, or 0d32)
#define PCA9535_ADDR 32

int ch0=0;
int ch1=0;
// variables to be displayed on the 7 segment display
// int temperature = 0;
// bool needAC = false;
int mask = B00000001;
int led0 = B11111111; // 7..0
int led1 = B11111110; // 8..15
int value = 0;

/* test code
 *   mask = mask << 1;  // left shift once
  led1 = ~mask;
 */

// led display mapping
int LOWD[10][2];   // LOWD[digit][led_]
int HIGHD[10][2];  // HIGHD[digit][led_]


void setupDisplay() 
{
  
  Wire.begin();

  // setup channel 0 and 1
  Wire.beginTransmission(PCA9535_ADDR);
  int w1 = Wire.write(6); // configure port 0 registers
  int w2 = Wire.write(B00000000); // set all as output
  int ret = Wire.endTransmission();

  Wire.beginTransmission(PCA9535_ADDR);
  int w4 = Wire.write(7); // configure port 1 registers
  int w5 = Wire.write(B00000000); // set all as output
  int ret2 = Wire.endTransmission();

  initDisplayMapping();
  
  //Serial.begin(9600);
}


void loopDisplay() 
{
  // channel 0
  
  Wire.beginTransmission(PCA9535_ADDR);
  Wire.write(2); // select IO channel 0
  Wire.write(led0); 
  Wire.endTransmission();

  Wire.beginTransmission(PCA9535_ADDR);
  Wire.write(3); // select IO channel 1
  Wire.write(led1); 
  Wire.endTransmission();

  
  displayValue(value);
  value++;
  needAC = !needAC;
  delay(1000);
  
}

/*
 * display input value
 */
void displayValue(int value) {
  int L0 = B00000000;
  int L1 = B00000000;

  int digit0 = value % 10;
  int digit1 = value / 10;

  L0 |= LOWD[digit0][0];
  L0 |= HIGHD[digit1][0];
  L1 |= LOWD[digit0][1];
  L1 |= HIGHD[digit1][1];
  if (needAC) L1 |= B00000001;

  led0 = ~L0;
  led1 = ~L1;
  
  Wire.beginTransmission(PCA9535_ADDR);
  Wire.write(2); // select IO channel 0
  Wire.write(led0); 
  Wire.endTransmission();

  Wire.beginTransmission(PCA9535_ADDR);
  Wire.write(3); // select IO channel 1
  Wire.write(led1); 
  Wire.endTransmission();

}    

/*
 * init led display mapping [digit_][led_]
 */
void initDisplayMapping() {

  LOWD[0][0] = B11100000;
  LOWD[0][1] = B00001110;
  LOWD[1][0] = B10000000;
  LOWD[1][1] = B00000010;
  LOWD[2][0] = B11010000;
  LOWD[2][1] = B00001100;
  LOWD[3][0] = B11010000;
  LOWD[3][1] = B00000110;
  LOWD[4][0] = B10110000;
  LOWD[4][1] = B00000010;
  LOWD[5][0] = B01110000;
  LOWD[5][1] = B00000110;
  LOWD[6][0] = B01110000;
  LOWD[6][1] = B00001110;
  LOWD[7][0] = B11000000;
  LOWD[7][1] = B00000010;
  LOWD[8][0] = B11110000;
  LOWD[8][1] = B00001110;    
  LOWD[9][0] = B11110000;
  LOWD[9][1] = B00000110;

  HIGHD[0][0] = B00001110;
  HIGHD[0][1] = B11100000; 
  HIGHD[1][0] = B10001000;
  HIGHD[1][1] = B00100000;   
  HIGHD[2][0] = B00001101;
  HIGHD[2][1] = B11000000;   
  HIGHD[3][0] = B00001101;
  HIGHD[3][1] = B01100000;  
  HIGHD[4][0] = B00001011;
  HIGHD[4][1] = B00100000;
  HIGHD[5][0] = B00000111;
  HIGHD[5][1] = B01100000;
  HIGHD[6][0] = B00000111;
  HIGHD[6][1] = B11100000;
  HIGHD[7][0] = B00001100;
  HIGHD[7][1] = B00100000;
  HIGHD[8][0] = B00001111;
  HIGHD[8][1] = B11100000;
  HIGHD[9][0] = B00001111;
  HIGHD[9][1] = B01100000;
   
}

