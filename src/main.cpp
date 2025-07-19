#include <Arduino.h>
#include "RacingDashboard.h"


// สร้าง instance ของ RacingDashboard
RacingDashboard* racingDashboard;


void setup() {
  // เริ่มต้นการสื่อสารแบบ Serial ที่ Baud Rate 115200
  Serial.begin(115200);
  
  // สร้าง instance และเริ่มต้น
  racingDashboard = new RacingDashboard();
  racingDashboard->setup();
  
  Serial.println("Racing Dashboard initialized successfully");
}

void loop() {
  // เรียกฟังก์ชันเดียวที่รวมทุกอย่างไว้
  racingDashboard->update();
}