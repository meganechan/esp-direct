#include <Arduino.h>
#include "SHCustomProtocol.h"

// สร้าง instance ของ SHCustomProtocol
SHCustomProtocol* sHCustomProtocol;

void setup() {
  // เริ่มต้นการสื่อสารแบบ Serial ที่ Baud Rate 115200
  Serial.begin(115200);
  
  // สร้าง instance และเริ่มต้น
  sHCustomProtocol = new SHCustomProtocol();
  sHCustomProtocol->setup();
  
  Serial.println("LCD ST7262 initialized with Custom Protocol");
}

void loop() {
  // ตรวจสอบว่ามีข้อมูลส่งมาจาก SimHub หรือไม่
  if (Serial.available() > 0) {
    sHCustomProtocol->read();
  }
  
  // อัพเดทการแสดงผล
  sHCustomProtocol->loop();
  
  // idle processing
  sHCustomProtocol->idle();
}