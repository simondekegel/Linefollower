/*
 ----------INSTRUCTIES HOE JE DEZE CODE GEBRUIKT EN WAARDES AANPAST------------
 -----------------------------------CB-----------------------------------------
 voor het instellen van de cyletime stuur in de serial monitor "set cyle 1000000"
 voor calibratie sensor wit stuur in serial monitor "calibrate white"
 voor calibratie sensor zwart stuur in serial monitor "calibrate black"
 om de opgeslagen waarden van white & black uit het eeprom geheugen te controleren stuur je in de serial monitor "debug"
 diff <0.5 anders gaat er 1 van de motoren niet achteruit
*/

#include "SerialCommand.h"
#include "EEPROMAnything.h"
#define SerialPort Serial
#define Baudrate 9600
bool run;
SerialCommand sCmd(SerialPort);
bool debug = false;
const int sensor[] = {A7, A6, A5, A4, A3, A2, A1, A0}; //sensoren van links naar rechts
unsigned long previous, calculationTime;
const int led = 12; // voor het testen van de verschillende voids
long normalised[8];//array voor de nieuwe genomaliseerde waardes
const int knop = 13;
float position;//postie van de bay harbor speeder
int status;
int index = 0;
float debugPosition;
float iTerm, lastErr;
// Motorpins
const int M1_IN1 = 3;
const int M1_IN2 =5 ;
const int M2_IN3 = 11;
const int M2_IN4 = 6;
//
//
//EEPROM start
struct param_t //dit is voor het EEPROM geheugen, deze waardes worden onthouden bij een reset
{
  unsigned long cycletime ;
  int black[8]; //waarde sensor zwart
  int white[8]; //waarde sensor wit
  float Kp;
  float diff;
  int power;
} params;
//EEPROM stop
//
//
void setup()
{
  SerialPort.begin(Baudrate);
  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.setDefaultHandler(onUnknownCommand);
  sCmd.addCommand("calibrate", onCalibrate);
  EEPROM_readAnything(0,params);
  SerialPort.println("ready");
  sCmd.addCommand("run", onRun);
  pinMode (led, OUTPUT);
  pinMode (knop, INPUT);
  status = LOW;
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN3, OUTPUT);
  pinMode(M2_IN4, OUTPUT);
}
//
//
//
void loop()
{ 
  Aanzetten ();
  sCmd.readSerial(); 
  unsigned long current = micros();// non blokking wait
  if (current - previous >= params.cycletime ) //cycletime in de params (EEPROM) omgeving
  {
    previous = current;
    //normalisatie start
    for (int i = 0; i < 8; i++)
    {
      long value = analogRead(sensor[i]);
      normalised[i] = map(value, params.black[i], params.white[i], 0, 1024);//herschaling van de sensor waarden
    }
    //normalisatie einde

  //herbekijk de video 
  /*volgende code zorgt voor de interpolatie van de sensor waarde. met een bereik van 30 t.e.m. -30 */                                                                                                         if (status == false){run = false;}  
  //
  
  for (int i=1; i < 6; i++) if (normalised[i] < normalised[index]) index = i;
  if(normalised[index]>500) run = false; //als de line-follower van de zwarte lijn is zal hij stoppen
  if (index == 0) position = -20;
  else if (index == 5) position = 20;
  else
  {
    int sNul = normalised[index];
    int sMinEen = normalised[index-1];
    int sPlusEen = normalised[index+1];
    float b = sPlusEen - sMinEen;
    b = b / 2;
    float a = sPlusEen - b - sNul;
    position = -b /(2 * a);
    position += index;
    position -= 3.5;
    position *= 15;
  }
    //
    //
    //PID
    debugPosition = position;
    float error = -position;
    float output = error * params.Kp;
    output = constrain(output, -510, 510); //-510 & 510 zijn de uiterste pwm waarde met de motoren
    int powerLeft =0;
    int powerRight =0;
  if(run == 1)if (output >= 0)
  {
  powerLeft = constrain(params.power + params.diff * output, -255, 255);
  powerRight = constrain(powerLeft - output, -255, 255);
  powerLeft = powerRight + output;
  }
  else
  {
  powerRight = constrain(params.power - params.diff * output, -255, 255);
  powerLeft = constrain(powerRight + output, -255, 255);
  powerRight = powerLeft - output;
  }
  analogWrite(M1_IN1, powerLeft > 0 ? powerLeft : 0);
  analogWrite(M1_IN2, powerLeft < 0 ? -powerLeft : 0);
  analogWrite(M2_IN3, powerRight > 0 ? powerRight : 0);
  analogWrite(M2_IN4, powerRight < 0 ? -powerRight : 0);
  }
  if(run == 0) {digitalWrite (led,LOW);}
  unsigned long difference = micros() - current; //-----geen idee wat dit doet
  if (difference > calculationTime) calculationTime = difference; //-----geen idee wat dit doet

}
//
//
//
void onUnknownCommand(char *command) //dit deel wordt uitgevoerd als onbekende wordt geschreven in de serialmonitor
{
  SerialPort.print("unknown command: \"");
  SerialPort.print(command);
  SerialPort.println("\"");
  digitalWrite (led,LOW);
}
//
//
//
void onSet() //dit deel wordt uitgevoerd als set wordt geschreven in de serialmonitor
{  
  char* param = sCmd.next();
  char* value = sCmd.next();
  if (strcmp(param, "cycle") == 0) params.cycletime = atol(value) ; //debug = false;
  else if (strcmp(param, "power")==0) params.power = atol(value); 
  else if (strcmp(param,"kp")==0) params.Kp = atof(value); //Kp is een float
  else if (strcmp(param,"diff")==0) params.diff = atof(value); //diff is een float
  EEPROM_writeAnything(0, params);
}
//
//
//
void onDebug() // debug, hier komen de belangrijke waarden om te debuggen
{
  SerialPort.print("cycle time: ");
  SerialPort.println(params.cycletime);
  //
  SerialPort.print("black: ");
  for (int i =0; i<7; i++ )
    {
      SerialPort.print(params.black[i]);
      SerialPort.print(" ");
    }
    SerialPort.println(" ");
  //
   SerialPort.print("white: ");
  for (int i =0; i<7; i++ )
    {
      SerialPort.print(params.white[i]);
      SerialPort.print(" ");
    }
    
    SerialPort.println(" ");
  //
    Serial.print("position: ");
    Serial.println(position);
  //
  /* parameters weergeven met behulp van het debug commando doe je hier ... */
  Serial.print("run: ");
  Serial.println(run);

  SerialPort.print("calculation time: ");
  SerialPort.println(calculationTime);
  calculationTime = 0;

  SerialPort.print("Kp: ");
  SerialPort.print(params.Kp);
  SerialPort.println(" ");

  SerialPort.print("power: ");
  SerialPort.print(params.power);
  SerialPort.println(" ");

  SerialPort.print("diff: ");
  SerialPort.print(params.diff);
  SerialPort.println(" ");
}
//
//
//
void onCalibrate() //calibratie voor de sensor
{
  char* param = sCmd.next();
  digitalWrite (led,HIGH);

  if (strcmp(param, "black") == 0) //calibratie als "black" wordt geschreven
  {
    SerialPort.print("start calibrating black... ");
    for (int i = 0; i < 8; i++) params.black[i]=analogRead(sensor[i]);
    SerialPort.println("done");
  }
  else if (strcmp(param, "white") == 0) //calibratie als "white" wordt geschreven
  {
    SerialPort.print("start calibrating white... ");    
    for (int i = 0; i < 8; i++) params.white[i]=analogRead(sensor[i]);  
    SerialPort.println("done");      
  }

  EEPROM_writeAnything(0, params);
}
//
//
//
void Aanzetten ()
{ 
  if (run == false)
  {
    if (digitalRead(knop) == 1)
    {
      run = true;
      digitalWrite (led,HIGH);
    }
  }
 
}
//
//
//
void onRun()
{
  char* parameter = sCmd.next();
  char* value = sCmd.next();
  if (strcmp(parameter, "on") == 0 )
  {
   iTerm = 0;
   run = true;
   digitalWrite (led,HIGH);
  }
  else run = false;
}