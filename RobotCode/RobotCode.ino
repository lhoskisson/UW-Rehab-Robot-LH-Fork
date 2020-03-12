//Download library here https://learn.sparkfun.com/tutorials/tb6612fng-hookup-guide#library-and-example-code, manually import by copying library to User\Documents\Arduino\libraries
#include <SparkFun_TB6612.h>
#include "common.h"
#include "Timer.h"
#include "Log.h"
#include <SD.h>
#include <SPI.h>

using namespace defs; //inlcudes all definitions made in common.h

namespace //limits the scope of decalations inside namespace to this file.
{
Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY); //I need to check the motor code I wrote for testing to change these. I know offset needs to be changed
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY);

Timer calibrationTimer = Timer(CALIBRATION_TIME);
Timer outputTimer = Timer(SECOND);   // one second interval between outputs
Timer logTimer = Timer(SECOND * 30); // set a 30 second timer to log all values during that time.

Log sensorLog = Log(SD_CS, "sensorLog.csv");

File logFile = sensorLog.getLogFile();

int logIndex = 0;
int sensorLog[dataPoints][NUM_SENSORS];
int maxIR = 775;
int minIR = 230;
} //end namespace init

void setup()
//all actions that are only done once
{
  pinMode(13, OUTPUT); //only needed once so done in setup()
  Serial.begin(9600);
  Serial.println(F("Connected")); //quick check to make sure device is communicating

  startup();
  //calibrate();
}

void startup()
//
{
  welcomeMessage(); //will display a welcome message
  pinMode(BUTTON, INPUT);
  Timer calibrateStartupTimer = Timer(5 * SECOND);
  //wait for 5 seconds at startup. If button is pressed, recalibrate. otherwise, load from file.
  while (!calibrateStartupTimer.timeElapsed())
  {
    if (digitalRead(BUTTON))
    {
      calibrate();
      break;
    }
    else
    {
      Serial.println("reading calibration from file...");
      calibrationFromFile();
    }
  }
}

//////////////////////// loop

void loop()
{
  //Serial.print("loop");
  readSensor(); //Collects data from sensors and stores in an array

  IRdirection = getRatio();
  propForward(IRdirection);
  if (!logTimer.timeElapsed())
  {
  }
}

//This should be a better way to find a turning value

float getRatio()
{
  //Serial.print(F("getRatio"));
  int outputArr[NUM_SENSORS]; //Create temporary array to store values
  int sum = 0;                //Initialize sum variable

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    outputArr[i] = sensorDataRaw[i] * weightArray[i]; //stores weighted values
  }

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    if (i != 2)
    {
      sum = sum + outputArr[i];
    }
  }

  //This will include the center sensor. We're assuming that the center sensor has a fairly small value
  //(handled by the turn threshold), however if the line is between the center and inner sensors, we want to still
  //consider the center sensor value

  if (sum > 0)
  {
    sum = sum + (CENTER_WEIGHT * outputArr[2]);
  }
  else if (sum < 0)
  {
    sum = sum - (CENTER_WEIGHT * outputArr[2]);
  }

  //This will constrain sum incase somehow it reaches values we wouldn't expect (likely to come up for very sharp turns).
  //May want to try and remove this
  sum = constrain(sum, (-OUTER_WEIGHT * maxIR), (OUTER_WEIGHT * maxIR));

  //maps to 0 to 1, try and mess with this
  float temp = map(sum, (-OUTER_WEIGHT * maxIR), (OUTER_WEIGHT * maxIR), 0, 1000.00); //Values were in wrong order, they've been rearranged

  //Printing for debugging
  Serial.print("Sum: ");
  Serial.print(sum);

  Serial.print(" ratio: ");
  Serial.print(temp / 1000.00);

  Serial.println(" ");

  return (temp / 1000.00);
}

void readSensor()
{
  for (int x = 0; x < NUM_SENSORS; x++)
  {
    sensorDataRaw[x] = analogRead(IRSensor[x]);
  }
}

void blink()
//Changed blink to only blink once, but for the online function it calls blink() as long as the device isn't on a line
{
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
}

void propForward(float ratio)
{
  int speed1 = SPEED * ratio;
  int speed2 = SPEED * (1 - ratio);
  motor1.drive(speed1);
  motor2.drive(speed2);
}

void calibrate()
{
  //collects sensor data and defines the maximum and minimum line brightness values

  //initialize temporary max and min, set max equal to real min, min equal to real max
  int tempMax = SENSOR_MIN;
  int tempMin = SENSOR_MAX;
  Serial.println("calibrating...");
  //writeToScreen("calibrating..."); //so far unimplemented, to replace printing information to serial
  while (!calibrationTimer.timeElapsed())
  {               // perform calibration for set time.
    readSensor(); //populate sensor data array

    //find max and min over x minutes
    tempMax = getMax(tempMax);
    tempMin = getMin(tempMin);
  }
  Serial.println("calibration complete");
  Serial.println(tempMax);
  Serial.println(tempMin);

  //Now that we have the absolute max and min the sensors found, we can modify those to find an operational range. This will hopefully account for outliers
  //Modifiers can be increase or decreased as needed
  //Honestly, we might want to completely remove modifiers, as there's a chance in the IRdirection function it'll create negatives where we don't want them

  maxIR = 0.9 * tempMax;
  minIR = 1.1 * tempMin;
}

void printData()
{
  // prints debug info to serial connection
  Serial.println("calibrated min and max: ");
  Serial.println(maxIR);
  Serial.println(minIR);
  Serial.println();
  int readoutIndex = 0;
  while (readoutIndex < logIndex)
  {
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      Serial.print(sensorLog[readoutIndex][i]);
      Serial.print(", ");
    }
    Serial.println(readoutIndex);
  }
}

void logToSD()
//writes the current state of the sensor array to a file called "SensorLog.txt" one line at a time;
{

  if (logFile)
  {
    while (logFile.available())
    {
      for (int i = 0; i <)
      {
        for (int j = 0; j < NUM_SENSORS; j++)
        {
          logFile.print("");
        }
      }
    }
    logFile.close();
  }
  else
  {
    Serial.println("Fatal error: card read failed. reset device.");
    while (true)
      ;
  }
}

// smaller version of getMax and getMin using loops
int getMax(int tempMax)
{
  //Compares all readings from the IR sensors to the temporary max
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    tempMax = max(sensorDataRaw[i], tempMax); // sets tempMax to be the larger of the two between each sensor reading and the previous tempMax.
  }
  return tempMax; //Returns tempMax to the calibrate while loop
}

int getMin(int tempMin)
{
  //Compares all readings from the IR sensors to the temporary minimum.
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    tempMin = min(sensorDataRaw[i], tempMin);
  }
  return tempMin; //Returns tempMin to the calibrate while loop
}

void storeData()
{
  //This will store sensor data, use just for debugging/information presenting
  if (logIndex < dataPoints)
  { // checks whether or not sensorLog is full.
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      sensorLog[logIndex][i] = sensorDataRaw[i];
    }
    logIndex++; //increments the log index value.
  }
}

void welcomeMessage()
//displays a welcome message
{
}

void calibrationFromFile()
//sets the calibration values from the previous values stored on SD in "calibration.txt"
{
  if (!SD.begin(SD_CS))
  {
    Serial.println("SD card initialization failed.");
  }
  File calibration;
  if (SD.exists("calibration.txt"))
  {
    Serial.println("calibration file found.");
    calibration = SD.open("calibration.txt", FILE_READ);
  }
  else
  {
    Serial.println("calibration file not found.");
  }

  if (calibration)
  {
    while (calibration.available())
    {
      //Serial.write(calibration.read());
      maxIR = calibration.read();
      minIR = calibration.read();
      Serial.println(maxIR);
      Serial.println(minIR);
    }
    calibration.close();
  }
  else
  {
    Serial.println("read failed.");
    while (true)
      ;
  }
}