#include <VL53L0X.h>
#include <ServoTimer2.h>

// 停止距離
#define BALL_GET_DISTANCE 20
#define BALL_RELEASE_DISTANCE 20

// アームを下した後の移動時間
#define GET_BALL_TIME 3000 // ms

// 旋回時間
#define TURN_TIME 5000 // ms

// 後方安全距離
#define BACK_SAFTY_DISTANCE 300 //PSDの値

// PDを無視する距離(ToFの和の閾値)
#define IGNORE_LENGTH 500 //mm

// 直進用PIDゲイン
#define KP_NUM 1
#define KP_DEN 20
#define KD_NUM 1
#define KD_DEN 20
#define KI_NUM 1
#define KI_DEN 20

// 各種ピン
#define MOTOR_L_IN1 5
#define MOTOR_L_IN2 6
#define MOTOR_R_IN1 9
#define MOTOR_R_IN2 10

#define SERVO_PIN 2
#define BUTTON_PIN 15

#define TOF_FORWERD_XSHUT 12
#define TOF_LEFT_XSHUT 13
#define TOF_RIGHT_XSHUT 14

// アームサーボ位置
#define ARM_UP_POS 1000
#define ARM_DOWN_POS 2000
#define ARM_MOVE_SPEED 10 //puls/s

// TOFADDRESS
#define TOF_FORWERD_ADDR 0x31
#define TOF_LEFT_ADDR 0x33
#define TOF_RIGHT_ADDR 0x35

//PSD
#define PSD_LEFT A0
#define PSD_RIGHT A5

ServoTimer2 servo;
VL53L0X forwerd, left, right;

void setup()
{
  Serial.begin(38400);
  servo.attach(SERVO_PIN, 544, 2400);
  servo.write(1500);
  
}

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

void loop()
{
  //goTo(100);
  
}

void back()
{
  // 後ろにスペースがあることを確認
  // PDを使いながら後退
  // 左右の距離が広がって曲がる位置を認識したら停止
}
void rightTurn()
{
  // 90度右旋回
  // 要調節
  int v = 100;
  setMotorPulse(v, -v);
  delay(TURN_TIME);
  setMotorPulse(0, 0);
}
void goToBall()
{
  goTo(BALL_GET_DISTANCE);
}
void getBall()
{
  // アームを下す
  servo.write(ARM_DOWN_POS);
  delay(500);
  // 前進&停止
  int v = 150; // 前進速度
  setMotorPulse(v, v);
  delay(GET_BALL_TIME);
  setMotorPulse(0, 0);
  // アームを持ち上げる
  servo.write(ARM_UP_POS);
  delay(500);
}

void goToSouko()
{
  goTo(BALL_RELEASE_DISTANCE);
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
  int loopTime = 50; // ms
  double e = 0, ePrev = 0, eDiff = 0;
  int v = 200; // 前進速度
  unsigned int t;
  while (1)
  {
    t = millis();
    int l, r, d;
    l = analogRead(PSD_LEFT);
    r = analogRead(PSD_RIGHT);
    if (l + r < 200)
    {
      setMotorPulse(v, v);
      continue;
    }
    e = r - l;
    eDiff = (e - ePrev) * 1000.0 / loopTime;
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
    delay(millis() - t);
    ePrev=e;
  }
}

void setMotorPulse(int left, int right)
{
  right+=10;
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
