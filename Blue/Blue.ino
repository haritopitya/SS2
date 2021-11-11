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
#define ARM_UP_POS 100
#define ARM_DOUWN_POS 200
#define ARM_MOVE_SPEED 10 //puls/s

// TOFADDRESS
#define TOF_FORWERD_ADDR 0x31
#define TOF_LEFT_ADDR 0x33
#define TOF_RIGHT_ADDR 0x35

ServoTimer2 servo;
VL53L0X forwerd, left, right;

void setup()
{
  Serial.begin(38400);
  servo.attach(SERVO_PIN, 544, 2400);
  pinMode(TOF_FORWERD_XSHUT, OUTPUT);
  pinMode(TOF_LEFT_XSHUT, OUTPUT);
  pinMode(TOF_RIGHT_XSHUT, OUTPUT);

  // ToF
  //電源オフ
  digitalWrite(TOF_FORWERD_XSHUT, LOW);
  digitalWrite(TOF_LEFT_XSHUT, LOW);
  digitalWrite(TOF_RIGHT_XSHUT, LOW);
  delay(100);
  // アドレス変更&計測開始
  // FORWERD
  pinMode(TOF_FORWERD_XSHUT, INPUT);
  delay(50);
  if (!forwerd.init())
    Serial.write("fowerd INIT ERROR!");
  forwerd.setTimeout(500);
  forwerd.startContinuous();
  forwerd.setAddress(TOF_FORWERD_ADDR);
  // LEFT
  pinMode(TOF_LEFT_XSHUT, INPUT);
  delay(50);
  if (!left.init())
    Serial.write("fowerd INIT ERROR!");
  left.setTimeout(500);
  left.startContinuous();
  left.setAddress(TOF_LEFT_ADDR);
  // RIGHT
  pinMode(TOF_RIGHT_XSHUT, INPUT);
  delay(50);
  if (!right.init())
    Serial.write("fowerd INIT ERROR!");
  right.setTimeout(500);
  right.startContinuous();
  right.setAddress(TOF_RIGHT_ADDR);

  // 開始ボタン
  while (digitalRead(BUTTON_PIN))
    ;
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
  back();        // 後ろに十分距離があることを確認しながら右折位置まで後退
  rightTurn();   // 右折
  goToBall();    // ボールを取る位置まで前進
  getBall();     // ボール取得動作
  back();        // 右折位置まで後退
  rightTurn();   // 右折
  goToSouko();   // 倉庫まで前進
  releaseBall(); // ボール収納動作
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
  servo.write(ARM_DOUWN_POS);
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
  servo.write(ARM_DOUWN_POS);
  //ちょっと待って
  delay(1000);
  //アームを上げる
  servo.write(ARM_UP_POS);
  delay(500);
}

void goTo(int stop)
{
  // リアプノプベース制御を使いながら前進
  // 前の距離がstopになったら停止(PIDを使うかは未定)

  int v_d = 200; // 前進速度
  int K = 1;     // パラメータ
  int L = 400;   // 通路の幅
  int d = 80;    // センサー同士の距離
  while (1)
  {
    if (forwerd.readRangeContinuousMillimeters() < stop)
    {
      setMotorPulse(0, 0);
      return;
    }
    uint16_t l, r;
    l = left.readRangeContinuousMillimeters();
    r = right.readRangeContinuousMillimeters();
    if (l + r > 500)
    { // 和が長かったら直進
      setMotorPulse(v_d, v_d);
      break;
    }
    double cos = (double)L / (l + r + d);
    double x_e = 2 * (l - r) * cos;
    int v_1 = cos * x_e;  // リアプノプ制御(TBD)
    int v_2 = -cos * x_e; //(TBD) vの単位はなんだろうか
    setMotorPulse(v_1, v_2);
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