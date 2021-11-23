#include <VL53L0X.h>
#include <ServoTimer2.h>
#include <Wire.h>

#define DEBUG

// 停止までの時間
#define BALL_GET_TIME 2000ULL
#define BALL_RELEASE_TIME 1500ULL

// 旋回時間
#define TURN_TIME 550ULL // ms

// 回転位置までの後退時間
#define BACK_TIME 1300ULL // ms

// PDを無視する距離(ToFの和の閾値)
#define IGNORE_LENGTH 500 //mm

// 直進用PIDゲイン
#define KP_NUM 1
#define KP_DEN 3
#define KD_NUM 1
#define KD_DEN 5
#define KI_NUM 1
#define KI_DEN 1

// 各種ピン
#define MOTOR_L_IN1 5
#define MOTOR_L_IN2 6
#define MOTOR_R_IN1 9
#define MOTOR_R_IN2 10

#define SERVO_PIN 2
#define BUTTON_PIN 4

#define TOF_BACK_XSHUT 11
#define TOF_LEFT_XSHUT 12
#define TOF_RIGHT_XSHUT 13

#define BATTERY_REF A0

// アームサーボ位置
#define ARM_UP_POS 544
#define ARM_DOWN_POS 1600
#define ARM_MOVE_SPEED 1 //ms/puls

// TOFADDRESS
#define TOF_BACK_ADDR 0x31
#define TOF_LEFT_ADDR 0x33
#define TOF_RIGHT_ADDR 0x35

// 電池基準電圧
#define BATTERY_BASE_VOLTAGE (2.4 / 3.3 * 1024)

ServoTimer2 servo;
VL53L0X back_senser, left, right;

// メインルーチン用
void back();
void leftTurn();
void goToBall();
void getBall();
void waitBlue();
void goToSouko();
void releaseBall();

// 内部関数
void setMotorPulse(int left, int right);
void blink(int n);
void PD(int v);
void PDreset();
void PDdebug(int l, int r, int e, int ediff, int w);

// PD制御用グローバル変数
int l, r;
int e, ePrev, w;
double eDiff;
unsigned int t, prevTime;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  servo.attach(SERVO_PIN, 544, 2400);
  pinMode(TOF_BACK_XSHUT, OUTPUT);
  pinMode(TOF_LEFT_XSHUT, OUTPUT);
  pinMode(TOF_RIGHT_XSHUT, OUTPUT);

  // ToF
  //電源オフ
  digitalWrite(TOF_BACK_XSHUT, LOW);
  digitalWrite(TOF_LEFT_XSHUT, LOW);
  digitalWrite(TOF_RIGHT_XSHUT, LOW);
  delay(100);
  // アドレス変更&計測開始
  // BACK
  pinMode(TOF_BACK_XSHUT, INPUT);
  delay(50);
  if (!back_senser.init())
  {
    while (1)
    {
      Serial.write("BACK INIT ERROR!");
      blink(3);
    }
  }
  back_senser.setTimeout(500);
  back_senser.startContinuous();
  back_senser.setAddress(TOF_BACK_ADDR);
  // LEFT
  pinMode(TOF_LEFT_XSHUT, INPUT);
  delay(50);
  if (!left.init())
  {
    while (1)
    {
      Serial.write("LEFT INIT ERROR!");
      blink(3);
    }
  }
  left.setTimeout(500);
  left.startContinuous();
  left.setAddress(TOF_LEFT_ADDR);
  // RIGHT
  pinMode(TOF_RIGHT_XSHUT, INPUT);
  delay(50);
  if (!right.init())
  {
    while (1)
    {
      Serial.write("RIGHT INIT ERROR!");
      blink(3);
    }
  }
  right.setTimeout(500);
  right.startContinuous();
  right.setAddress(TOF_RIGHT_ADDR);

  blink(5);
  // 開始ボタン
  while (digitalRead(BUTTON_PIN))
    ;
}

void loop()
{
  back();                    // 左折位置まで後退
  leftTurn();                // 左折
  servo.write(ARM_DOWN_POS); // アームを下す
  goToBall();                // ボールを取る位置まで前進
  getBall();                 // ボール取得動作
  waitBlue();                // 後ろを通過するまで待機
  back();                    // 左折位置まで後退
  leftTurn();                // 左折
  goToSouko();               // 倉庫まで前進
  releaseBall();             // ボール収納動作
}

void back()
{
  // PDを使いながら後退
  int v = -150;
  l = left.readRangeContinuousMillimeters();
  r = right.readRangeContinuousMillimeters();
  PDreset();
  while (r + l < IGNORE_LENGTH)
    PD(v);

  blink(1);
  // 転回位置まで後退
  setMotorPulse(v, v);
  delay(BACK_TIME * analogRead(BATTERY_REF) / BATTERY_BASE_VOLTAGE);
  setMotorPulse(0, 0);
}

void leftTurn()
{
  // 90度左旋回
  // 要調節
  blink(3);
  int v = 150;
  setMotorPulse(-v, v);
  delay(TURN_TIME * analogRead(BATTERY_REF) / BATTERY_BASE_VOLTAGE);
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
  while ((millis() - startTime) < BALL_GET_TIME * analogRead(BATTERY_REF) / BATTERY_BASE_VOLTAGE)
    PD(v);
  setMotorPulse(0, 0); // 停止
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

bool isBluePass()
{
  // センサーを読む
  // ???
}

void waitBlue()
{
  while (!isBluePass())
    ;
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
  while ((millis() - startTime) < BALL_RELEASE_TIME * analogRead(BATTERY_REF) / BATTERY_BASE_VOLTAGE)
    PD(v);
  setMotorPulse(0, 0); // 停止
}

void releaseBall()
{
  blink(3);
  //アームを下す
  int i = servo.read();
  for (i; i <= ARM_DOWN_POS; i++)
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

void blink(int n) // LED点滅
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
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
  e = r - l; // 右が離れれば正
  eDiff = (e - ePrev) * 1000.0 / (t - prevTime);
  w = (double)e * KP_NUM / KP_DEN + eDiff * KD_NUM / KD_DEN;
  int leftSpeed, rightSpeed;
  // wが正のとき右が遠い->左(の絶対値)を速くする
  if (v > 0)
  {
    leftSpeed = v + w;
    rightSpeed = v - w;
  }
  else
  {
    leftSpeed = v - w;
    rightSpeed = v + w;
  }
  setMotorPulse(leftSpeed, rightSpeed);
  PDdebug(l, r, e, (int)eDiff, w);
  ePrev = e;
  prevTime = t;
}

void PDdebug(int l, int r, int e, int eDiff, int w)
{
#ifdef DEBUG
  Serial.print(l);
  Serial.print(",");
  Serial.print(r);
  Serial.print(",");
  Serial.print(e);
  Serial.print(",");
  Serial.print(eDiff);
  Serial.print(",");
  Serial.println(w);
#endif
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
