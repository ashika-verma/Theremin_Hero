/*
 * created by Rui Santos, https://randomnerdtutorials.com
 * 
 * Complete Guide for Ultrasonic Sensor HC-SR04
 *
    Ultrasonic sensor Pins:
        VCC: +5VDC
        Trig : Trigger (INPUT) - Pin11
        Echo: Echo (OUTPUT) - Pin 12
        GND: GND
 */


#include <math.h>
int trigPin = 25;    // Trigger
int echoPin = 26;    // Echo
int buzzpin = 27;
long duration, cm, inches,old_cm,older_cm,avg_cm;
int notes[] = {262,294,330,349,392,440,494,523};
void setup() {
  //Serial Port begin
  Serial.begin(115200); 
  //Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
//  pinMode(buzzpin, OUTPUT);
  ledcSetup(0,2000,8);
  ledcAttachPin(buzzpin,0);
}
 
void loop() {
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  Serial.print(duration);
  Serial.println("~~~~~~~~~~~~");
  Serial.println(log(duration));
  // Convert the time into a distance
  older_cm = old_cm;
  old_cm = cm;
  cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343

  avg_cm = ((cm+old_cm+older_cm)/3);
  inches = (duration/2) / 74;   // Divide by 74 or multiply by 0.0135

  if (abs(old_cm - cm) <= 5){
    int index = avg_cm-3;
    if (index <0 | index > 7){
      index = 0;                                                                                                               
    }
    ledcWriteTone(0, notes[index]);
    ledcWrite(0,100);
  }






  
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  
  delay(100);
}
