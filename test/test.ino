
#include <VL53L0X.h>
#include <ServoTimer2.h>
#include <Wire.h>

// 停止距離
#define BALL_GET_DISTANCE 20
#define BALL_RELEASE_DISTANCE 20

// アームを下した後の移動時間
#define GET_BALL_TIME 3000 // ms

// 旋回時間
#define TURN_TIME 5000 // ms

// 後方安全距離
#define BACK_SAFTY_DISTANCE 300 // PSDの値

// PDを無視する距離(ToFの和の閾値)
#define IGNORE_LENGTH 500 // mm

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

#define TOF_FORWERD_XSHUT 11
#define TOF_LEFT_XSHUT 13
#define TOF_RIGHT_XSHUT 12

// アームサーボ位置
#define ARM_UP_POS 544
#define ARM_DOWN_POS 1600
#define ARM_MOVE_SPEED 1 // puls/s

// TOFADDRESS
#define TOF_FORWERD_ADDR 0x31
#define TOF_LEFT_ADDR 0x33
#define TOF_RIGHT_ADDR 0x35

ServoTimer2 servo;
VL53L0X back_senser, left, right;

void blink(int n) // LED点滅
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  pinMode(TOF_LEFT_XSHUT, OUTPUT);
  pinMode(TOF_RIGHT_XSHUT, OUTPUT);
  // ToF
  //電源オフ
  digitalWrite(TOF_RIGHT_XSHUT, LOW);
  digitalWrite(TOF_LEFT_XSHUT, LOW);
  delay(100);
  Serial.print("start init");
  // アドレス変更&計測開始
  // BACK
  // pinMode(TOF_BACK_XSHUT, INPUT);
  // delay(50);
  // if (!back_senser.init())
  // {
  //   while (1)
  //   {
  //     Serial.write("BACK INIT ERROR!");
  //     blink(3);
  //   }
  // }
  // back_senser.setTimeout(500);
  // back_senser.startContinuous();
  // back_senser.setAddress(TOF_BACK_ADDR);
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
}
void releaseBall()
{
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

void loop()
{
  int l, r;
  l = left.readRangeContinuousMillimeters();
  Serial.println(l);
  Serial.print("aaa");
}
