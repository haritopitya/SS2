#include <ServoTimer2.h>

// 停止距離
#define BALL_GET_TIME 3000
#define BALL_RELEASE_TIME 3000

// アームを下した後の移動時間
#define GET_BALL_TIME 2000 // ms

// 旋回時間
#define TURN_TIME 500 // ms

// 回転位置までの後退時間
#define BACK_TIME 1550 // ms

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

ServoTimer2 servo;

// メインルーチン用
void back();
void rightTurn();
void goToBall();
void getBall();
void goToSouko();
void releaseBall();

// 内部関数
void goTo(int stop);
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
  {
    back(); //最初の交差点中心まで後退
    rightTurn();
    servo.write(ARM_DOWN_POS);
    goToBall();
    getBall();
    back();
    rightTurn();
  }
}

void loop()
{
  // back(); //最初の交差点中心まで後退
  // rightTurn();
  //servo.write(ARM_DOWN_POS);
  // goToBall();
  // getBall();
  // back();
  // rightTurn();
  // goToSouko();
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
    if (l + r < 400)
    {
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
  int v = 200;
  setMotorPulse(v, -v);
  delay(TURN_TIME);
  setMotorPulse(0, 0);
}
void goToBall()
{
  int l, r, d;
  l = analogRead(PSD_LEFT);
  r = analogRead(PSD_RIGHT);
  int v = 150;
  setMotorPulse(v, v);
  // 横壁検出まで直進
  while (l + r < 400)
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
  for (i; i != ARM_UP_POS; i--)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  delay(50);
}

void goToSouko()
{
  int l, r, d;
  l = analogRead(PSD_LEFT);
  r = analogRead(PSD_RIGHT);
  int v = 200;
  setMotorPulse(v, v);
  // 横壁検出まで直進
  while (l + r < 400)
  {
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
  }
  double e = 0, ePrev = 0, eDiff = 0;
  unsigned int prevTime = millis(), t, startTime = millis();
  while (1)
  {
    t = millis();
    int l, r, d;
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    if (l + r < 300)
    {
      setMotorPulse(v, v);
      break;
    }
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse(-(v + w), -(v - w));
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
  while (l + r < 400)
  {
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
  }
  e = 0, ePrev = 0, eDiff = 0;
  prevTime = millis(), t, startTime = millis();
  while ((millis() - startTime) < BALL_RELEASE_TIME)
  {
    t = millis();
    int l, r, d;
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    if (l + r < 300)
    {
      setMotorPulse(v, v);
      break;
    }
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse(-(v + w), -(v - w));
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
  //アームを下す
  servo.write(ARM_DOWN_POS);
  //ちょっと待って
  delay(1000);
  //アームを上げる
  servo.write(ARM_UP_POS);
  delay(500);
}

void goTo(int stop)
{
  // PDを使いながら前進
  // 前の距離がstopになったら停止(PIDを使うかは未定)

  /*
  定時間ループ
  センサーを読む
  左右の差を計算
  Diffを計算
  PD計算
  モーター駆動
  停止位置ならモーターを停止してreturn
  */
  double e = 0, ePrev = 0, eDiff = 0;
  int v = 200; // 前進速度
  unsigned int prevTime = millis(), t;
  while (1)
  {
    t = millis();
    int l, r, d;
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    if (l + r < 300)
    {
      setMotorPulse(v, v);
      continue;
    }
    e = r - l;
    eDiff = (e - ePrev) * 1000 / (t - prevTime);
    int w = e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
    setMotorPulse(v + w, v - w);
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
    ePrev = e;
    prevTime = t;
  }
}

void blink(int n)
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
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