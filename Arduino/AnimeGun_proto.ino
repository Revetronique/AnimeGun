/*
  MotorKnob
  
  A stepper moter follows the turns of a potetiometer
  (or other sensor) on analog input 0.
  
  https://www.arduino.cc/en/Reference/Stepper
  This example code is in the public domain.
  
  How to use stepper methods<instance stepper>
   Step C0 C1  <C0:pin1  C1:pin2>
     1  0  1
     2  1  1
     3  1  0
     4  0  0
     
  ２層励磁による駆動方法<軸を正面に見たときの左回転>
     X   ~X    Y   ~Y
     0    1    0    1
     1    0    0    1
     1    0    1    0
     0    1    1    0
*/

#include <Stepper.h>

// change this to the number of steps on your motor
#define STEPS 360
#define MTRPIN1 4
#define MTRPIN2 7
#define LEDPIN  6
#define SWITCH 13
//#define PERSTEP 30

int perStep = 30;

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS, MTRPIN1, MTRPIN2);  // specify the number of the steps pre round and the input pins

void setup()
{
  // set the speed of the motor to 30 RPMs
  //stepper.setSpeed(30);  // set the rotation speed of the motor<rpm>
  //stepper.setSpeed(60);  //the limit of the speed is 60 rpm <on 9V DC>
  
  pinMode(LEDPIN, OUTPUT);  //ストロボフラッシュ<LED>
  pinMode(12, OUTPUT);    //無くてもよい
  pinMode(SWITCH, INPUT);  //トリガー
  
  digitalWrite(12, HIGH);    //無くてもよい
}

void loop()
{
  static int preSpd = 0;  //前回のrpm<ストロボのfps=周波数>
  static int count = 0;   //ステップ数
  
  // set the speed of the motor
  // FPS:8,12,24,30,60 || 8~60
  int spd = analogRead(0);
  spd = map(spd, 0, 1023, 8, 60);
  stepper.setSpeed(spd);
  
  if(digitalRead(SWITCH) == LOW){
    // minus value makes inverse rotation.
    stepper.step(1);  // the count of the steps of the motor
    
    // make a strobe light
    // whose blinking speed syncronizes the speed of rotation of the motor
    count++;
    if(count >= perStep){    // the motor steps each frame of anime
      digitalWrite(LEDPIN, HIGH);
      count = 0;
    }
    else  digitalWrite(LEDPIN, LOW);
  }
}
