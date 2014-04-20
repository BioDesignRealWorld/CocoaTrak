/*
 * CocoaTrak --- An open-source multi-sensor temperature logger for Cocoa Fermentation
 * Copyright (c) 2014 Robin Scheibler, Nur Akbar, Iyok
 * All rights reserved.
 *
 * This code was written during Hackteria Lab 2014 in Yogyakarta Indonesia
 * http://hackteria.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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

#include <Wire.h>
#include <SD.h>
#include <DHT.h>
#include <DS1302.h>

#include <limits.h>


// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// pin declaration
int pin_DHT0 = A0;
int pin_DHT1 = A1;
int pin_DHT2 = A2;
int pin_DHT3 = A3;
int pin_led = 7;
int pin_sd_cs = 5;
int pin_rtc_ce = 6;
int pin_rtc_data = 4;
int pin_rtc_sclk = 3;

// RTC object
DS1302 rtc(pin_rtc_ce, pin_rtc_data, pin_rtc_sclk);
Time t;

// setup 5 temperature sensors (because that's what we have)
#define NSENSORS 4
DHT dht0(pin_DHT0, DHTTYPE);
DHT dht1(pin_DHT1, DHTTYPE);
DHT dht2(pin_DHT2, DHTTYPE);
DHT dht3(pin_DHT3, DHTTYPE);
// store all the objects in an array because it's neat
DHT sensors[NSENSORS] = { dht0, dht1, dht2, dht3 };

double humi[NSENSORS] = {0};
double temp[NSENSORS] = {0};

// filename to log the temperature
const char filename[] = "temp_log.csv";
File datafile;
unsigned long last_write;
int write_now = 1;

// log everything every 5 minutes
//unsigned long write_delay = 300000; // in ms
unsigned long write_delay = 5000; // in ms

void setup() 
{
  int i;

  // init serial com
  Serial.begin(57600); 
  Serial.println("Super Cocoa Logger!!");
  Serial.println("by Lifepatch/Biodesign");

  // initialize all sensors
  for (i = 0 ; i < NSENSORS ; i++)
    sensors[i].begin();

  // set pin 10 as OUTPUT to make sure SPI does not go in slave mode
  pinMode(10, OUTPUT);
  // initialize the SD card
  if (!SD.begin(pin_sd_cs)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // make sure RTC is running
  rtc.halt(false);
  rtc.writeProtect(false);
}

void loop() 
{
  int i;

  // check the time before we decide to write
  if (ellapsed(last_write) > write_delay)
    write_now = 1;

  if (write_now)
  {
    // read the time!
    t = rtc.getTime();

    // readout all the sensors
    for (i = 0 ; i < NSENSORS ; i++)
    {
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      humi[i] = sensors[i].readHumidity();
      temp[i] = sensors[i].readTemperature();

      // check if returns are valid, if they are NaN (not a number) then something went wrong!
      if (isnan(temp[i]) || isnan(humi[i])) 
      {
        Serial.print("Failed to read from DHT number ");
        Serial.println(i);
      } 
    }

    // write to the SD card
    char line[100];

    int l = sprintf_P(line, PSTR("%04d-%02d-%02dT%02d:%02d:%02d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d"), \
    (int)t.year, (int)t.mon, (int)t.date, \
    (int)t.hour, (int)t.min, (int)t.sec, \
    parti(temp[0]), partf(temp[0],2), \
    parti(temp[1]), partf(temp[1],2), \
    parti(temp[2]), partf(temp[2],2), \
    parti(temp[3]), partf(temp[3],2), \
    parti(humi[0]), partf(humi[0],2), \
    parti(humi[1]), partf(humi[1],2), \
    parti(humi[2]), partf(humi[2],2), \
    parti(humi[3]), partf(humi[3],2));


    // print to the SD card
    datafile = SD.open("TEMPLOG.CSV", FILE_WRITE);
    if (datafile)
    {
      // print to log file
      datafile.println(line);
      datafile.close();

      // display in serial
      Serial.println(line);
    }
    else
      Serial.println("Error opening log file.");

    // record time
    last_write = millis();
    write_now = 0;
  }
}

// compute mean of array
float mean(float *a, int N)
{
  if (N <= 0)
    return 0;

  int i;
  float m = a[0];

  for (i = 1 ; i < N ; i++)
    m += a[i];

  return m/N;
}

// compute time ellapsed since a timestamp
unsigned long ellapsed(unsigned long timestamp)
{
  unsigned long now = millis();
  if (now - timestamp < 0)
    return ULONG_MAX - now + timestamp;
  else
    return now - timestamp;
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
