/*
* CocoaTrak --- An open-source multi-sensor temperature logger for Cocoa Fermentation
* Copyright (c) 2014 Akbar, Iyok, Fakufaku (fakufaku[at]gmail[dot]com)
* All rights reserved.
*
* This code was written during Hackteria Lab 2014 in Yogyakarta Indonesia
* http://hackteria.org
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer. 
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* The views and conclusions contained in the software and documentation are those
* of the authors and should not be interpreted as representing official policies, 
* either expressed or implied, of the FreeBSD Project.
*/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

long previousMillis = 0;        // will store last time LED was updated
int ledState = LOW; 
long interval = 5000;           // interval at which to blink (milliseconds)

const int led1 = 3;
const int led2 = 4;
const int chipSelect = 10;
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress sensorAddress[10];

DeviceAddress SensorFactoryAddress[10] = 
{
	{0x28,0x3D,0xED,0x4F,0x5,0x0,0x0,0xC0},
	{0x28,0x64,0x6F,0xB5,0x5,0x0,0x0,0x5A},
	{0x28,0xDA,0x29,0x51,0x5,0x0,0x0,0x7C},
	{0x28,0x3,0x4B,0x48,0x5,0x0,0x0,0x17},
	{0x28,0x2C,0x70,0x48,0x5,0x0,0x0,0x71},
	{0x28,0x53,0x5E,0x40,0x5,0x0,0x0,0x8B},
	{0x28,0xA0,0x54,0x51,0x5,0x0,0x0,0xAA},
	{0x28,0x67,0x69,0x48,0x5,0x0,0x0,0x73},
	{0x28,0x57,0x42,0x48,0x5,0x0,0x0,0x16},
	{0x28,0x27,0xF2,0x4F,0x5,0x0,0x0,0x47}	
};

float sensorValue[10];


struct SensorID{
 int id;
 int factory_id; 
};

struct SensorID sensorID[10];

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("CocoaTrak ::Lifepatch::Biodesign::Hackterialab2014");
  Serial.println("By: Nur Akbar Arofatullah, Budi Prakosa and Robin Scheibler");
  // leds
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

   
  //Real Time Clock Initiation  
  #ifdef AVR
  Wire.begin();
  #else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
  #endif
   rtc.begin();
   
   if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    
  }
  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  //outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };

  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  // 
  // method 1: by index
  
  for (int i = 0; i < 10; i++)
  {
      if (!sensors.getAddress(sensorAddress[i], i)) Serial.println("Unable to find address for Device"); 
  }
  

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them.  It might be a good idea to 
  // check the CRC to make sure you didn't get garbage.  The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");
  // assigns the seconds address found to outsideThermometer
  //if (!oneWire.search(outsideThermometer)) Serial.println("Unable to find address for outsideThermometer");

  // show the addresses we found on the bus

  // set the resolution to 9 bit
  for (int i = 0; i < 10; i++)
  {
      sensors.setResolution(sensorAddress[i], TEMPERATURE_PRECISION);
  }
  
  //sensor id mapping;
  for (uint8_t i = 0; i < 10; i++)
  {    
      sensorID[i].id = i + 1;
      for (uint8_t j = 0; j < 10; j++)
      {
       if (isSensorAddressmatch(sensorAddress[j], SensorFactoryAddress[i])) sensorID[i].factory_id = j;
      }
  }
  
  
  
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
   digitalWrite(led1, HIGH);
   delay(200);
   digitalWrite(led1, LOW);
   delay(200);
   digitalWrite(led1, HIGH);
   delay(200);
   digitalWrite(led1, LOW);
   delay(200);
   digitalWrite(led1, HIGH);
   delay(200);
   digitalWrite(led1, LOW);
   delay(200);
   digitalWrite(led1, HIGH);
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print(" C: ");
  Serial.print(tempC);
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("ADDR: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
}


void setSensorFromAddress()
{
  
}


bool isSensorAddressmatch(DeviceAddress deviceAddress, DeviceAddress fixedSensorAddress)
{
  bool addrmatch = true;
  for (uint8_t i = 0; i < 8; i++)
  {
     if (deviceAddress[i] != fixedSensorAddress[i]) 
     {
       addrmatch = false;
     }
  }
  
  if (addrmatch)
  {
    return  true;
  }else{
    return false; 
  }

}


void loop(void)
{ 
  
    
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   
    
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
      
    
      // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
 // Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();

//printAddress(sensorAddress[0]);
//Serial.println();
//printAddress(SensorFactoryAddress[0]);

  /*
if (testPrintAddress(sensorAddress[0], 0)){
 Serial.println("FOUND"); 
}else{
   Serial.println("NOT FOUND"); 
}

 */
 /*
    for (uint8_t i = 0; i < 10; i++)
    {
         Serial.print("sensorID.id:");
         Serial.println(sensorID[i].id);
         Serial.print("sensorID.factory_id:");
         Serial.print(sensorID[i].factory_id);
         Serial.println();
    }
 */
 
   String dataString = "";

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("DATALOG.CSV", FILE_WRITE);

  
  
 
    for (uint8_t k = 0; k < 10; k++)
    {
      int factory_id = sensorID[k].factory_id;
      float tempC = sensors.getTempC(sensorAddress[factory_id]); 
      //printAddress(sensorAddress[factory_id]);
      //Serial.print(" id:" );   
      //Serial.print(sensorID[k].id);
      //Serial.print(" C:" );     
      
      dataString += String(parti(tempC));
      dataString += ".";
      dataString += String(partf(tempC,2));
      
     // Serial.print(tempC);
      
       //if (k != 9) Serial.print(",");
       if (k != 9) dataString += ",";

    }
  //Request RTC
        
    
    Serial.println();
    DateTime now = rtc.now();
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(' ');
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.print(',');
    Serial.println(dataString);
    
    
  // if the file is available, write to it:
  if (dataFile) {
      dataFile.print(now.year(), DEC);
      dataFile.print('/');
      dataFile.print(now.month(), DEC);
      dataFile.print('/');
      dataFile.print(now.day(), DEC);
      dataFile.print(' ');
      dataFile.print(now.hour(), DEC);
      dataFile.print(':');
      dataFile.print(now.minute(), DEC);
      dataFile.print(':');
      dataFile.print(now.second(), DEC);
      dataFile.print(',');
    dataFile.println(dataString);
    digitalWrite(led2, ledState);
    dataFile.close();
    delay(3600000);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening DATALOG.CSV");
  } 
    
    //intervals stop

  }

}


int parti(float x)
{
  return (int)x;
}

int partf(float x, int m)
{
  int i = parti(x);
  while (m > 0)
  {
    i *= 10;
    x *= 10;
    m--;
  }
  return x - i;
}

