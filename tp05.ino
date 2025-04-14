#pragma region constantes

#include <AccelStepper.h>
#define MOTOR_INTERFACE_TYPE 4
#define IN_1 8
#define IN_2 9
#define IN_3 10
#define IN_4 11
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

#include <HCSR04.h>
#define TRIGGER_PIN 6
#define ECHO_PIN 7
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);

#include <LCD_I2C.h>
LCD_I2C lcd(0x27, 16, 2);

enum AppState {TOOTOOCLOSE , TOOCLOSE , AUTOMATIC , TOOFAR};
AppState appState = AUTOMATIC;

const int SETUP_DELAY = 1000;
const int REFRESH_RATE = 100;
const int NO_DISTANCE = 0;
const long MAX_ANGLE = 170;
const long MIN_ANGLE = 10;
const long MAX_DISTANCE = 60;
const long MIN_DISTANCE = 30;
const int MAX_SPEED = 500;
const int MAX_ACCEL = 200;
const int TOUR = 2038;
const float MAX_POSITION = 960;
const float MIN_POSITION = 50;
const int TIMER_RATE = 3000;

const int RED_PIN = 3;
const int GREEN_PIN = 4;
const int BLUE_PIN = 5;

const int BUZZER_PIN = 2;
const int BUZZER_TONE = 1000;

unsigned int currentTime = 0;
int distance;
int angle;
int previousState = 1;
bool timerOn = false;
bool timerOverride = true;
bool wasAlarmState;
String message = "";

enum LedState {RED , BLUE , WHITE};
LedState ledState  = RED;

const int red[3] = {255, 0, 0};
const int blue[3] = {0, 0, 255};
const int white[3] = {255, 255, 255};
const int voidLed[3] = {0, 0, 0};

#pragma endregion

#pragma region Modèles
void xTask(unsigned long ct) {
  static unsigned long lastTime = 0;
  unsigned long rate = 500;
  
  if (ct - lastTime < rate) {
    return;
  }
  
  lastTime = ct;
  
  // Faire le code de la tâche ici
  
}
#pragma endregion

#pragma region functions
//revoie l'angle auquel le stepper dois bouger selon la distance
int angleTask(){
  angle = map(distance, 30, 60, 10, 170);

}
//lire et renvoyer la distance perçue par le capteur
int distanceTask() {
  static unsigned long lastTime = 0;
  unsigned long rate = 50;
  static int lastResult = 0;
  
  if (currentTime - lastTime < rate) {
    return lastResult;
  }
  
  lastTime = currentTime;
  
  // Faire le code de la tâche ici

  distance = hc.dist();

  //vérifier si la distance est valide
  if(distance == 0.0) distance = 1;
  
  lastResult = distance;
}
//afficher les informations sur la led
void ledTask(){
  static unsigned long lastTime = 0;
  const int rate = 150;

  if(currentTime - lastTime < rate) return;
  lastTime = currentTime;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Dist : ");

  lcd.setCursor(7, 0);
  lcd.print(distance);

  lcd.setCursor(10, 0);
  lcd.print("cm");

  lcd.setCursor(0, 1);
  lcd.print("Obj  :");

  lcd.setCursor(7, 1);
  lcd.print(message);

  if(appState != AUTOMATIC) return;

  lcd.setCursor(10, 1);
  lcd.print("deg");
}
//indique au stepper motor où pointer et le fais bouger
void automaticStepperTask() {
  float goTo = map(distance, MIN_DISTANCE, MAX_DISTANCE, MIN_POSITION, MAX_POSITION);
  //goTo = constrain(goTo, MIN_POSITION, MAX_POSITION);

  //Serial.println(goTo);
  //Serial.println(myStepper.distanceToGo());

  myStepper.moveTo(goTo);
}

//le stepper retourne à 0 lorsque trop proche
void tooCloseStepperTask() {

  myStepper.moveTo(MIN_POSITION);
}

//le stepper retourne à 2038 lorsque trop loin
void tooFarStepperTask() {

  myStepper.moveTo(MAX_POSITION);
}

//dispatch les états du programme
void stateManager() {
  // Adapter selon votre situation!
  bool tooClose = distance < 30;
  bool tooTooClose = distance < 15;
  bool tooFar = distance > 60;

  switch (appState) {
    
    case TOOTOOCLOSE:
      if(!tooTooClose){
        appState = TOOCLOSE;
      }
      message = "ALARME!!!";

      //variable qui permet de lancer le timer
      wasAlarmState = true;
      alarmTask();
      
      break;
      
    case TOOCLOSE:
      if(!tooClose){
        appState = AUTOMATIC;
      }
      if(tooTooClose){
        appState = TOOTOOCLOSE;
      }
      message = "trop pres";
      tooCloseStepperTask();
      break;

    case AUTOMATIC:
      if(tooClose){
        appState = TOOCLOSE;
      }
      if(tooFar){
        appState = TOOFAR;
      }
      message = angle;
      automaticStepperTask();
      break;

    case TOOFAR:
      if(!tooFar){
        appState = AUTOMATIC;
      }
      message = "trop loin";
      tooFarStepperTask();
      break;
  }
}

void alarmTimer(){
  static unsigned long previousTime = 0;
  const int rate = 3000;

  //pas besoin de lancer le timer si on est deja en alarme
  if(appState == TOOTOOCLOSE){
    previousTime = currentTime;
    return;
  }

  //si on était en alarme
  if(!wasAlarmState){
    previousTime = currentTime;
    return;
  }

  //appelle l'alarme
  alarmTask();

  //après 3 secondes, on reset l'alarme
  if( currentTime - previousTime > rate){
    wasAlarmState = false;

    //reset la del et le buzzer à la fin de l'alarme
    rgbTask(voidLed);
    noTone(BUZZER_PIN);
  }
}

void alarmTask(){
  static unsigned long lastTime = 0;
  const int rate = 250;

  if(!(currentTime - lastTime > rate)){
    return;
  }

  lastTime = currentTime;

  //fais flash la del chaque quart de seconde en changeant sa couleur
  switch (ledState) {
    case RED:
      rgbTask(red);
      ledState = BLUE;
      break;
    case BLUE:
      rgbTask(blue);
      ledState = WHITE;
      break;
    case WHITE:
      rgbTask(white);
      ledState = RED;
      break;
  }

  //appelle la fonction qui fait produire un son au buzzer
  tone(BUZZER_PIN, BUZZER_TONE);
}

void rgbTask(int tableau[3]){
  analogWrite(RED_PIN, tableau[0]);
  analogWrite(GREEN_PIN,  tableau[1]);
  analogWrite(BLUE_PIN, tableau[2]);
}
#pragma endregion

#pragma region setup-loop
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.begin();
  lcd.backlight();

  myStepper.setMaxSpeed(500);  
  myStepper.setAcceleration(100); 

  pinMode(RED_PIN,  OUTPUT);              
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  setupTask();
}
//message au début du programme affiché sur le lcd
void setupTask(){
  lcd.setCursor(0, 0);
  lcd.print("6229147");

  lcd.setCursor(0, 1);
  lcd.print("labo 4B");

  delay(SETUP_DELAY);

  lcd.clear();
}
//messages à la console
void SerialTask(){
  static unsigned long lastTime = 0;

  if(currentTime - lastTime < REFRESH_RATE) return;

  Serial.print("etd:6229147");
  Serial.print(",dist:");
  Serial.print(distance);
  Serial.print(",deg:");


  if(angle < MIN_ANGLE){
    Serial.println(MIN_ANGLE);
    return;
  }
  else if(angle > MAX_ANGLE){
    Serial.println(MAX_ANGLE);
    return;
  }

  Serial.println(angle);
}

void loop() {
  currentTime = millis();
  distanceTask();
  angleTask();

  SerialTask();
  
  stateManager();

  ledTask();

  alarmTimer();

  myStepper.run();

  previousState = appState;
}
#pragma endregion