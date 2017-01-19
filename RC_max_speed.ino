#include <TinyGPS++.h>
#include <EEPROM.h>

// The TinyGPS++ object
TinyGPSPlus gps;
static byte numRecords = 0;
static byte oldestRecord = 0;
static float maxSpeed = 0.0f;
static float lastSpeed = 0.0f;
static long timer = 0;

//Check for !Serial before any writes
//This is to avoid adding a record when 
//data is just being accessed
void setup()
{
  Serial1.begin(9600);
  digitalWrite(13, LOW); //Make sure LED is off, waste of battery

  numRecords = EEPROM.read(0); //Read number of records stored
  oldestRecord = EEPROM.read(1); //Read oldest record ID
  
  numRecords += 1; //New record

  //If we don't have the memory for another record
  //We write over the oldest record and record that we did that
  if ((numRecords * sizeof(float)) + 2 >= EEPROM.length() && !Serial) {
    --numRecords;
    if (oldestRecord == 0) {
      oldestRecord = 1;
      EEPROM.write(1, oldestRecord);
    }
    else {
      ++oldestRecord;
      EEPROM.write(1, oldestRecord);
    }
  }
  
  if (!Serial) EEPROM.write(0, numRecords);
}

void loop()
{  
  while (Serial1.available() > 0) gps.encode(Serial1.read()); //pass serial data to gps module
  
  if (gps.speed.isValid()) { //if speed is ok read mph
    float temp = gps.speed.mps();
    if (temp > maxSpeed) maxSpeed = temp; //If returned speed is higher than current max record it
    }
  
  if (timer <= millis()) {//Every second check to see if max speed increased, if so write to eeprom
    if (maxSpeed > lastSpeed && numRecords != 0) {
      lastSpeed = maxSpeed;
      if (oldestRecord == 0 && !Serial) EEPROM.put(2 + ((numRecords - 1) * sizeof(float)), maxSpeed);
      else if (!Serial) EEPROM.put(2 + ((oldestRecord - 1) * sizeof(float)), maxSpeed);
    }
    timer = millis() + 1000;
  }
  
  if (Serial.available()) { //Serial monitor activity...
    char c = Serial.read();
    
    if (c == 'c' || c == 'C') Serial.println(gps.speed.mps()); //Print GPS current speed
    
    if (c == 'p' || c == 'P') { //Print speed records
      for (int i = 0; i < numRecords; ++i) {
        float readVal = 0.0f;
        if ((oldestRecord == 0 && i == 0) || (oldestRecord - 1) == i) Serial.print("(Oldest) ");
        Serial.print("Max speed[");
        Serial.print(i);
        Serial.print("]: ");
        EEPROM.get(2 + (i * sizeof(float)), readVal);
        Serial.println(readVal);
      }
    }
    
    if (c == 'E') { //Clear EEPROM
      Serial.print("Clearing EEPROM(");
      Serial.print(EEPROM.length());
      Serial.println(" bytes)...");
      numRecords = 0;
      oldestRecord = 0;
      for (int i = 0; i < EEPROM.length(); ++i) EEPROM.write(i, 0);
      Serial.println("EEPROM cleared.");
    }
  }
}
