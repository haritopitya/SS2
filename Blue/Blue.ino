#include <ServoTimer2.h>

// 停止距離
#define BALL_GET_TIME 3000
#define BALL_RELEASE_TIME 1500

// アームを下した後の移動時間
#define GET_BALL_TIME 1000 // ms

// 旋回時間
#define TURN_TIME 550 // ms

// 回転位置までの後退時間
#define BACK_TIME 1300 // ms

// 後方安全距離
#define BACK_SAFTY_DISTANCE 300 //PSDの値

// PDを無視する距離(ToFの和の閾値)
#define IGNORE_LENGTH 500 //mm

// 直進用PIDゲイン
#define KP_NUM 1
#define KP_DEN 10
#define KD_NUM 1
#define KD_DEN 10
#define KI_NUM 1
#define KI_DEN 20

// 各種ピン
#define MOTOR_L_IN1 5
#define MOTOR_L_IN2 6
#define MOTOR_R_IN1 9
#define MOTOR_R_IN2 10

#define SERVO_PIN 2
#define BUTTON_PIN 15

// アームサーボ位置
#define ARM_UP_POS 544
#define ARM_DOWN_POS 1600
#define ARM_MOVE_SPEED 1 //puls/s

//PSD
#define PSD_LEFT A0
#define PSD_RIGHT A5

// Battery
#define BATTERY_PIN A1
#define BATTERY_REF 200

ServoTimer2 servo;

// メインルーチン用
void back();
void rightTurn();
void goToBall();
void getBall();
void goToSouko();
void releaseBall();

// 内部関数
void setMotorPulse(int left, int right);
void blink(int n);

void setup()
{
  Serial.begin(38400);
  pinMode(LED_BUILTIN, OUTPUT);
  servo.attach(SERVO_PIN, 544, 2400);
  servo.write(ARM_UP_POS);
  // while (1)
  // {
  //   int i = 0;
  //   while (digitalRead(BUTTON_PIN))
  //   {
  //     i++;
  //     if (i > 100)
  //     {
  //       blink(3);
  //       while (digitalRead(BUTTON_PIN))
  //         ;
  //       return;
  //     }
  //   }
  // }
}

void loop()
{
  {
    back();                    //最初の交差点中心まで後退
    rightTurn();               // ボール方向へ
    servo.write(ARM_DOWN_POS); // アームを下す
    goToBall();                // ボールへ
    getBall();                 // アームをあげる
    back();                    // 交差点まで後退
    rightTurn();               // 倉庫に向く
    goToSouko();
    releaseBall();
  }
}

void back()
{
  blink(5);
  // PDを使いながら後退
  // 左右の距離が広がって曲がる位置を認識したら停止
  double e = 0, ePrev = 0, eDiff = 0;
  int v = 150; // 後退速度
  unsigned int prevTime = millis(), t;
  while (1)
  {
    t = millis();
    int l, r, d;
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    if (l + r < 500)
    {
      blink(1);
      setMotorPulse(-v, -v);
      delay(BACK_TIME);
      setMotorPulse(0, 0);
      return;
    }
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse(-(v - w), -(v + w));
    {
      Serial.print(l);
      Serial.print(",");
      Serial.print(r);
      Serial.print(",");
      Serial.print((int)e);
      Serial.print(",");
      Serial.print(eDiff);
      Serial.print(",");
      Serial.print(w);
      Serial.print(",");
      Serial.println();
    }
    ePrev = e;
    prevTime = t;
  }
}
void rightTurn()
{
  // 90度右旋回
  // 要調節
  blink(3);
  int v = 150;
  setMotorPulse(-v, v);
  delay(TURN_TIME);
  setMotorPulse(0, 0);
}

void goToBall()
{
  int l, r, d;
  l = analogRead(PSD_LEFT);
  r = analogRead(PSD_RIGHT);
  int v = 150;
  setMotorPulse(v, v - 10);
  // 横壁検出まで直進
  while (l + r < 500)
  {
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
  }
  double e = 0, ePrev = 0, eDiff = 0;
  v = 150; // 前進速度
  unsigned int prevTime = millis(), t, startTime = millis();
  while ((millis() - startTime) < BALL_GET_TIME)
  {
    t = millis();
    int l, r, d;
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse((v - w), (v + w));
    {
      Serial.print(l);
      Serial.print(",");
      Serial.print(r);
      Serial.print(",");
      Serial.print((int)e);
      Serial.print(",");
      Serial.print(eDiff);
      Serial.print(",");
      Serial.print(w);
      Serial.print(",");
      Serial.println();
    }
    ePrev = e;
    prevTime = t;
  }
  setMotorPulse(0, 0);
}

void getBall()
{
  // アームを持ち上げる
  int i = servo.read();
  for (i; i > ARM_UP_POS; i--)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  delay(10);
}

void goToSouko()
{
  int l, r, d;
  l = analogRead(PSD_LEFT);
  r = analogRead(PSD_RIGHT);
  int v = 150;
  setMotorPulse(v, v - 10);
  // 横壁検出まで直進
  while (l + r < 500)
  {
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
  }
  double e = 0, ePrev = 0, eDiff = 0;
  unsigned int prevTime = millis(), t, startTime = millis();
  while (1)
  {
    t = millis();
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    if (l + r < 500)
    {
      setMotorPulse(v, v - 20);
      blink(1);
      break;
    }
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse((v - w), (v + w));
    {
      Serial.print(l);
      Serial.print(",");
      Serial.print(r);
      Serial.print(",");
      Serial.print((int)e);
      Serial.print(",");
      Serial.print(eDiff);
      Serial.print(",");
      Serial.print(w);
      Serial.print(",");
      Serial.println();
    }
    ePrev = e;
    prevTime = t;
  }

  while (l + r < 500)
  {
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
  }
  e = 0;
  ePrev = 0;
  eDiff = 0;
  prevTime = millis();
  startTime = millis();
  while ((millis() - startTime) < BALL_RELEASE_TIME)
  {
    t = millis();
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse((v + w), (v - w));
    {
      Serial.print(l);
      Serial.print(",");
      Serial.print(r);
      Serial.print(",");
      Serial.print((int)e);
      Serial.print(",");
      Serial.print(eDiff);
      Serial.print(",");
      Serial.print(w);
      Serial.print(",");
      Serial.println();
    }
    ePrev = e;
    prevTime = t;
  }
  setMotorPulse(0, 0);
}

void releaseBall()
{
  blink(3);
  //アームを下す
  int i = servo.read();
  for (i; i <= 1700; i++)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  //ちょっと待って
  delay(500);
  //アームを上げる
  servo.write(ARM_UP_POS);
  delay(20);
}

void blink(int n)
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}

void setMotorPulse(int left, int right)
{
  if (left > 0)
  {
    analogWrite(MOTOR_L_IN1, min(left, 255));
    analogWrite(MOTOR_L_IN2, 0);
  }
  else
  {
    analogWrite(MOTOR_L_IN1, 0);
    analogWrite(MOTOR_L_IN2, min(-left, 255));
  }
  if (right > 0)
  {
    analogWrite(MOTOR_R_IN1, min(right, 255));
    analogWrite(MOTOR_R_IN2, 0);
  }
  else
  {
    analogWrite(MOTOR_R_IN1, 0);
    analogWrite(MOTOR_R_IN2, min(-right, 255));
  }
}