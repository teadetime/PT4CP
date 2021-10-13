//What nicola thinks is probablythe most recent from summer 2021 but we can check and update
////////////////PRESSURE INITIAL /////////////////////////////////////////////////
////Pins for Pump and Solenoids
int pump = 3; // Pump connected (Needs to be a pin with a tilda so it can do PWM) 
int switchValve = 4; // 3:2 solonoid connected
/*The 3:2 Solonoid swiches between connecting the pump to the flexion and extension system.  
  The valve is set up to be switched to extension when unpowered and switched to flexion when powered.
*/
int flxValve = 5; //exhaust valve for flexion system
int extValve = 6; //exhaust valve for extension system, powered opens
/*Both 2 way valves open when powered. Eg. The flexion system will allow for exhausting when flxValve is powered*/


//Pins for Pressure Valves:
int extPressure = A0; //The Pressure Sensor monitering the Extension system
int flxPressure = A1;  //The Pressure Sensor monitering the Flexion system

//Pins for Switches
int killPin = 8; //The kill switch/wire
int switchPin = 9; //The on/off switch
//Set Presure
int maxPressureExt = 15; //Set the desired maximum PSI for the extension bladders in each extension cylce
int maxPressureFlx = 15; //Set the desired maximum PSI for the flexion bladders in each extension cylce

///MANUALLY CONTROL FOR PERFORMACE
float maxStraight = 15.5;
float maxBent = 6.5;

float safetyPressureExt = 16;
float safetyPressureFlx = 15;

// Introduce Variables
int pressureRawExt, pressureRawFlx, psiExt, psiFlx, psiInitExt, psiInitFlx; //Initialize variables to use later to calculate the PSI

//Create and Set States
int switchState; // Initializes variable for if switch is on or off
int killSwitch; // Initializes variable for the result of reading the kill switch
int inflationState = 1; //Tracks if glove should be extending or flexing, when 0 it will extend
int killedState = 0; // Initializes variable for if kill switch has been triggered since reset and starts it at 0

//Set Serial Plotter Scale
/* These are only used to prevent serial plotter auto scaling*/
int scaleMax = 20; 
int scaleMin = 0;


////////////////POSITION INITIAL /////////////////////////////////////////////////
#include <SparkFun_ADS1015_Arduino_Library.h>
#include <Wire.h>

ADS1015 pinkySensor;
ADS1015 indexSensor;
float hand[4] = {0, 0, 0, 0};
//Calibration Array
uint16_t handCalibration[4][2] = {
//{low, hi} switch these to reverse which end is 1 and which is 0
  {701, 1020}, //index
  {768, 958},  //middle
  {680,950},  //ring
  {405,830}//{736, 907},  //pinky
};

///////////////////////////////////////////////////////////////////////////////
void setup() {
////////////PRESSURE SETUP /////////////////////////////////////////////////
pinMode(pump, OUTPUT);
pinMode(switchValve, OUTPUT);
pinMode(extValve, OUTPUT);
pinMode(flxValve, OUTPUT);

pinMode(switchPin, INPUT);
pinMode(killPin, INPUT);

Serial.begin(9600);

analogWrite(pump, 0);
digitalWrite(extValve, HIGH);
digitalWrite(flxValve, HIGH);

//Code to subtract initial value from pressure sensor:
//delay (3000);
//pressureRawExt = analogRead(extPressure);
//pressureRawFlx = analogRead(flxPressure);
//float psiInitExt = 25*(pressureRawExt)*(5.0/1023.0) - 12.5;
//float psiInitFlx = 25*(pressureRawFlx)*(5.0/1023.0) - 12.5;
//Serial.println(psiInitExt);
//Serial.println(psiInitFlx);


////////////////POSITION SETUP /////////////////////////////////////////////////
  
  Wire.begin();
  Serial.begin(9600);
  
  //Begin our finger sensors, change addresses as needed.
  if (pinkySensor.begin(ADS1015_ADDRESS_SDA) == false) 
  {
     Serial.println("Pinky not found. Check wiring.");
     while (1);
  }
  if (indexSensor.begin(ADS1015_ADDRESS_GND) == false) 
  {
     Serial.println("Index not found. Check wiring.");
     while (1);
  }
  
  pinkySensor.setGain(ADS1015_CONFIG_PGA_TWOTHIRDS); // Gain of 2/3 to works well with flex glove board voltage swings (default is gain of 2)
  indexSensor.setGain(ADS1015_CONFIG_PGA_TWOTHIRDS); // Gain of 2/3 to works well with flex glove board voltage swings (default is gain of 2)  
  
  //Set the calibration values for the hand.
  for (int channel; channel < 2; channel++)
  {
    for (int hiLo = 0; hiLo < 2; hiLo++)
    {
      indexSensor.setCalibration(channel, hiLo, handCalibration[channel][hiLo]);
      pinkySensor.setCalibration(channel, hiLo, handCalibration[channel + 2][hiLo]);
    }
    Serial.println();

  }
}
 
/////////////////////////////////////////////////////////////////////////////
void loop() {
////////////////POSITION LOOP /////////////////////////////////////////////////
for (int channel = 0; channel < 2; channel++)
  {
    //Keep in mind that getScaledAnalogData returns a float
    hand[channel] = indexSensor.getScaledAnalogData(channel);
    hand[channel + 2] = pinkySensor.getScaledAnalogData(channel);
  }
  
  for (int finger = 0; finger < 4; finger++)
  {

    Serial.print(hand[finger]*20);
    Serial.print(" ");
  }
 //Serial.println();


////////////////PRESSURE LOOP /////////////////////////////////////////////////  
//Read Pressure Sensors and convert to PSI:
pressureRawExt = analogRead(extPressure);  
pressureRawFlx = analogRead(flxPressure);
float psiExt = 25*(pressureRawExt)*(5.0/1023.0) - 12.5; //- psiInitExt;
float psiFlx = 25*(pressureRawFlx)*(5.0/1023.0) - 12.5; //- psiInitFlx;
  
////////Print version for MATLAB////////////
  Serial.print(psiExt);
  Serial.print("  ");
  Serial.print(psiFlx);
  Serial.println();

///////Print version for Serial Plotter///////
//  Serial.print("ScaleMax: ");
//  Serial.print(scaleMax);
// Serial.print("  ");
//  Serial.print("ScaleMin: ");
//  Serial.print(scaleMin);
//  Serial.print("  ");
//  Serial.print("Extension PSI:");
//  Serial.print(psiExt);
//  Serial.print("  ");
//  Serial.print("Flex PSI:");
// Serial.print(psiFlx);
// Serial.println();
  
//Check Switch States
killSwitch = digitalRead(killPin);
switchState = digitalRead(switchPin);
if (killedState == 1) {
  //Turn off Pump and Open all Exhaust Valves:
  digitalWrite(pump, LOW);
  digitalWrite(extValve, HIGH);
  digitalWrite(flxValve, HIGH);
}


else if (killedState == 0) { //Only move into the rest of the code if kill switch has never been triggered:
  if (killSwitch == HIGH){  //Check if kill switch is currently triggered
    Serial.println("Killed"); 
    killedState = 1; //Tell the program that kill switch has been triggered
    
  }
 
  else if (switchState == HIGH) { 
  
     if (inflationState == 0) { //Extension
      if (hand[0]*20 < maxStraight) { //(psiExt < maxPressureExt) {
         digitalWrite(switchValve, LOW); //Switch to Inflating Extension
         digitalWrite(pump, HIGH); //Turn Pump On
         digitalWrite(extValve, LOW); //Close Extension Exhaust

         
         digitalWrite(flxValve, HIGH); //Open Flexion Exhaust
         //Serial.println("Extending");
         if (psiExt > safetyPressureExt) { //If extension bladder gets too high pressure
            inflationState = 0.1; //Tell Program to switch to flexion
         }
         }
      else {// if (psiExt >= maxPressureExt) {
        inflationState = 1; //Tell Program to switch to flexion
        //digitalWrite(pump, 1); //Turn Pump down?
        //delay(1000);
        }
    }
    
    if (inflationState == 1) { //Flexion
      if (hand[0]*20 > maxBent ) {//(psiFlx < maxPressureFlx) {
         digitalWrite(switchValve, HIGH); //Switch to Inflating Flexion
         digitalWrite(pump, HIGH); //Turn Pump On
         digitalWrite(flxValve, LOW); //Close Flexion Exhaust

         
         digitalWrite(extValve, HIGH); //Open Extension Exhaust
         //Serial.println("Flexing");
          if (psiFlx > safetyPressureFlx) { //If extension bladder gets too high pressure
            inflationState = 0; //Tell Program to switch to flexion
         }
         }
      else {//if  (psiFlx >= maxPressureFlx) {
        inflationState = 0; //Tell Program to switch to extension
        }
    }
  }
  
  else { //If switch is off
       digitalWrite(pump, LOW); //Turn Pump Off
       
       //Keep Exhaust Valves Closed (to hold pressure)
       digitalWrite(extValve, LOW); 
       digitalWrite(flxValve, LOW);
       
       //Leave Switch Valve how it is
  }

}
}
