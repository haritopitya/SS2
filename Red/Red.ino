#include <VL53L0X.h>
#include <ServoTimer2.h>
#include <Wire.h>

#define DEBUG

// 停止までの時間
#define BALL_GET_TIME 1600
#define BALL_RELEASE_TIME 1500

// 旋回時間
#define TURN_TIME 700 // ms

// 後方安全距離
#define BACK_SAFTY_DISTANCE 300 // mm

// 回転位置までの後退時間
#define BACK_TIME 500 // ms

// PDを無視する距離(ToFの和の閾値)
#define IGNORE_LENGTH 440 // mm

// 直進用PIDゲイン
#define KP_NUM 3
#define KP_DEN 10
#define KD_NUM 2
#define KD_DEN 20
#define KS_NUM 4
#define KS_DEN 2
#define ROAD_WIDTH 350

// 各種ピン
#define MOTOR_L_IN1 5
#define MOTOR_L_IN2 6
#define MOTOR_R_IN1 9
#define MOTOR_R_IN2 10

#define SERVO_PIN 2

#define TOF_BACK_XSHUT 11
#define TOF_LEFT_XSHUT 8
#define TOF_RIGHT_XSHUT 7
#define TOF_TB_XSHUT 4

// アームサーボ位置
#define ARM_UP_POS 544
#define ARM_DOWN_POS 1600
#define ARM_RELAESE_POS 1800
#define ARM_MOVE_SPEED 1 // ms/puls

// TOFADDRESS
#define TOF_BACK_ADDR 0x31
#define TOF_LEFT_ADDR 0x33
#define TOF_RIGHT_ADDR 0x35
#define TOF_TB_ADDR 0x37

ServoTimer2 servo;
VL53L0X back_senser, left, right, tb;

// メインルーチン用
void back();
void leftTurn();
void goToBall();
void downArm();
void getBall();
void waitBlue();
void goToSouko();
void releaseBall();

// 内部関数
void setMotorPulse(int left, int right);
void blink(int n);
void PD(int v);
void PDreset();
void PDdebug(int l, int r, int b, int e, int ediff, int w, int f);

// PD制御用グローバル変数
int l, r, b;
int f_b_diff;
int e, ePrev, w;
int f, x;
double eDiff;
unsigned int t, prevTime;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  // servo.attach(SERVO_PIN, 544, 2400);
  pinMode(TOF_BACK_XSHUT, OUTPUT);
  pinMode(TOF_LEFT_XSHUT, OUTPUT);
  pinMode(TOF_RIGHT_XSHUT, OUTPUT);
  pinMode(TOF_TB_XSHUT, OUTPUT);

  // ToF
  //電源オフ
  digitalWrite(TOF_BACK_XSHUT, LOW);
  digitalWrite(TOF_LEFT_XSHUT, LOW);
  digitalWrite(TOF_RIGHT_XSHUT, LOW);
  digitalWrite(TOF_TB_XSHUT, LOW);
  delay(100);
  // アドレス変更&計測開始
  // BACK
  pinMode(TOF_BACK_XSHUT, INPUT);
  delay(50);
  while (!back_senser.init())
  {
    Serial.write("BACK INIT ERROR!");
    blink(3);
  }
  back_senser.setTimeout(500);
  back_senser.startContinuous();
  back_senser.setAddress(TOF_BACK_ADDR);
  // LEFT
  pinMode(TOF_LEFT_XSHUT, INPUT);
  delay(50);
  while (!left.init())
  {
    Serial.print("LEFT INIT ERROR!");
    blink(3);
  }
  left.setTimeout(500);
  left.startContinuous();
  left.setAddress(TOF_LEFT_ADDR);
  // RIGHT
  pinMode(TOF_RIGHT_XSHUT, INPUT);
  delay(50);
  while (!right.init())
  {
    Serial.print("RIGHT INIT ERROR!");
    blink(3);
  }
  right.setTimeout(500);
  right.startContinuous();
  right.setAddress(TOF_RIGHT_ADDR);
  // true back
  pinMode(TOF_TB_XSHUT, INPUT);
  delay(50);
  while (!tb.init())
  {
    Serial.print("TRUE BACK INIT ERROR!");
    blink(3);
  }
  tb.setTimeout(500);
  tb.startContinuous();
  tb.setAddress(TOF_TB_ADDR);
  blink(5);
  servo.attach(SERVO_PIN, 544, 2400);
  servo.write(544);
  delay(1000);
  servo.detach();
  // 開始ボタン
  // while (digitalRead(BUTTON_PIN))
  //   ;
}

void loop()
{
  back();     // 左折位置まで後退
  leftTurn(); // 左折
  downArm();
  goToBall();    // ボールを取る位置まで前進
  getBall();     // ボール取得動作
  waitBlue();    // 後ろを通過するまで待機
  back();        // 左折位置まで後退
  leftTurn();    // 左折
  goToSouko();   // 倉庫まで前進
  releaseBall(); // ボール収納動作
}

void back()
{
  // PDを使いながら後退
  int v = -150;
  l = left.readRangeContinuousMillimeters();
  r = right.readRangeContinuousMillimeters();
  b = back_senser.readRangeContinuousMillimeters();
  PDreset();
  while (b < 250)
    PD(v);
  for (int i = 0; i < 10; i++)
    PD(v);
  blink(1);
  // 転回位置まで後退
  setMotorPulse(v, v);
  delay(BACK_TIME);
  setMotorPulse(0, 0);
}

void leftTurn()
{
  // 90度左旋回
  // 要調節
  blink(3);
  int v = 150;
  setMotorPulse(-v, v);
  delay(TURN_TIME);
  setMotorPulse(0, 0);
}

void goToBall()
{
  int v = 150; // 前進速度

  // 横壁検出まで直進
  setMotorPulse(v, v);
  do
  {
    l = left.readRangeContinuousMillimeters();
    r = right.readRangeContinuousMillimeters();
  } while (l + r > IGNORE_LENGTH);

  // 検出後PDで直進
  unsigned int startTime = millis();
  PDreset();
  while ((millis() - startTime) < BALL_GET_TIME)
    PD(v);
  setMotorPulse(0, 0); // 停止
}

void downArm()
{
  servo.attach(SERVO_PIN, 544, 2400);
  servo.write(ARM_UP_POS);
  delay(100);
  int i = servo.read();
  for (i; i < ARM_DOWN_POS; i++)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  delay(100);
  servo.detach();
}

void getBall()
{
  // アームを持ち上げる
  servo.attach(SERVO_PIN, 544, 2400);
  servo.write(ARM_DOWN_POS);
  delay(100);
  int i = servo.read();
  for (i; i > ARM_UP_POS; i--)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  delay(1000);
  servo.detach();
}

bool isBluePass()
{
  return tb.readRangeContinuousMillimeters() > BACK_SAFTY_DISTANCE;
}

void waitBlue()
{
  while (isBluePass())
    ;
  while (!isBluePass())
    ;
  delay(2000);
}

void goToSouko()
{
  int v = 150; // 前進速度

  // 横壁検出まで直進
  setMotorPulse(v, v);
  do
  {
    l = left.readRangeContinuousMillimeters();
    r = right.readRangeContinuousMillimeters();
  } while (l + r > IGNORE_LENGTH);

  // 交差点に入るまでPD
  PDreset();
  while (l + r < IGNORE_LENGTH)
    PD(v);

  // 交差点を抜け出すまで直進
  setMotorPulse(v, v);
  blink(1);
  do
  {
    l = left.readRangeContinuousMillimeters();
    r = right.readRangeContinuousMillimeters();
  } while (l + r > IGNORE_LENGTH);

  // リリース位置までPDで
  unsigned int startTime = millis();
  PDreset();
  while ((millis() - startTime) < BALL_RELEASE_TIME)
    PD(v);
  setMotorPulse(0, 0); // 停止
}

void releaseBall()
{
  blink(3);
  servo.attach(SERVO_PIN, 544, 2400);
  //アームを下す
  int i = servo.read();
  for (i; i <= ARM_RELAESE_POS; i++)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  //ちょっと待って
  delay(500);
  //アームを上げる
  for (i = servo.read(); i >= ARM_UP_POS; i--)
  {
    servo.write(i);
    delay(ARM_MOVE_SPEED);
  }
  delay(100);
  servo.detach();
}

void blink(int n) // LED点滅
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void PDreset()
{
  ePrev = 0;
  prevTime = millis();
}

void PD(int v)
{
  t = millis();
  l = left.readRangeContinuousMillimeters();
  r = right.readRangeContinuousMillimeters();
  b = back_senser.readRangeContinuousMillimeters();
  e = r - l; // 右が離れれば正
  f = l + r - ROAD_WIDTH;
  f_b_diff = r - b;
  eDiff = (e - ePrev) * 1000.0 / (t - prevTime);
  w = (double)e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
  x = f_b_diff * KS_NUM / KS_DEN;
  // 後ろが抜けたら
  if (b > 250)
    x = e / abs(e) * (l + r - ROAD_WIDTH) * KS_NUM / KS_DEN;
  int leftSpeed, rightSpeed;
  // wが正のとき右が遠い->左(の絶対値)を速くする
  if (v > 0)
  {
    leftSpeed = v + (w + x);
    rightSpeed = v - (w + x);
  }
  else
  {
    leftSpeed = v - (w - x);
    rightSpeed = v + (w - x);
  }
  setMotorPulse(leftSpeed, rightSpeed);
  PDdebug(l, r, b, e, (int)eDiff, w, f);
  ePrev = e;
  prevTime = t;
}

void PDdebug(int l, int r, int b, int e, int eDiff, int w, int f)
{
#ifdef DEBUG
  Serial.print(l);
  Serial.print(",");
  Serial.print(r);
  Serial.print(",");
  Serial.print(b);
  Serial.print(",");
  Serial.print(e);
  Serial.print(",");
  Serial.print(eDiff);
  Serial.print(",");
  Serial.print(f);
  Serial.print(",");
  Serial.println(w);
#endif
}

void setMotorPulse(int left, int right)
{
  if (left > 0)
  {
    left += 30;
    analogWrite(MOTOR_L_IN1, min(left, 255));
    analogWrite(MOTOR_L_IN2, 0);
  }
  else
  {
    left -= 30;
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
