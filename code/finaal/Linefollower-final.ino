/*
 ----------INSTRUCTIES HOE JE DEZE CODE GEBRUIKT EN WAARDES AANPAST------------
 -----------------------------------CB-----------------------------------------
 voor de waardes intestellen (diff, power, cycle,...) stuur je in de serial monitor => "set (naam) (value)"
 voor het instellen van de cyletime stuur in de serial monitor "set cyle 1000000", cycle time voor goede werking <500
 *
 voor calibratie sensor wit stuur in serial monitor "calibrate white" ----aangeraden om voor elk gebruik opnieuw tedoen, waardes kunnen veelverschillen afhankelijk van omgeving
 voor calibratie sensor zwart stuur in serial monitor "calibrate black" ----aangeraden om voor elk gebruik opnieuw tedoen, waardes kunnen veelverschillen afhankelijk van omgeving
 *
 om de opgeslagen waarden van het eeprom geheugen te controleren stuur je in de serial monitor "debug"
 diff <0.5 anders gaat er 1 van de motoren niet achteruit!!! (fysiek motor rechts, in de programmatie is dit motor links, ben te lui om dit aantepassen)
 *
 
*/
#include "SerialCommand.h"
#include "EEPROMAnything.h"
#define SerialPort Serial
#define Baudrate 9600
bool run;  // variabele om de line follower aantezetten, zowel fysiek als via de serialmonitor
SerialCommand sCmd(SerialPort);
bool debug = false;
const int sensor[] = { A7, A6, A5, A4, A3, A2, A1, A0 };  //sensoren van links naar rechts
unsigned long previous, calculationTime;
const int led = 12;  // fysieke signalisatie om te weten of run aanstaat
long normalised[8];  //array voor de nieuwe genomaliseerde waardes
const int knop = 13;
float position;  //postie van de bay harbor speeder
int status = 0;
int index = 0;
float debugPosition;
float iTerm, lastErr;
// Motorpins
const int M1_IN1 = 3;
const int M1_IN2 = 5;
const int M2_IN3 = 11;
const int M2_IN4 = 6;
//
//
//EEPROM start
struct param_t  //dit is voor het EEPROM geheugen, deze waardes worden onthouden bij een reset
{
  unsigned long cycletime;
  int black[8];  //waarde sensor zwart
  int white[8];  //waarde sensor wit
  float Kp;
  float Ki;
  float Kd;
  float diff;
  int power;
} params;
//EEPROM stop
//
//
void setup() {
  ADCSRA &= ~((1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0)); // testen, geeft extra ruis
  ADCSRA |= (1<<ADPS2); // testen, geeft extra ruis // geeft stabiele sensor waarde maar een calc time tijdens run van 792µs
  //ADCSRA = (ADCSRA & 0b11111000) | 0b010; //geeft calc tijd van 712µs maar iets minder stabiele sensor waarden 
  SerialPort.begin(Baudrate);
  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.setDefaultHandler(onUnknownCommand);
  sCmd.addCommand("calibrate", onCalibrate);
  EEPROM_readAnything(0, params);
  SerialPort.println("ready");
  sCmd.addCommand("run", onRun);
  pinMode(led, OUTPUT);
  pinMode(knop, INPUT);
  status = LOW;
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN3, OUTPUT);
  pinMode(M2_IN4, OUTPUT);
}
//
//
//
void loop() {
  Aanzetten();
  sCmd.readSerial();
  unsigned long current = micros();            // non blokking wait
  if (current - previous >= params.cycletime)  //cycletime in de params (EEPROM) omgeving
  {
    previous = current;
    //normalisatie start
    for (int i = 0; i < 8; i++) {
      long value = analogRead(sensor[i]);
      normalised[i] = map(value, params.black[i], params.white[i], 0, 1024);  //herschaling van de sensor waarden
    }
    //normalisatie einde
    //herbekijk de video interpolatie
    /*volgende code zorgt voor de interpolatie van de sensor waarde. met een bereik van 30 t.e.m. -30 */                                                                                                                                                                                                                                                                                                          if (status == 0){run = 0;}   
    //
    for (int i = 1; i < 6; i++)
      if (normalised[i] < normalised[index]) index = i;
    if (normalised[index] > 500) run = false;  //als de line-follower van de zwarte lijn is zal hij stoppen
    if (index == 0) position = -20;
    else if (index == 5) position = 20;
    else {
      int sNul = normalised[index];
      int sMinEen = normalised[index - 1];
      int sPlusEen = normalised[index + 1];
      float b = sPlusEen - sMinEen;
      b = b / 2;
      float a = sPlusEen - b - sNul;
      position = -b / (2 * a);
      position += index;
      position -= 3.5;
      position *= 15;
    }
    //
    //
    //PID
    if (run == 1)  //Voor de Iterm, zodat deze pas zal werken als de auto actief is, DUBBEL CHECKEN
    {
      debugPosition = position;
      float error = -position;
      float output = error * params.Kp;  // output berekening met Kp
      //
      //I regelaar start
      //iterm mag enkel worden uitgevoerd tijdens het rijden, anders zal deze zeer snel naar zen maximum lopen
      //
      iTerm += params.Ki * error;  // output berekening met Ki
      iTerm = constrain(iTerm, -510, 510);
      output += iTerm;  // zorgt ervoor dat de output geen sprongen maakt als ki opeens van waarde verandert
      // i regelaar einde
      //
      //d-regelaar start
      output += params.Kd * (error - lastErr);
      lastErr = error;
      //einde d-rgelaar
      //
      output = constrain(output, -510, 510);  //-510 & 510 zijn de uiterste pwm waarde met de motoren
      int powerLeft = 0;
      int powerRight = 0;

        if (output >= 0) {
          powerLeft = constrain(params.power + params.diff * output, -255, 255);  //limiteerd de uiterste pwm waarde van de motor
          powerRight = constrain(powerLeft - output, -255, 255);                  //limiteerd de uiterste pwm waarde van de motor
          powerLeft = powerRight + output;
        } else {
          powerRight = constrain(params.power - params.diff * output, -255, 255);  //limiteerd de uiterste pwm waarde van de motor
          powerLeft = constrain(powerRight + output, -255, 255);                   //limiteerd de uiterste pwm waarde van de motor
          powerRight = powerLeft - output;
        }
      analogWrite(M1_IN1, powerLeft > 0 ? powerLeft : 0);
      analogWrite(M1_IN2, powerLeft < 0 ? -powerLeft : 0);
      analogWrite(M2_IN3, powerRight > 0 ? powerRight : 0);
      analogWrite(M2_IN4, powerRight < 0 ? -powerRight : 0);
    }
    if (run == 0) { 
      digitalWrite(led, LOW);
      analogWrite(M1_IN1, LOW);
      analogWrite(M1_IN2, LOW);
      analogWrite(M2_IN3, LOW);
      analogWrite(M2_IN4, LOW);
     }
    unsigned long difference = micros() - current;                   //-----geen idee wat dit doet
    if (difference > calculationTime) calculationTime = difference;  //-----geen idee wat dit doet
  }
}
//
//
//
void onUnknownCommand(char* command)  //dit deel wordt uitgevoerd als onbekende commandos worden geschreven in de serialmonitor
{
  SerialPort.print("unknown command: \"");
  SerialPort.print(command);
  SerialPort.println("\"");
  run = false;
  
}
//
//
//
void onSet()  //dit deel wordt uitgevoerd als set wordt geschreven in de serialmonitor
{
  char* param = sCmd.next();
  char* value = sCmd.next();
  // als KI niet werkt -> if (strcmp(param, "cycle") == 0) params.cycletime = atol(value) ; //debug = false;
  //
  //KI om te testen, berekend ki a.d.h.v. eenheid per seconde (moet getest worden)
  if (strcmp(param, "cycle") == 0) {
    long newCycleTime = atol(value);
    float ratio = ((float)newCycleTime) / ((float)params.cycletime);

    params.Ki *= ratio;
    params.Kd /= ratio;

    params.cycletime = newCycleTime;
  } 
  
  else if (strcmp(param, "ki") == 0) {
    float cycleTimeInSec = ((float)params.cycletime) / 1000000;
    params.Ki = atof(value) * cycleTimeInSec;
  }
  //einde ki berekenen
  //kd
   else if (strcmp(param, "kd") == 0)
  {
    float cycleTimeInSec = ((float) params.cycletime) / 1000000;
    params.Kd = atof(value) / cycleTimeInSec;
  }
  
  //
  else if (strcmp(param, "power") == 0)
    params.power = atol(value);
  else if (strcmp(param, "kp") == 0) params.Kp = atof(value);      //Kp is een float
  else if (strcmp(param, "diff") == 0) params.diff = atof(value);  //diff is een float
  EEPROM_writeAnything(0, params);
}
//
//
//
void onDebug()  // debug, hier komen de belangrijke waarden om te debuggen
{
  SerialPort.print("cycle time: ");
  SerialPort.println(params.cycletime);

  SerialPort.print("calculation time: ");
  SerialPort.println(calculationTime);
  calculationTime = 0;

  //
  SerialPort.print("black: ");
  for (int i = 0; i < 7; i++) {
    SerialPort.print(params.black[i]);
    SerialPort.print(" ");
  }
  SerialPort.println(" ");
  //
  SerialPort.print("white: ");
  for (int i = 0; i < 7; i++) {
    SerialPort.print(params.white[i]);
    SerialPort.print(" ");
  }

  SerialPort.println(" ");
  //
  Serial.print("position: ");
  Serial.println(position);
  //
  Serial.print("run: ");
  Serial.println(run);

  SerialPort.print("Kp: ");
  SerialPort.print(params.Kp);
  SerialPort.println(" ");

  SerialPort.print("power: ");
  SerialPort.print(params.power);
  SerialPort.println(" ");

  SerialPort.print("diff: ");
  SerialPort.print(params.diff);
  SerialPort.println(" ");
  //
  //Geeft Ki terug met eenheid per sconde (moet getest worden)
  float cycleTimeInSec = ((float)params.cycletime) / 1000000;
  float ki = params.Ki / cycleTimeInSec;
  SerialPort.print("Ki: ");
  SerialPort.println(ki);
  //kd
   float kd = params.Kd * cycleTimeInSec;
  SerialPort.print("Kd: ");
  SerialPort.println(kd);
  
  //
}
//
//
//
void onCalibrate()  //calibratie voor de sensor
{
  char* param = sCmd.next();
  digitalWrite(led, HIGH);

  if (strcmp(param, "black") == 0)  //calibratie als "black" wordt geschreven
  {
    SerialPort.print("start calibrating black... ");
    for (int i = 0; i < 8; i++) params.black[i] = analogRead(sensor[i]);
    SerialPort.println("done");
  } else if (strcmp(param, "white") == 0)  //calibratie als "white" wordt geschreven
  {
    SerialPort.print("start calibrating white... ");
    for (int i = 0; i < 8; i++) params.white[i] = analogRead(sensor[i]);
    SerialPort.println("done");
  }

  EEPROM_writeAnything(0, params);
}
//
//
//
void Aanzetten() {
  if (run == false) {
    if (digitalRead(knop) == 1) {
      run = true;
      digitalWrite(led, HIGH);
    }
  }
}
//
//
//
void onRun() {
  char* parameter = sCmd.next();
  char* value = sCmd.next();
  if (strcmp(parameter, "on") == 0) {
    iTerm = 0;
    run = true;
    digitalWrite(led, HIGH);
  } else run = false;
}