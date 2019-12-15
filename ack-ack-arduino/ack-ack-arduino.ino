// SainSmart Instabots Upright Rover rev. 3.0
// Updates at http://www.sainsmart.com

// Updates (spelling, comments) EAS

#include <Wire.h> // I2C/TWI
#include <SPI.h>
#include <Mirf.h> // nRF24L01
#include <nRF24L01.h> // nRF24L01 constants (#define)
#include <MirfHardwareSpiDriver.h> // nRF24L01 SPI
#include <I2Cdev.h> // I2C class
#include <MPU6050.h> // MotionTracker

MPU6050 accelgyro;
MPU6050 initialize;
int16_t ax, ay, az;
int16_t gx, gy, gz;

#define Gyro_offset 0  //The offset of the gyro
#define Gyro_gain 131
#define Angle_offset 0  // The offset of the accelerator
#define RMotor_offset 0  // The offset of the Right Motor
#define LMotor_offset 0  // The offset of the Left Motor
#define pi 3.14159

float Angle_Delta, Angle_Recursive, Angle_Confidence;

float kp, ki, kd;
float Angle_Raw, Angle_Filtered, omega, dt;
float Turn_Speed = 0, Run_Speed = 0;
float LOutput, ROutput, Input, Output;

unsigned long prevTime, lastTime;
float errSum, dErr, error, lastErr;
int timeChange;

long M1_Motion_Count, Sum_Right_Temp = 150, M2_Motion_Count, Sum_Left_Temp = 150, Distance, Distance_Right, Distance_Left, Speed;

// Arduino/Shield Pins/Functions for L298 Motor Driver
int L298_O1_M2M = 25;       // L298_In1 (for Out1 - Bridge-A : Motor2 12-)
int L298_O2_M2P = 24;       // L298_In2 (for Out2 - Bridge-A : Motor2 12+)
int L298_ENA_M2Enable = 5;  // L298_EnA (Enable Bridge-A)
int L298_O3_M1M = 23;       // L298_In3 (for Out3 - Bridge-B : Motor1 12-)
int L298_O4_M1P = 22;       // L298_In4 (for Out4 - Bridge-B : Motor1 12+)
int L298_ENB_M1Enable = 4;  // L298_EnB (Enable Bridge-B)
int M1_PWM_A = 18;          // PWM input-A from Motor1 (D18)
int M1_PWM_B = 19;          // PWM input-B from Motor1 (D19)
int M2_PWM_A = 2;           // PWM input-A from Motor2 (D2)
int M2_PWM_B = 3;           // PWM input-B from Motor2 (D3)


struct Axis  // Data from remote control
{
  uint16_t axis_1;
  uint16_t axis_2;
  uint16_t axis_3;
  uint16_t axis_4;
  uint16_t axis_5;
  uint16_t axis_6;
  uint16_t axis_7;
  uint16_t axis_8;
};
Axis axis_x;

struct Gesture  // Data sent back to remote control
{
  float angle;
  float omega;
  int speed;
  uint16_t P;
  uint16_t I;
  uint16_t D;
  uint16_t null_1;
  uint16_t null_2;
};
Gesture data;

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  TCCR3A = _BV(COM3A1) | _BV(WGM31) | _BV(WGM30); // TIMER_3 @1K Hz, fast pwm
  TCCR3B = _BV(CS31);
  TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00); // TIMER_0 @1K Hz, fast pwm
  TCCR0B = _BV(CS01) | _BV(CS00);

  /* If the robot was turned on with the angle over 45(-45) degrees,the wheels
   will not spin until the robot is in right position. */
  accelgyro.initialize();
  for (int i = 0; i < 200; i++) // Looping 200 times to get the real gesture when starting
  {
    Filter();
  }
  if (abs(Angle_Filtered) < 45)  // Start to work after cleaning data
  {
    omega = Angle_Raw = Angle_Filtered = 0;
    Output = error = errSum = dErr = 0;
    Filter();
    myPID();
  }
  pinMode(L298_O3_M1M, OUTPUT);
  pinMode(L298_O4_M1P, OUTPUT);
  pinMode(L298_ENB_M1Enable, OUTPUT);
  pinMode(M1_PWM_A, INPUT);
  pinMode(L298_O1_M2M, OUTPUT);
  pinMode(L298_O2_M2P, OUTPUT);
  pinMode(L298_ENA_M2Enable, OUTPUT);
  pinMode(M2_PWM_A, INPUT);

  attachInterrupt(digitalPinToInterrupt(M1_PWM_B), Motor1_Motion_Sense, FALLING);
  attachInterrupt(digitalPinToInterrupt(M2_PWM_B), Motor2_Motion_Sense, FALLING);

  // nRF24L01 initialization
  Mirf.cePin = 53;
  Mirf.csnPin = 48;
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setRADDR((byte *)"serv1");
  Mirf.payload = 16;
  Mirf.config();
}

void loop()
{
  while (1)
  {
    Receive();
    if ((micros() - lastTime) > 10000)
    {
      Filter();
      // If angle > 45 or < -45 then stop the robot
      if (abs(Angle_Filtered) < 45)
      {
        myPID();
        PWMControl();
      }
      else
      {
        digitalWrite(L298_O1_M2M, HIGH);
        digitalWrite(L298_O2_M2P, HIGH);
        digitalWrite(L298_O3_M1M, HIGH);
        digitalWrite(L298_O4_M1P, HIGH);
      }
      lastTime = micros();
    }
  }
}

void Receive()
{
  if (!Mirf.isSending() && Mirf.dataReady())
  {
    // Read data from the remote controller
    Mirf.getData((byte *) &axis_x);
    /*Serial.print("axis_1=");
    Serial.print(axis_x.axis_1);
    Serial.print("  axis_2=");
    Serial.print(axis_x.axis_2);
    Serial.print("  axis_3=");
    Serial.print(axis_x.axis_3);
    Serial.print("  axis_4=");
    Serial.print(axis_x.axis_4);
    Serial.print("  axis_5=");
    Serial.print(axis_x.axis_5);
    Serial.print("  axis_6=");
    Serial.print(axis_x.axis_6);
    Serial.print("  axis_7=");
    Serial.print(axis_x.axis_7);
    Serial.print("  axis_8=");
    Serial.println(axis_x.axis_8);*/

    Mirf.setTADDR((byte *)"clie1");
    Mirf.send((byte *) &data);  // Send data back to the controller

    if (axis_x.axis_1 >= 520) // Y axis data from joystick_1
    {
      Turn_Speed = map(axis_x.axis_1, 520, 1023, 0, 120);
    }
    else if (axis_x.axis_1 <= 480)
    {
      Turn_Speed = map(axis_x.axis_1, 480 , 0, 0, -120);
    }
    else
    {
      Turn_Speed = 0;
    }

    if (axis_x.axis_4 >= 520) // X axis data from joystick_2
    {
      Run_Speed = map(axis_x.axis_4, 520, 1023, 0, 100);
    }
    else if (axis_x.axis_4 <= 480)
    {
      Run_Speed = map(axis_x.axis_4, 480, 0, 0, -100);
    }
    else
    {
      Run_Speed = 0;
    }

  }
  else
  {
    axis_x.axis_1 = axis_x.axis_4 = 500;
  }
  data.omega = omega;
  data.angle = Angle_Filtered;
  data.speed = M1_Motion_Count;
  data.P = analogRead(A0);
  data.I = analogRead(A1);
  data.D = analogRead(A2);
}

void Filter()
{
  // Raw data
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Angle_Raw = (atan2(ay, az) * 180 / pi + Angle_offset);
  omega = gx / Gyro_gain + Gyro_offset;
  // Filter data to get the real gesture
  unsigned long now = micros();
  timeChange = now - prevTime;
  prevTime = now;
  dt = timeChange * 0.000001;
  Angle_Delta = (Angle_Raw - Angle_Filtered) * 0.64;
  Angle_Recursive = Angle_Delta * dt + Angle_Recursive;
  Angle_Confidence = Angle_Recursive + (Angle_Raw - Angle_Filtered) * 1.6 + omega;
  Angle_Filtered = Angle_Confidence * dt + Angle_Filtered;
}

void myPID()
{
  kp = 22.000; 
  ki = 0;
  kd = 1.60;
  // Calculating the output values using the gesture values and the PID values.
  error = Angle_Filtered;
  errSum += error;
  dErr = error - lastErr;
  Output = kp * error + ki * errSum + kd * omega;
  lastErr = error;
  noInterrupts();
  if(abs(M2_Motion_Count - Sum_Left_Temp) > 300)
  {
    M2_Motion_Count = Sum_Left_Temp;
  }
  if(abs(M1_Motion_Count - Sum_Right_Temp) > 300)
  {
    M1_Motion_Count = Sum_Right_Temp;
  }
  Speed = (M1_Motion_Count + M2_Motion_Count) / 2;
  Distance += Speed + Run_Speed;
  Distance = constrain(Distance, -8000, 8000);
  Output += Speed * 2.4 + Distance * 0.025;
  Sum_Right_Temp = M1_Motion_Count;
  Sum_Left_Temp = M1_Motion_Count;
  M1_Motion_Count = 0;
  M2_Motion_Count = 0;

  ROutput = Output + Turn_Speed;
  LOutput = Output - Turn_Speed;
  interrupts();
}

void PWMControl()
{
  if (LOutput > 0)
  {
    digitalWrite(L298_O1_M2M, HIGH);
    digitalWrite(L298_O2_M2P, LOW);
  }
  else if (LOutput < 0)
  {
    digitalWrite(L298_O1_M2M, LOW);
    digitalWrite(L298_O2_M2P, HIGH);
  }
  else
  {
    OCR3A = 0;
  }
  if (ROutput > 0)
  {
    digitalWrite(L298_O3_M1M, HIGH);
    digitalWrite(L298_O4_M1P, LOW);
  }
  else if (ROutput < 0)
  {
    digitalWrite(L298_O3_M1M, LOW);
    digitalWrite(L298_O4_M1P, HIGH);
  }
  else
  {
    OCR0B = 0;
  }
  OCR3A = min(1023, (abs(LOutput * 4) + LMotor_offset * 4)); // Timer/Counter3 is a general purpose 16-bit Timer/Counter module
  OCR0B = min(255, (abs(ROutput) + RMotor_offset)); // Timer/Counter0 is a general purpose 8-bit Timer/Counter module
}

/*void Motor1_Motion_Sense()
{
  FlagA = digitalRead(18);
}

void Motor2_Motion_Sense()
{
  FlagB = digitalRead(19);
  if (FlagA == FlagB)
  {
    Counter --;
  }
  else
  {
    Counter ++;
  }
}*/

/**
 * Use the motor quadrature input to determine motor rotation direction 
 * and speed (rpm)
 * 
 * The direction is based on the 'A' output when the 'B' output transitions 
 * from High to Low (the inturrupt condition for this method).
 * 
 * If 'A' is HIGH when 'B' transitions from HIGH to LOW the direction is 
 * 'Forward' (of course that is relative). If 'A' is LOW when 'B' transitions 
 * from HIGH to LOW the direction is 'Reverse' (again relative, but opposite).
 * 
 * This method also records the RPM by calculating the time between interupts 
 * and the direction. If the direction went from +/- to -/+ the RPM is 0. If 
 * the direction is in the same direction then it is the time between pulses 
 * (interupts) 
 */
void Motor1_Motion_Sense()
{
  if (digitalRead(M1_PWM_A))
  {
    M1_Motion_Count ++;
  }
  else
  {
    M1_Motion_Count --;
  }
}

void Motor2_Motion_Sense()
{
  if (!digitalRead(M2_PWM_A))
  {
    M2_Motion_Count ++;
  }
  else
  {
    M2_Motion_Count --;
  }
}
