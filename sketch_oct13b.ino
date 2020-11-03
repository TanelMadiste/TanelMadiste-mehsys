//Vajalikud libraryd
#include <AccelStepper.h>
#include <MultiStepper.h>

//Mootorite andmed
#define stepsPerRevolution 2048
#define stepper1Pin1  2
#define stepper1Pin2  3
#define stepper1Pin3  4
#define stepper1Pin4  5
#define stepper2Pin1  6
#define stepper2Pin2  7
#define stepper2Pin3  8
#define stepper2Pin4  9
#define stepper3Pin1  10
#define stepper3Pin2  11
#define stepper3Pin3  12
#define stepper3Pin4  13
#define MotorInterfaceType 8

//Kasutatav kiirus
#define Speed 1000

//Steppermootorite objektid
AccelStepper stepperMotor1 = AccelStepper(MotorInterfaceType, stepper1Pin1, stepper1Pin3, stepper1Pin2, stepper1Pin4);
AccelStepper stepperMotor2 = AccelStepper(MotorInterfaceType, stepper2Pin1, stepper2Pin3, stepper2Pin2, stepper2Pin4);
AccelStepper stepperMotor3 = AccelStepper(MotorInterfaceType, stepper3Pin1, stepper3Pin3, stepper3Pin2, stepper3Pin4);

//Multistepperi klassi lisamine
MultiStepper steppers;


//Globaalsed muutujad
int joyDeadX;
int joyDeadY;

boolean isControlledByJoystick = false;

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];

char cmdFromSM[numChars] = {0};
long valXFromSM = 0;
long valYFromSM = 0;
long valZFromSM = 0;
int valSFromSM = 0;

boolean newData = false;

void setup() {
  Serial.begin(9600);
  //Mootorite algandmed
  stepperMotor1.setMaxSpeed(Speed);
  stepperMotor1.setCurrentPosition(0);
  stepperMotor2.setMaxSpeed(Speed);
  stepperMotor2.setCurrentPosition(0);
  stepperMotor3.setMaxSpeed(Speed);
  stepperMotor3.setCurrentPosition(0);

  //Joysticku deadzone (keskkoht)
  joyDeadX = analogRead(A1);
  joyDeadY = analogRead(A2);

  //Multistepperi klassi stepperite lisamine
  steppers.addStepper(stepperMotor1);
  steppers.addStepper(stepperMotor2);
  steppers.addStepper(stepperMotor3);
  
  Serial.println("Ready");
}

void loop() {
  //Kui on nõutud, et saaks joystickuga juhtida siis kutsub vastavad funktsioonid
  if (isControlledByJoystick == true) {
    int pot = analogRead(A0);
    int joystickX = analogRead(A1);
    int joystickY = analogRead(A2);
    stepperMotor1Turn(pot);
    stepperMotor2Turn(joystickX);
    stepperMotor3Turn(joystickY);
    stepperMotor1.run();
    stepperMotor2.runSpeed();
    stepperMotor3.runSpeed();
  }
  //Liigutab iga Arduino ühe tsükli korral kõiki steppermootoreid ühe sammu võrra
  steppers.run();
  //Loeb Serial Monitori
  recvWithEndMarker();
  //Kui on uus sisend siis töötleb seda
  if (newData == true) {
    strcpy(tempChars, receivedChars); // Kui loop võtab kaua aega hakkab uut infot lugema kuigi vana on töötlemata (bugib ära vahepeal)
    parseData();
    processParsedData();
  }
}

//Liigutab esimest mootorit potentsiomeetri sisendiga.
void stepperMotor1Turn(int var) {
  stepperMotor1.moveTo(var*2);
  stepperMotor1.setSpeed(Speed);
}
void stepperMotor2Turn(int var) {
  //Joysticku asukohast lahutatakse keskkoht ja see määratakse kiiruseks
  int a = var - joyDeadX;
  stepperMotor2.setSpeed(a);
}

void stepperMotor3Turn(int var) {
  //Joysticku asukohast lahutatakse keskkoht ja see määratakse kiiruseks
  int a = var - joyDeadY;
  stepperMotor3.setSpeed(a);
}

//Prindib sisestatud väärtused Serial Monitori (testimiseks)
void printParsedValues() {
  Serial.print("Received Data ");
  Serial.println(receivedChars);
  Serial.print("Command ");
  Serial.println(cmdFromSM);
  Serial.print("Value 1 ");
  Serial.println(valXFromSM);
  Serial.print("Value 2 ");
  Serial.println(valYFromSM);
  Serial.print("Value 3 ");
  Serial.println(valZFromSM); 
  Serial.print("Value 4 ");
  Serial.println(valSFromSM);  
}

//Liigutab steppereid Multistepperi funktsiooniga, et kõik stepperid jõuaksid sihtohta sama aegselt.
void moveSteppers() {
  Serial.println("All steppers arrive at their position at the same time");
  long pos[3];
  pos[0] = valXFromSM;
  pos[1] = valYFromSM;
  pos[2] = valZFromSM;
  steppers.moveTo(pos);
}

/*
 * Loeb Serial Monitori ühe tähe haaval, ehk ei blocki. Arduino funktsioon on blockiv, seega kui kirjutada midagi Serial Monitori 
 * siis nii kaua mootorid ei liigu kui sisestatud info on loetud.
 */
void recvWithEndMarker() {
  static byte ndx = 0; // Index, mida kasutatakse array asukoha määramiseks
  char endMarker = '\n'; // Lõppetab lugemise reavahetusega
  char rc; //Loetud täht/arv
 
  while (Serial.available() > 0 && newData == false) { //Loeb nii kaua kuni on midagi lugeda ja newData on false
    rc = Serial.read(); // Loeb ühe tähe

    if (rc != endMarker) { // Kui loetud täht ei ole endMarker ('\n') kirjutab saadud tähe receivedChars arraysse
      receivedChars[ndx] = rc;
      ndx++; // Index läheb suuremaks
      if (ndx >= numChars) { // Kui sisestatud teksti pikkus on suurem kui numChars (32) siis muudab ainult viimast tähte
        ndx = numChars - 1;
      }
    }  
    else { //Lõpetab stringi, index 0 järgmise korra jaoks ja nüüd on uus sisend ehk newData = true
      receivedChars[ndx] = '\0';
      ndx = 0;
      newData = true;
    }
  }
}

void parseData() {

  // Splitib sisendi vastavateks osadeks
  char * strtokIndx; // Pointer, mida kasutatakse strtoki kasutamisel
  strtokIndx = strtok(tempChars," "); // Loeb kuni tühikuni
  strcpy(cmdFromSM, strtokIndx); // Copyb saadud info cmdFromSM massiivi 
  strtokIndx = strtok(NULL, " "); // Loeb eelmises kohas pidama jäänud kohast edasi kuni tühikuni
  valXFromSM = atol(strtokIndx);  // Muudab saadud info (string kujul) long kujule
  strtokIndx = strtok(NULL, " ");  
  valYFromSM = atol(strtokIndx);
  strtokIndx = strtok(NULL, " ");  
  valZFromSM = atol(strtokIndx);
  strtokIndx = strtok(NULL, " ");
  valSFromSM = atoi(strtokIndx); // Muudab saadud info int kujule
}

void processParsedData() {
  // Kui saadud info võrdub "move"ga kutsub vastava funktsiooni
  if (!(strcmp(cmdFromSM, "move"))) {
    moveSteppers();
  }    
  else if (!(strcmp(cmdFromSM, "reset"))) {
    stepperMotor1.setCurrentPosition(0);
    stepperMotor2.setCurrentPosition(0);
    stepperMotor3.setCurrentPosition(0);
    Serial.println("Current position set to 0 on every motor");
  }
  else if (!(strcmp(cmdFromSM, "joys"))) {
    if (isControlledByJoystick == false) {
      isControlledByJoystick = true;
      Serial.println("Arm can be controlled with joystick");
    }
    else {
      isControlledByJoystick = false;
      Serial.println("Arm can be controlled with commands now");
    }
  }
  else if (!(strcmp(cmdFromSM, "cpos"))) {
    Serial.println("Current position of motors:");
    Serial.print("Motor 1 ");
    Serial.println(stepperMotor1.currentPosition());
    Serial.print("Motor 2 ");
    Serial.println(stepperMotor2.currentPosition());
    Serial.print("Motor 3 ");
    Serial.println(stepperMotor3.currentPosition());
  }
  else if (!(strcmp(cmdFromSM, "help"))) {
    Serial.println("Available commands:");
    Serial.println("move: requires 3 parametres");
    Serial.println("joys: Allows to use joystick and potentiometer as inputs");
    Serial.println("reset: Sets current position to 0 on all motors");
    Serial.println("cpos: Reports current position of all motors");
  }
  else if (!(strcmp(cmdFromSM, "print"))) {
    printParsedValues();
  }
  else if (!(strcmp(cmdFromSM, "clear"))) {
    clearInput();
  }
  else {
    Serial.print(cmdFromSM);
    Serial.println(" is an unknown command.");
    Serial.println("Type 'help' for available commands.");
  }
  //Uus info on töödeldud, seega võib oodata järgmist infot
  newData = false;
  /*
   * Kustutab vana info, vastasel juhul kirjutades reset ja peale seda kirjutada move,
   * loeb arvuti uueks infoks movet, ehk 5ndat kohta ei uuendata
   */
  clearInput();
}

void clearInput() {
  //Tühjendab bufri
  while (Serial.available()) {
     Serial.read();
  }
  // Kustutab arraydes oleva info
  for (int i; i<numChars; i++) {
     receivedChars[i] = 0;
     tempChars[i] = 0;
  }
}
