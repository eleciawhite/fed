#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <SdFat.h>                // SD and RTC libraries
SdFat SD;
#include "RTClib.h"
#include <Wire.h>
#include <Stepper.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <SoftwareSerial.h>


#define rxPin 255 // 255 because we don't need a receive pin
// Connect the Arduino pin 3 to the rx pin on the 7 segment display
#define txPin 9

SoftwareSerial LEDserial = SoftwareSerial(rxPin, txPin);


///
// any pins can be used
//#define SCK 13
//#define MOSI 11
//#define SS 4

//Adafruit_SharpMem display(SCK, MOSI, SS);

//#define BLACK 0
//#define WHITE 1

///

//int pokeCount = 0;

long previousMillis = 0;
long startTime = 0;
long timeElapsed = 0;
long day2 = 86400000; // 86400000 milliseconds in a day
long hour2 = 3600000; // 3600000 milliseconds in an hour
long minute2 = 60000; // 60000 milliseconds in a minute
long second2 =  1000; // 1000 milliseconds in a second

const int CS_pin = 10;
RTC_DS1307 RTC;    // refer to the real-time clock on the SD shield
String year, month, day, hour, minute, second, date, time;
String time2, time3;
File dataFile;

const int MOTOR_INPUT1_PIN = 7 ;
const int MOTOR_INPUT2_PIN = 6;
const int MOTOR_INPUT3_PIN = 5;
const int MOTOR_INPUT4_PIN = 4;
const int steps = 64;
const int MOTOR_STEPS_PER_REVOLUTION = 512;
const int MOTOR_ENABLE12_PIN = 8;
const int MOTOR_ENABLE34_PIN = 9;
const int TTL = 3;


int PIState = 1;
int lastState = 1;
int PIPin = 2;

int pelletCount = 0;

Stepper motor(MOTOR_STEPS_PER_REVOLUTION, MOTOR_INPUT1_PIN, MOTOR_INPUT2_PIN, MOTOR_INPUT3_PIN, MOTOR_INPUT4_PIN);

int logData() {
  DateTime datetime = RTC.now();
  year = String(datetime.year(), DEC);
  month = String(datetime.month(), DEC);
  day  = String(datetime.day(),  DEC);
  hour  = String(datetime.hour(),  DEC);
  minute = String(datetime.minute(), DEC);
  second = String(datetime.second(), DEC);

  // concatenates the strings defined above into date and time
  time = month + "/" + day + " " + hour + ":" + minute + ":" + second;

  // opens a file on the SD card and prints a new line with the
  // current reinforcement schedule, the time stamp,
  // the number of sucrose deliveries, the number of active and
  // inactive pokes, and the number of drinking well entries.

  dataFile = SD.open("PELLETJuly.csv", FILE_WRITE);
  if (dataFile) {
    Serial.println("File successfully written...");
    Serial.println(time);
    dataFile.print(time);
    dataFile.print(",");
    dataFile.print(pelletCount);
    dataFile.print(",");
    dataFile.println(timeElapsed);
    dataFile.close();
  }
}

void setMotorToTurn() {
    digitalWrite(MOTOR_ENABLE12_PIN, HIGH);
    digitalWrite(MOTOR_ENABLE34_PIN, HIGH);
    pinMode(MOTOR_INPUT1_PIN, OUTPUT);
    pinMode(MOTOR_INPUT2_PIN, OUTPUT);
    pinMode(MOTOR_INPUT3_PIN, OUTPUT);
    pinMode(MOTOR_INPUT4_PIN, OUTPUT);
}

void setMotorToSleep() {
    pinMode(MOTOR_INPUT1_PIN, INPUT);
    pinMode(MOTOR_INPUT2_PIN, INPUT);
    pinMode(MOTOR_INPUT3_PIN, INPUT);
    pinMode(MOTOR_INPUT4_PIN, INPUT);
    digitalWrite(MOTOR_ENABLE12_PIN, LOW);
    digitalWrite(MOTOR_ENABLE34_PIN, LOW);  
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up...");

  LEDserial.begin(9600); //Talk to the Serial7Segment at 9600 bps

  DDRD &= B00000011;       // set Arduino pins 2 to 7 as inputs, leaves 0 & 1 (RX & TX) as is
  DDRB = B00000000;        // set pins 8 to 13 as inputs
  PORTD |= B11111100;      // enable pullups on pins 2 to 7M
  PORTB |= B11111111;      // enable pullups on pins 8 to 13

  pinMode(MOTOR_ENABLE12_PIN, OUTPUT);
  pinMode(MOTOR_ENABLE34_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE12_PIN, HIGH);
  digitalWrite(MOTOR_ENABLE34_PIN, HIGH);

  pinMode(MOTOR_INPUT1_PIN, OUTPUT);
  pinMode(MOTOR_INPUT2_PIN, OUTPUT);
  pinMode(MOTOR_INPUT3_PIN, OUTPUT);
  pinMode(MOTOR_INPUT4_PIN, OUTPUT);
  pinMode(PIPin, INPUT);
  pinMode(TTL, OUTPUT);
  pinMode(CS_pin, OUTPUT);
  pinMode(SS, OUTPUT);
  pinMode(9, OUTPUT);

#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  if (!SD.begin(CS_pin)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1) ;
  }

  Serial.println("card initialized.");

  dataFile = SD.open("PELLETJuly.csv", FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog.txt");
    // Wait forever since we cant write data
    while (1) ;
  }

  else {
    dataFile.print(time);
    dataFile.println("Time,Pellet Count,Pellet Drop Delay");
    dataFile.close();
  }

  motor.setSpeed(35);



  delay (500);
}

void loop()
{
  PIState = digitalRead(PIPin);
  Serial.print("Photointerrupter State: ");
  Serial.println(PIState);
  digitalWrite(TTL, LOW);

  if (PIState == 1  & PIState != lastState) {
    setMotorToTurn();
  
    digitalWrite(TTL, HIGH);
    delay(50);
    digitalWrite(MOTOR_INPUT1_PIN, HIGH);
    digitalWrite(MOTOR_INPUT2_PIN, HIGH);
    digitalWrite(MOTOR_INPUT3_PIN, HIGH);
    digitalWrite(MOTOR_INPUT4_PIN, HIGH);
    startTime = millis();
    Serial.print("Time Elapsed Check: ");
    Serial.println(timeElapsed);
    timecounter(timeElapsed);
    logData();
    pelletCount ++;
    Serial.println("It did work");
    LEDserial.print("v"); // this is the reset display command
    delay(5); // allow a very short delay for display response
    LEDserial.print(pelletCount);
    //    Serial.println("It did work again");
    lastState = PIState;

  }
  else if (PIState == 1) {
    //Serial.print("Beam Broken: ");
    //Serial.println("No");
    Serial.println("Turning motor...");
    motor.step(-steps / 2);
    motor.step(steps);
    lastState = PIState;
  }
  
  else if (PIState == 0 & PIState != lastState) {
    Serial.print("Time Elapsed Since Last Pellet: ");
    timeElapsed = millis() - startTime;
    Serial.println(timeElapsed);
    lastState = PIState;
    
  }
    
  else  {
    //Serial.print("Beam Broken: ");
    //Serial.println("Yes");
    lastState = PIState;
    digitalWrite(MOTOR_INPUT1_PIN, LOW);
    digitalWrite(MOTOR_INPUT2_PIN, LOW);
    digitalWrite(MOTOR_INPUT3_PIN, LOW);
    digitalWrite(MOTOR_INPUT4_PIN, LOW);
    setMotorToSleep();
    enterSleep();

  }



  delay(1000);

  //  display.refresh();
  //  updateCount();
  //  pokeCount ++;
  //  delay(1000);

}

void timecounter(long timeNow) {

  Serial.println(timeNow);
  int days = timeNow / day2 ;                                //number of days
  int hours = (timeNow % day2) / hour2;                       //the remainder from days division (in milliseconds) divided by hours, this gives the full hours
  int minutes = ((timeNow % day2) % hour2) / minute2 ;         //and so on...
  int seconds = (((timeNow % day2) % hour2) % minute2) / second2;
  int mil = ((((timeNow % day2) % hour2) % minute2) % second2);

  // digital clock display of current time
  //Serial.print(days,DEC);
  //time2 = '';
  //time3 = 'Time';
  printDigits(hours); Serial.print(":");
  //time3 += time2; time3 += ":";
  printDigits(minutes); Serial.print(":");
  //time3 += time2; time3 += ":";
  printDigits(seconds); Serial.print(":");
  //time3 += time2; time3 += ":";
  printDigits(mil);
  //time3 += time2;
  Serial.println();
  Serial.println(time3);

}

void printDigits(byte digits) {
  // utility function for digital clock display: prints colon and leading 0
  //Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits, DEC);
  //time2 = String(digits);
}

void enterSleep()
{
  sleep_enable();

  attachInterrupt(0, pinInterrupt, RISING);
  lastState = 0;
  delay(100);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  cli();
  sleep_bod_disable();
  sei();
  sleep_cpu();


  sleep_disable();
}

void pinInterrupt(void)
{
  detachInterrupt(0);

  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */

  /* Re-enable the peripherals. */
  power_all_enable();

}

