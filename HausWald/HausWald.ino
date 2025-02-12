#include <Servo.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// SoftwareSerial für MP3-Module
SoftwareSerial mp3FireSerial(10, 11); // RX, TX
SoftwareSerial mp3DoorSerial(4, 5);   // Tür-Sound

DFRobotDFPlayerMini mp3Fire;
DFRobotDFPlayerMini mp3Door;
int mp3ModuleBaudRate = 9600;

// Pins für HC-SR04
const int ultraSonicSensorTriggerPin = 9;
const int ultraSonicSensorPin = 8;

// Servo
Servo myServo;
const int servoPin = 6;
int startPositionServo = 0; // Startposition
int endPositionServo = 50;  // Zielposition
int currentPositionServo = startPositionServo;
unsigned long previousMillisServo = 0;
const long durationServo = 500;                                                     // 3 Sekunden
const long stepDelayServo = durationServo / (endPositionServo - startPositionServo); // Zeit pro Grad

int delayForMp3 = 0;
int firepin = 3;    // Feuerstelle
int audioPin = 0;   // SPK1 mit Spanungsteiler oder DAC_L
int ledDoor = 2;    // https://www.electronicshub.org/wp-content/smush-webp/2021/01/Arduino-Nano-Pinout.jpg.webp
int brightness = 0; // how bright the LED is
long duration;
int distance, audioLevel;
int doorDurationTillOpen, ellapsedTime;
//int doorDelay = 3000;      // the value is a number of milliseconds
unsigned long startMillis; // some global variables available anywhere in the program
unsigned long currentMillis;
bool personDetected = false;
bool personDetectedBefore = false;
bool doorWasOpenOnces = false;
bool doorShouldOpened, doorShouldClosed = false;

void initPins()
{
  pinMode(ultraSonicSensorTriggerPin, OUTPUT);
  pinMode(ultraSonicSensorPin, INPUT);
  pinMode(firepin, OUTPUT);
  pinMode(ledDoor, OUTPUT);
  digitalWrite(ledDoor, HIGH);
}
void successMessage(char *msg)
{
  Serial.println(msg);
}

void errorMessageInitMp3Module()
{
  Serial.println(F("Unable to begin:"));
  Serial.println(F("1.Please recheck the connection!"));
  Serial.println(F("2.Please insert the SD card!"));
};
void initFirePlaceMp3(int volume)
{
  mp3FireSerial.begin(mp3ModuleBaudRate);

  if (!mp3Fire.begin(mp3FireSerial))
  {
    errorMessageInitMp3Module();
    while (true)
    {
      delay(0);
    }
  }
  successMessage("MP3-TF-16P Modul erfolgreich initialisiert!");
  delay(delayForMp3);
  mp3Fire.volume(volume); // Lautstärke einstellen (0-30)
  delay(delayForMp3);
  mp3Fire.loop(1); // Loop für Feuer (Feuergeräusche)
};
void initDoorMp3(int volume)
{
  mp3DoorSerial.begin(mp3ModuleBaudRate);

  if (!mp3Door.begin(mp3DoorSerial))
  {
    errorMessageInitMp3Module();
    while (true)
    {
      delay(0);
    }
  }
  successMessage("MP3-TF-16P Modul für Tür erfolgreich initialisiert!");
  delay(delayForMp3);
  mp3Door.volume(volume); // Lautstärke einstellen (0-30)
}
// the setup routine runs once when you press reset:
void setup()
{
  Serial.begin(115200);
  // MP3-Modul für Feuer
  // initFirePlaceMp3(1); // Lautstärke 20
  initDoorMp3(5); // Lautstärke 20

  initPins();

  // Servo initialisieren
  myServo.attach(servoPin);
  myServo.write(startPositionServo); // Startposition
}

void loop()
{
  flickerLED();
  checkIfPersonIsNear();
  if (doorWasOpenOnces && millis() - currentMillis > 10000)
  {
    doorWasOpenOnces = false;
  }

  if (!doorWasOpenOnces && personDetected && !personDetectedBefore)
  {
    startMillis = millis();
    mp3Door.play(2);
   // myServo.write(endPositionServo); // Startposition (entfernt weil weiter unten opendoor aktiviert wurde)
    //openDoor();
    doorShouldOpened = true;
    personDetectedBefore = true;
    Serial.println("Person erkannt!!!");
  }
  else if (!personDetected && personDetectedBefore && !doorWasOpenOnces)
  {
    currentMillis = millis(); // get the current time
    ellapsedTime = (currentMillis - startMillis) / 1000;

    if (ellapsedTime >= 3)
    {
      Serial.println("\r\n Tür ist geöffnet und kann geschlossenwerden");
      mp3Door.play(2);
      //myServo.write(startPositionServo); // Startposition (entfernt, weil closedoor aktiviert wurde)
       //closeDoor();
      doorWasOpenOnces = true;
      doorShouldClosed = true;
      
      Serial.print("Person weg nach Sekunden: ");
      Serial.println(ellapsedTime);
      personDetectedBefore = false;
    }
    else {
      Serial.print(".");
    }
    personDetected = false;
  }

 openDoor(doorShouldOpened);
 closeDoor(doorShouldClosed);
}

void flickerLED()
{
  audioLevel = analogRead(audioPin); // Audio Level lesen
  int mappedBrightness = map(audioLevel, 0, 500, 50, 255);
  mappedBrightness = constrain(mappedBrightness, 50, 255);

  int flicker = random(-50, 50);
  int brightness = mappedBrightness + flicker;
  brightness = constrain(brightness, 30, 255);
  analogWrite(firepin, brightness); // LED-Flackern steuern
  int randomDelay = random(30, 100);
  delay(randomDelay);
}




void checkIfPersonIsNear()
{
  digitalWrite(ultraSonicSensorTriggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultraSonicSensorTriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultraSonicSensorTriggerPin, LOW);

  duration = pulseIn(ultraSonicSensorPin, HIGH);
  distance = duration * 0.034 / 2;

  if (distance > 0 && distance <= 10)
  {
    personDetected = true;
  }
  else
  {
    personDetected = false;
  }
}

void openDoor(bool doorShouldOpen)
{
  if (doorShouldOpen)
  {
    unsigned long currentMillisServo = millis();
    // Bewegung in Schritten, um sanft in 3 Sekunden auf 90° zu kommen
    if (currentPositionServo == endPositionServo)
    {
      doorShouldOpened = false;
      Serial.println("[openDoor()] - Tür geöffnet...");
    }
    else if (currentMillisServo - previousMillisServo >= stepDelayServo)
    {
      //Serial.println("[openDoor()] - Tür wird geöffnet...");
      previousMillisServo = currentMillisServo;
      currentPositionServo++;
      myServo.write(currentPositionServo);
    }
  }
}

void closeDoor(bool doorShouldClose)
{
  if (doorShouldClose)
  {
    unsigned long currentMillisServo = millis();
    if (currentPositionServo == startPositionServo)
    {
      doorShouldClosed = false;
      Serial.println("[closeDoor()] - Tür geschlossen...");
    }
    else if (currentMillisServo - previousMillisServo >= stepDelayServo && currentPositionServo > startPositionServo)
    {
      // Serial.println("[closeDoor()] - Tür wird geschlossen...");
      previousMillisServo = currentMillisServo;
      currentPositionServo--;
      myServo.write(currentPositionServo);
    }
  }
}