#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <VL53L1X.h>
#include <Preferences.h>


constexpr uint8_t PIN_SDA     = 6;  
constexpr uint8_t PIN_SCL     = 7;   
constexpr uint8_t PIN_XSHUT_A = 10;  
constexpr uint8_t PIN_XSHUT_B = 4;   
constexpr uint8_t PIN_LASER_A = 5;   
constexpr uint8_t PIN_LASER_B = 21;  
constexpr uint8_t PIN_BTN     = 20;  
constexpr uint8_t PIN_VBAT    = 3;   


constexpr uint8_t ADDR_OLED  = 0x3C;
constexpr uint8_t ADDR_IMU   = 0x68;
constexpr uint8_t ADDR_TOF_A = 0x30;  
constexpr uint8_t ADDR_TOF_B = 0x31;


constexpr float    SENSOR_SPAN_MM  = 100.0f;  
constexpr float    TOF_OFFSET_A_MM = 0.0f;    
constexpr float    TOF_OFFSET_B_MM = 0.0f;
constexpr float    LEVEL_TOL_DEG   = 0.5f;    
constexpr float    ALPHA           = 0.98f;   
constexpr int      PITCH_SIGN      = 1;       
constexpr int      ROLL_SIGN       = 1;
constexpr uint16_t LONG_PRESS_MS   = 800;
constexpr uint32_t SLEEP_AFTER_MS  = 60000;
constexpr float    BATT_DIV_RATIO  = 2.0f;    
constexpr float    BATT_EMPTY_V    = 3.30f;   
constexpr float    BATT_FULL_V     = 4.15f;
constexpr uint16_t TOF_MAX_MM      = 4000;


Adafruit_SSD1306 display(128, 64, &Wire, -1);
VL53L1X tofA, tofB;
Preferences prefs;


enum Mode { M_LEVEL, M_DIST_A, M_DIST_B, M_SPAN, M_TILT, MODE_COUNT };
const char *MODE_NAMES[MODE_COUNT] = {"LEVEL", "DIST A", "DIST B", "SPAN", "TILT"};

enum BtnEvent { BTN_NONE, BTN_SHORT, BTN_LONG };

struct Imu { float ax, ay, az, gx, gy, gz; };

struct Readings {
  bool  aOk = false, bOk = false, spanOk = false, level = false;
  float aMm = NAN, bMm = NAN, spanMm = NAN, horizMm = NAN, heightMm = NAN;
  float pitch = 0, roll = 0;
};


struct Median5 {
  uint16_t v[5] = {0};
  uint8_t n = 0, idx = 0, bad = 0;
  void push(uint16_t x) { v[idx] = x; idx = (idx + 1) % 5; if (n < 5) n++; bad = 0; }
  void miss() { if (++bad >= 4) { n = 0; idx = 0; bad = 4; } }
  bool valid() const { return n >= 3; }
  float get() const {
    uint16_t t[5];
    for (uint8_t i = 0; i < n; i++) t[i] = v[i];
    for (uint8_t i = 1; i < n; i++) {           
      uint16_t k = t[i];
      int j = i - 1;
      while (j >= 0 && t[j] > k) { t[j + 1] = t[j]; j--; }
      t[j + 1] = k;
    }
    return t[n / 2];
  }
};


Mode mode = M_LEVEL;
bool holdOn = false;
bool idleMode = false;
Readings live, held;

float pitch = 0, roll = 0;               
float pitchZero = 0, rollZero = 0;       
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;
bool  attInit = false;

Median5 filtA, filtB;

float battV = 0;
int   battPct = 0;

uint32_t lastActivity = 0;
uint32_t toastUntil = 0;
const char *toastText = "";


void showMsg(const char *l1, const char *l2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  display.println(l1);
  display.setCursor(0, 16); display.println(l2);
  display.display();
}

void fatal(const char *msg) {
  digitalWrite(PIN_LASER_A, LOW);
  digitalWrite(PIN_LASER_B, LOW);
  Serial.println(msg);
  showMsg("ERROR", msg);
  while (true) delay(1000);
}

void toast(const char *t) { toastText = t; toastUntil = millis() + 900; }


bool imuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ADDR_IMU);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool imuReadBytes(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(ADDR_IMU);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)ADDR_IMU, (uint8_t)n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

bool imuInit() {
  uint8_t who = 0;
  if (!imuReadBytes(0x75, &who, 1)) return false;
  Serial.printf("IMU WHO_AM_I = 0x%02X (0x71 = MPU9250, 0x73 = MPU9255, 0x70 = MPU6500, 0x68 = MPU6050)\n", who);
  if (who != 0x71) Serial.println("Warning: not a genuine MPU9250 ID. Probably a clone/other chip; accel+gyro should still work.");
  imuWrite(0x6B, 0x01);   
  delay(100);
  imuWrite(0x1A, 0x03);   
  imuWrite(0x1B, 0x00);   
  imuWrite(0x1C, 0x00);   
  imuWrite(0x1D, 0x03);   
  delay(50);
  return true;
}

bool readImu(Imu &o) {
  uint8_t b[14];
  if (!imuReadBytes(0x3B, b, 14)) return false;
  auto s16 = [&](int i) { return (int16_t)((b[i] << 8) | b[i + 1]); };
  o.ax = s16(0) / 16384.0f;
  o.ay = s16(2) / 16384.0f;
  o.az = s16(4) / 16384.0f;

  o.gx = s16(8)  / 131.0f;
  o.gy = s16(10) / 131.0f;
  o.gz = s16(12) / 131.0f;
  return true;
}

void calibrateGyro() {
  float sx = 0, sy = 0, sz = 0;
  int n = 0;
  for (int i = 0; i < 200; i++) {
    Imu m;
    if (readImu(m)) { sx += m.gx; sy += m.gy; sz += m.gz; n++; }
    delay(5);
  }
  if (n < 50) fatal("IMU read fail");
  gyroBiasX = sx / n; gyroBiasY = sy / n; gyroBiasZ = sz / n;
}

void updateAttitude() {
  static uint32_t lastUs = micros();
  Imu m;
  if (!readImu(m)) return;                       

  uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt <= 0 || dt > 0.5f) dt = 0.01f;          

  float gx = m.gx - gyroBiasX;
  float gy = m.gy - gyroBiasY;
  float gz = m.gz - gyroBiasZ;


  if (fabsf(gx) + fabsf(gy) + fabsf(gz) > 15.0f) lastActivity = millis();

  float accPitch = atan2f(-m.ax, sqrtf(m.ay * m.ay + m.az * m.az)) * RAD_TO_DEG * PITCH_SIGN;
  float accRoll  = atan2f(m.ay, m.az) * RAD_TO_DEG * ROLL_SIGN;

  if (!attInit) {
    pitch = accPitch; roll = accRoll; attInit = true;
    return;
  }


  pitch += PITCH_SIGN * gy * dt;
  roll  += ROLL_SIGN  * gx * dt;


  float mag = sqrtf(m.ax * m.ax + m.ay * m.ay + m.az * m.az);
  if (mag > 0.8f && mag < 1.2f) {
    pitch = ALPHA * pitch + (1.0f - ALPHA) * accPitch;
    roll  = ALPHA * roll  + (1.0f - ALPHA) * accRoll;
  }
}


bool configTof(VL53L1X &t, uint8_t newAddr) {
  t.setTimeout(500);
  if (!t.init()) return false;           
  t.setAddress(newAddr);                 
  t.setDistanceMode(VL53L1X::Long);      
  t.setMeasurementTimingBudget(50000);   
  return true;
}

void initTof() {
  pinMode(PIN_XSHUT_A, OUTPUT);
  pinMode(PIN_XSHUT_B, OUTPUT);
  digitalWrite(PIN_XSHUT_A, LOW);        
  digitalWrite(PIN_XSHUT_B, LOW);
  delay(10);

  digitalWrite(PIN_XSHUT_A, HIGH);       
  delay(10);
  if (!configTof(tofA, ADDR_TOF_A)) fatal("ToF A init fail");

  digitalWrite(PIN_XSHUT_B, HIGH);       
  delay(10);
  if (!configTof(tofB, ADDR_TOF_B)) fatal("ToF B init fail");

  tofA.startContinuous(50);
  tofB.startContinuous(50);
}

void pollTof(VL53L1X &t, Median5 &f) {
  if (!t.dataReady()) return;
  t.read(false);                         
  if (t.ranging_data.range_status == VL53L1X::RangeValid && t.ranging_data.range_mm < TOF_MAX_MM)
    f.push(t.ranging_data.range_mm);
  else
    f.miss();
}


void applyLasers(bool allOff = false) {
  bool a = false, b = false;
  if (!allOff) {
    switch (mode) {
      case M_DIST_A:
      case M_TILT:   a = true; break;
      case M_DIST_B: b = true; break;
      case M_SPAN:   a = b = true; break;
      default: break;
    }
  }
  digitalWrite(PIN_LASER_A, a);
  digitalWrite(PIN_LASER_B, b);
}


void updateBattery() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_VBAT);
  float v = (mv / 16.0f) * BATT_DIV_RATIO / 1000.0f;
  battV = (battV == 0) ? v : 0.8f * battV + 0.2f * v;
  float p = (battV - BATT_EMPTY_V) / (BATT_FULL_V - BATT_EMPTY_V) * 100.0f;
  battPct = constrain((int)p, 0, 100);
}


BtnEvent pollButton() {
  static bool lastRaw = HIGH, stable = HIGH, longFired = false;
  static uint32_t lastChange = 0, pressedAt = 0;

  bool raw = digitalRead(PIN_BTN);
  uint32_t now = millis();
  if (raw != lastRaw) { lastRaw = raw; lastChange = now; }

  if (now - lastChange > 30 && raw != stable) {      
    stable = raw;
    if (stable == LOW) { pressedAt = now; longFired = false; }
    else if (!longFired) return BTN_SHORT;           
  }
  if (stable == LOW && !longFired && now - pressedAt >= LONG_PRESS_MS) {
    longFired = true;
    return BTN_LONG;
  }
  return BTN_NONE;
}

void saveZero() {
  prefs.putFloat("pz", pitchZero);
  prefs.putFloat("rz", rollZero);
}

void handleButton(BtnEvent e) {
  if (e == BTN_SHORT) {
    mode = (Mode)((mode + 1) % MODE_COUNT);
    holdOn = false;
    applyLasers();
  } else if (e == BTN_LONG) {
    if (mode == M_LEVEL) {
      pitchZero = pitch;               
      rollZero  = roll;
      saveZero();
      toast("ZEROED");
    } else {
      holdOn = !holdOn;
      if (holdOn) held = live;         
      toast(holdOn ? "HOLD" : "LIVE");
    }
  }
}


Readings computeReadings() {
  Readings r;
  r.pitch = pitch - pitchZero;
  r.roll  = roll  - rollZero;
  r.aOk = filtA.valid();
  r.bOk = filtB.valid();
  if (r.aOk) r.aMm = filtA.get() + TOF_OFFSET_A_MM;
  if (r.bOk) r.bMm = filtB.get() + TOF_OFFSET_B_MM;

  float th = r.pitch * DEG_TO_RAD;
  float c = cosf(th), s = sinf(th);
  if (r.aOk) { r.horizMm = r.aMm * c; r.heightMm = r.aMm * s; }
  r.spanOk = r.aOk && r.bOk;
  if (r.spanOk) r.spanMm = (r.aMm + SENSOR_SPAN_MM + r.bMm) * c;
  r.level = fabsf(r.pitch) <= LEVEL_TOL_DEG && fabsf(r.roll) <= LEVEL_TOL_DEG;
  return r;
}


void fmtM(char *out, size_t n, bool ok, float mm) {
  if (!ok) snprintf(out, n, "---");
  else     snprintf(out, n, "%.2f m", mm / 1000.0f);
}

void printAt(int x, int y, int size, const char *s) {
  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(s);
}

void drawHeader() {
  display.setTextColor(SSD1306_WHITE);
  printAt(0, 0, 1, MODE_NAMES[mode]);
  if (holdOn) printAt(56, 0, 1, "HOLD");
  char b[8];
  snprintf(b, sizeof b, "%d%%", battPct);
  printAt(128 - (int)strlen(b) * 6, 0, 1, b);
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
}

void drawLevel(const Readings &r) {
  char s[16];
  snprintf(s, sizeof s, "P%+.1f", r.pitch); printAt(0, 14, 2, s);
  snprintf(s, sizeof s, "R%+.1f", r.roll);  printAt(0, 34, 2, s);
  printAt(0, 55, 1, r.level ? "LEVEL OK" : "NOT LEVEL");


  const int cx = 108, cy = 36, R = 18;
  display.drawCircle(cx, cy, R, SSD1306_WHITE);
  display.drawFastHLine(cx - 4, cy, 9, SSD1306_WHITE);
  display.drawFastVLine(cx, cy - 4, 9, SSD1306_WHITE);
  int dx = constrain((int)(r.roll  * 3), -(R - 4), R - 4);
  int dy = constrain((int)(r.pitch * 3), -(R - 4), R - 4);
  display.fillCircle(cx + dx, cy - dy, 3, SSD1306_WHITE);
}

void drawDist(const Readings &r, bool isA) {
  char s[16];
  printAt(0, 14, 1, isA ? "Beam A (front)" : "Beam B (rear)");
  fmtM(s, sizeof s, isA ? r.aOk : r.bOk, isA ? r.aMm : r.bMm);
  printAt(0, 26, 3, s);
  snprintf(s, sizeof s, "Angle %+.1f", r.pitch);
  printAt(0, 55, 1, s);
}

void drawSpan(const Readings &r) {
  char v[16], s[24];
  fmtM(v, sizeof v, r.aOk, r.aMm);     snprintf(s, sizeof s, "A %s", v); printAt(0, 13, 2, s);
  fmtM(v, sizeof v, r.bOk, r.bMm);     snprintf(s, sizeof s, "B %s", v); printAt(0, 29, 2, s);
  fmtM(v, sizeof v, r.spanOk, r.spanMm); snprintf(s, sizeof s, "T %s", v); printAt(0, 45, 2, s);
}

void drawTilt(const Readings &r) {
  char v[16], s[24];
  fmtM(v, sizeof v, r.aOk, r.aMm);
  snprintf(s, sizeof s, "Ang %+.1f  d %s", r.pitch, v);
  printAt(0, 14, 1, s);
  if (r.aOk) snprintf(v, sizeof v, "%.2f m", r.horizMm / 1000.0f); else snprintf(v, sizeof v, "---");
  snprintf(s, sizeof s, "H %s", v); printAt(0, 26, 2, s);
  if (r.aOk) snprintf(v, sizeof v, "%+.2f m", r.heightMm / 1000.0f); else snprintf(v, sizeof v, "---");
  snprintf(s, sizeof s, "V %s", v); printAt(0, 44, 2, s);
}

void drawScreen() {
  const Readings &r = holdOn ? held : live;
  display.clearDisplay();
  drawHeader();
  switch (mode) {
    case M_LEVEL:  drawLevel(r);       break;
    case M_DIST_A: drawDist(r, true);  break;
    case M_DIST_B: drawDist(r, false); break;
    case M_SPAN:   drawSpan(r);        break;
    case M_TILT:   drawTilt(r);        break;
    default: break;
  }
  if (millis() < toastUntil) {
    display.fillRect(20, 22, 88, 22, SSD1306_BLACK);
    display.drawRect(20, 22, 88, 22, SSD1306_WHITE);
    printAt(26, 26, 2, toastText);
  }
  display.display();
}


void enterIdle() {
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  applyLasers(true);
  tofA.stopContinuous();
  tofB.stopContinuous();
  idleMode = true;
}

void wakeUp() {
  display.ssd1306_command(SSD1306_DISPLAYON);
  filtA = Median5(); filtB = Median5();
  tofA.startContinuous(50);
  tofB.startContinuous(50);
  idleMode = false;
  lastActivity = millis();
  applyLasers();
}


void setup() {
  Serial.begin(115200);

  pinMode(PIN_LASER_A, OUTPUT); digitalWrite(PIN_LASER_A, LOW);
  pinMode(PIN_LASER_B, OUTPUT); digitalWrite(PIN_LASER_B, LOW);
  pinMode(PIN_BTN, INPUT_PULLUP);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(50);                   

  if (!display.begin(SSD1306_SWITCHCAPVCC, ADDR_OLED)) {
    Serial.println("OLED not found at 0x3C");   
    while (true) delay(1000);
  }

  showMsg("Starting...", "Keep device STILL");
  if (!imuInit()) fatal("IMU not found");
  calibrateGyro();

  showMsg("Starting...", "Distance sensors");
  initTof();

  prefs.begin("level", false);
  pitchZero = prefs.getFloat("pz", 0.0f);
  rollZero  = prefs.getFloat("rz", 0.0f);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_VBAT, ADC_11db);
  updateBattery();

  lastActivity = millis();
  applyLasers();
  Serial.println("Ready.");
}


void loop() {
  static uint32_t tImu = 0, tDisp = 0, tBatt = 0, tLog = 0;
  uint32_t now = millis();


  if (idleMode) {
    if (digitalRead(PIN_BTN) == LOW) {
      while (digitalRead(PIN_BTN) == LOW) delay(10);   
      wakeUp();
    } else {
      delay(30);
    }
    return;
  }


  if (now - tImu >= 10) { tImu = now; updateAttitude(); }


  pollTof(tofA, filtA);
  pollTof(tofB, filtB);


  if (now - tBatt >= 2000) { tBatt = now; updateBattery(); }


  BtnEvent e = pollButton();
  if (e != BTN_NONE) { lastActivity = now; handleButton(e); }


  live = computeReadings();
  if (now - tDisp >= 100) { tDisp = now; drawScreen(); }


  if (now - tLog >= 500) {
    tLog = now;
    Serial.printf("pitch=%.2f roll=%.2f A=%s B=%s batt=%.2fV\n",
                  live.pitch, live.roll,
                  live.aOk ? String(live.aMm, 0).c_str() : "--",
                  live.bOk ? String(live.bMm, 0).c_str() : "--", battV);
  }


  if (now - lastActivity > SLEEP_AFTER_MS) enterIdle();

  delay(1);  
}
