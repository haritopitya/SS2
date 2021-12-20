# 1 "c:\\Users\\tatu4\\Documents\\匠\\大学\\3年\\3Q\\SS2\\Blue\\Blue.ino"
# 2 "c:\\Users\\tatu4\\Documents\\匠\\大学\\3年\\3Q\\SS2\\Blue\\Blue.ino" 2
# 3 "c:\\Users\\tatu4\\Documents\\匠\\大学\\3年\\3Q\\SS2\\Blue\\Blue.ino" 2
# 4 "c:\\Users\\tatu4\\Documents\\匠\\大学\\3年\\3Q\\SS2\\Blue\\Blue.ino" 2



// 停止までの時間




// 旋回時間


// 回転位置までの後退時間


// 後方安全距離


// PDを無視する距離(ToFの和の閾値)


// 直進用PIDゲイン
# 33 "c:\\Users\\tatu4\\Documents\\匠\\大学\\3年\\3Q\\SS2\\Blue\\Blue.ino"
// 各種ピン
# 46 "c:\\Users\\tatu4\\Documents\\匠\\大学\\3年\\3Q\\SS2\\Blue\\Blue.ino"
// アームサーボ位置





// TOFADDRESS




ServoTimer2 servo;
VL53L0X back_senser, left, right;

// メインルーチン用
void back();
void rightTurn();
void goToBall();
void downArm();
void getBall();
void goToSouko();
void releaseBall();

// 内部関数
void setMotorPulse(int left, int right);
void blink(int n);
void PD(int v);
void PDreset();
void PDdebug(int l, int r, int e, int ediff, int w, int f);

// PD制御用グローバル変数
int l, r;
int e, ePrev, w;
int f, x;
double eDiff;
unsigned int t, prevTime;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  // servo.attach(SERVO_PIN, 544, 2400);
  //  pinMode(TOF_BACK_XSHUT, OUTPUT);
  pinMode(7, 0x1);
  pinMode(8, 0x1);

  // ToF
  //電源オフ
  // digitalWrite(TOF_BACK_XSHUT, LOW);
  digitalWrite(7, 0x0);
  digitalWrite(8, 0x0);
  delay(100);
  // アドレス変更&計測開始
  // BACK
  // pinMode(TOF_BACK_XSHUT, INPUT);
  // delay(50);
  // while (!back_senser.init())
  // {
  //     Serial.write("BACK INIT ERROR!");
  //     blink(3);
  // }
  // back_senser.setTimeout(500);
  // back_senser.startContinuous();
  // back_senser.setAddress(TOF_BACK_ADDR);
  // LEFT
  pinMode(7, 0x0);
  delay(50);
  while (!left.init())
  {
    Serial.print("LEFT INIT ERROR!");
    blink(3);
  }

  left.setTimeout(500);
  left.startContinuous();
  left.setAddress(0x33);
  // RIGHT
  pinMode(8, 0x0);
  delay(50);
  while (!right.init())
  {
    Serial.print("RIGHT INIT ERROR!");
    blink(3);
  }
  right.setTimeout(500);
  right.startContinuous();
  right.setAddress(0x35);

  blink(5);
  // 開始ボタン
  // while (digitalRead(BUTTON_PIN))
  //   ;
  // テストブロック
  {
    back();
    rightTurn();
    goToBall();
  }
}

void loop()
{
  // back();                    // 後ろに十分距離があることを確認しながら右折位置まで後退
  // rightTurn();               // 右折
  // servo.write(ARM_DOWN_POS); // アームを下す
  // goToBall();                // ボールを取る位置まで前進
  // getBall();                 // ボール取得動作
  // back();                    // 右折位置まで後退
  // rightTurn();               // 右折
  // goToSouko();               // 倉庫まで前進
  // releaseBall();             // ボール収納動作
}

void back()
{
  // 後ろにスペースがあることを確認

  blink(1);

  // PDを使いながら後退
  int v = -150;
  l = left.readRangeContinuousMillimeters();
  r = right.readRangeContinuousMillimeters();
  PDreset();
  while (r + l < 440 /* mm*/)
    PD(v);

  blink(1);
  // 転回位置まで後退
  setMotorPulse(v, v);
  delay(600 /* ms*/);
  setMotorPulse(0, 0);
}

void rightTurn()
{
  // 90度右旋回
  // 要調節
  blink(3);
  int v = 150;
  setMotorPulse(v, -v);
  delay(850 /* ms*/);
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
  } while (l + r > 440 /* mm*/);

  // 検出後PDで直進
  unsigned int startTime = millis();
  PDreset();
  while ((millis() - startTime) < 1000)
    PD(v);
  setMotorPulse(0, 0); // 停止
  downArm();
  startTime = millis();
  PDreset();
  while ((millis() - startTime) < 1000)
    PD(v);
  setMotorPulse(0, 0); // 停止
}

void downArm()
{
  servo.attach(2, 544, 2400);
  servo.write(544);
  int i = servo.read();
  for (i; i > 1600; i++)
  {
    servo.write(i);
    delay(1 /* ms/puls*/);
  }
  delay(10);
  servo.detach();
}

void getBall()
{
  // アームを持ち上げる
  int i = servo.read();
  for (i; i > 544; i--)
  {
    servo.write(i);
    delay(1 /* ms/puls*/);
  }
  delay(10);
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
  } while (l + r > 440 /* mm*/);

  // 交差点に入るまでPD
  PDreset();
  while (l + r < 440 /* mm*/)
    PD(v);

  // 交差点を抜け出すまで直進
  setMotorPulse(v, v);
  blink(1);
  do
  {
    l = left.readRangeContinuousMillimeters();
    r = right.readRangeContinuousMillimeters();
  } while (l + r > 440 /* mm*/);

  // リリース位置までPDで
  unsigned int startTime = millis();
  PDreset();
  while ((millis() - startTime) < 1500)
    PD(v);
  setMotorPulse(0, 0); // 停止
}

void releaseBall()
{
  blink(3);
  //アームを下す
  int i = servo.read();
  for (i; i <= 1800; i++)
  {
    servo.write(i);
    delay(1 /* ms/puls*/);
  }
  //ちょっと待って
  delay(500);
  //アームを上げる
  servo.write(544);
  delay(20);
}

void blink(int n) // LED点滅
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(13, 0x1);
    delay(50);
    digitalWrite(13, 0x0);
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
  e = r - l; // 右が離れれば正
  eDiff = (e - ePrev) * 1000.0 / (t - prevTime);
  f = l + r - 350;
  w = (double)e * 3 / 10 + eDiff * 2 / 20;
  x = e / ((e)>0?(e):-(e)) * f * 4 / 2;
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
  PDdebug(l, r, e, (int)eDiff, w, f);
  ePrev = e;
  prevTime = t;
}

void PDdebug(int l, int r, int e, int eDiff, int w, int f)
{

  Serial.print(l);
  Serial.print(",");
  Serial.print(r);
  Serial.print(",");
  Serial.print(e);
  Serial.print(",");
  Serial.print(eDiff);
  Serial.print(",");
  Serial.print(f);
  Serial.print(",");
  Serial.println(w);

}

void setMotorPulse(int left, int right)
{
  if (left > 0)
  {
    analogWrite(5, ((left)<(255)?(left):(255)));
    analogWrite(6, 0);
  }
  else
  {
    analogWrite(5, 0);
    analogWrite(6, ((-left)<(255)?(-left):(255)));
  }
  if (right > 0)
  {
    analogWrite(9, ((right)<(255)?(right):(255)));
    analogWrite(10, 0);
  }
  else
  {
    analogWrite(9, 0);
    analogWrite(10, ((-right)<(255)?(-right):(255)));
  }
}
