/********************************************************************************************
High-voltage DC PWM control Hot Dog Cooker.
FET isolator/driver to Arduino P7.
Beeper between P2 and 5V or 3.3V.
Magnetometer current sensor: A0 = sensor OUT-; A1 = OUT+; 5V = Vcc.
Magnetic interlock switch: Arduino A2 (HIGH = open).
20 x 4 I2C LCD Module connected to SDA and SCL.
NVE Corporation (email: iso-apps@nve.com or sensor-apps@nve.com), rev. 6/9/24
*********************************************************************************************/  
#include <Wire.h> //I2C for display
#include <LiquidCrystal_I2C.h> //LCD Library
LiquidCrystal_I2C lcd(0x27, 20, 4); //20 x 4 LCD Module with 0x27 I2C address 
byte dutyCycle, n; //Duty cycle (%), array counter
int cookTime, displayTime; //Running time, displayed cook time (sec.)
String minStr, secStr; //Displayed cook time in minutes and seconds

long current, avgCurrent, energy; //Sensor instantaneous and avg current (mA); total energy (J) 
long offset; //Current sensor offset (ADC reading with FET off)
const int cycleTime=100; //Duty cycle time (ms)
const int sensitivity=5500; //Current sensor sensitivity (µV/V/A)
const int breakerCurrent=3500, powerTarget=6500; //Max. current (mA); Energy target (J)
const byte profile[][3]={{0,30,60},{100,80,0}}; //Cooking stages: elapsed time, % pwr   

void setup() {
pinMode(7, OUTPUT); //Set P7 as output for FET drive
lcd.init();
lcd.clear();
lcd.write(0xFF); lcd.print("NVE Hot Dog Cooker");lcd.write(0xFF); //Display headings
lcd.setCursor(0,1); lcd.print("Time        Pwr    %");
lcd.setCursor(0,2); lcd.print("    amp       joules");
lcd.backlight();
dutyCycle=profile[1][0]; //Initial duty cycle
for(n=0; n<100; n++) offset+=analogRead(A1)-analogRead(A0); //Read current sensor with no current
offset/=100;
}
void loop() {
if (digitalRead(A2)) fault("COVER OPEN");

if (millis()%cycleTime > cycleTime*dutyCycle/100) {  //"Off" portion of duty cycle
digitalWrite(7, LOW); //Turn FET off for duty cycling
}
else { //"On" portion of duty cycle
digitalWrite(7, HIGH); //Turn on FET

//Read current sensor
current=(analogRead(A1)-analogRead(A0)-offset)*976562/sensitivity; //mA
current=abs(current);
avgCurrent=(avgCurrent*99+current)/100; //Digitally-filtered current
if (avgCurrent>breakerCurrent) fault("OVERCURRENT");
}
//Update display (once per second)
cookTime=millis()/1000; 
if (cookTime>displayTime) { minStr="0"+String(cookTime/60);
secStr="0"+String(cookTime-cookTime/60*60); 
lcd.setCursor(5,1);
lcd.print(minStr.substring(minStr.length()-2)+":"+secStr.substring(secStr.length()-2)); //Cook time
lcd.setCursor(0,2);
lcd.print(avgCurrent/1000);//Display current
lcd.print(".");
lcd.print((avgCurrent%1000)/100); 
energy+=avgCurrent*120/1000; //Cumulative energy (joules)
lcd.setCursor(12-int(log(float(energy))/log(10.)),2);
lcd.print(energy); //Display energy
lcd.setCursor(16,1);
if(dutyCycle<100) lcd.print(" "); 
lcd.print(dutyCycle);
displayTime=cookTime; //Time on the display
if (energy>=powerTarget) done ("ENERGY DONE");

for(n=0; n<sizeof(profile)/2; n++) { //Scan through cooking profile array
if (cookTime >= profile[0][n]) dutyCycle=profile[1][n]; }
if (dutyCycle==0) done("TIME'S UP");
}}

void done(String doneType){
digitalWrite(7, LOW); //Turn off FET
lcd.setCursor(0,3); 
lcd.print(doneType);
for(byte n=0; n<3; n++) { //beep three times
pinMode(2, OUTPUT); //Turn on beeper (output LOW)
delay (500);
pinMode(2, INPUT); //Turn off beeper (output tristate)
delay (500); }
while(true); //Stop
}
void fault(String faultType){
digitalWrite(7, LOW); //Turn off FET
lcd.setCursor(0,3); 
lcd.print(faultType); lcd.print("  ");
pinMode(2, OUTPUT); //Turn on beeper (output LOW)
delay (1000); //Long beep
pinMode(2, INPUT); //Turn off beeper (output tristate)
while(true) { //Monitor the cover interlock
lcd.setCursor(0,3); 
if (digitalRead(A2)&&(faultType!="COVER OPEN")) { //Cover wasn't open; now is
lcd.print("COVER OPEN          ");
faultType="COVER OPEN"; }
if ((digitalRead(A2)==0)&&(faultType=="COVER OPEN")) { //Cover was open; now isn't
lcd.print("                    ");
faultType=""; }
}}
