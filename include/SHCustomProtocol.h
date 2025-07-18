#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <map>
#include "DisplayHelper.h"

#define DEVICE_NAME "ESP32 Display"

// Waveshare ESP32-S3-Touch-LCD-4.3 specifications
#define PIXEL_WIDTH 800
#define PIXEL_HEIGHT 480
#define PIXEL_PER_MM 4.0f

#define TFT_BL 2 // backlight pin


// Waveshare ESP32-S3-Touch-LCD-4.3 - 800x480, capacitive touch
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    5 /* DE */, 3 /* VSYNC */, 46 /* HSYNC */, 7 /* PCLK */,
    40 /* R0 */, 41 /* R1 */, 42 /* R2 */, 1 /* R3 */, 2 /* R4 */,
    39 /* G0 */, 0 /* G1 */, 45 /* G2 */, 48 /* G3 */, 47 /* G4 */, 21 /* G5 */,
    14 /* B0 */, 38 /* B1 */, 18 /* B2 */, 17 /* B3 */, 10 /* B4 */,
    0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    0 /* vsync_polarity */, 4 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 4 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */);


Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    PIXEL_WIDTH /* width */, PIXEL_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);


std::map<String, String> prevData;
std::map<String, int32_t> prevColor;

class SHCustomProtocol {
private:
	int cellTitleHeight = 0;
	bool hasReceivedData = false;
	
	// ตัวแปรสำหรับเก็บข้อมูลจาก SimHub
	int speed = 0;
	int rpm = 0;
	String gear = "N";
	
	// ตัวแปรสำหรับตรวจสอบว่ามีการอัพเดทข้อมูลใหม่หรือไม่
	bool dataUpdated = false;
	
public:
	void setup() {
		gfx->begin();
	    gfx->fillScreen(BLACK);



	#ifdef TFT_BL
		pinMode(TFT_BL, OUTPUT);
		digitalWrite(TFT_BL, HIGH);
	#endif
		gfx->setTextColor(RGB565(0xff, 0x66, 0x00));
		gfx->print("Dashboard Display booting...");

		gfx->setCursor(0, 0);
		gfx->setTextColor(WHITE);
		gfx->setTextSize(1);

        delay(2000);
        
        // แสดงข้อความเริ่มต้น
        gfx->setTextSize(3);
        gfx->setTextColor(WHITE);
        gfx->setCursor(50, 50);
        gfx->print("Waiting for data...");
	}

	// Called when new data is coming from computer
	void read() {
		// ตรวจสอบว่ามีข้อมูลส่งมาจาก SimHub หรือไม่
		if (Serial.available() > 0) {
			// อ่านข้อมูลที่เข้ามาทีละบรรทัด (จนกว่าจะเจอเครื่องหมาย '\n')
			String incomingString = Serial.readStringUntil('\n');

			// แยกข้อมูล (Parse) จาก String ที่ได้รับ
			// รูปแบบที่คาดหวังจาก SimHub คือ "S<speed>R<rpm>G<gear>"
			// เช่น "S120R8500G4"

			// ค้นหาตำแหน่งของตัวอักษรนำหน้าแต่ละค่า
			int sPos = incomingString.indexOf('S');
			int rPos = incomingString.indexOf('R');
			int gPos = incomingString.indexOf('G');

			// ตัด String เพื่อเอาเฉพาะค่าของแต่ละตัวแปร
			if (sPos != -1 && rPos != -1 && gPos != -1) {
				String speedStr = incomingString.substring(sPos + 1, rPos);
				String rpmStr = incomingString.substring(rPos + 1, gPos);
				
				// สำหรับค่าสุดท้าย (Gear) เราไม่ต้องหาตำแหน่งสิ้นสุด
				String gearStr = incomingString.substring(gPos + 1);

				// แปลง String เป็น int
				speed = speedStr.toInt();
				rpm = rpmStr.toInt();
				gear = gearStr;

				// แสดงผลลัพธ์ที่ได้ใน Serial Monitor เพื่อตรวจสอบความถูกต้อง
				Serial.print("Speed: ");
				Serial.print(speed);
				Serial.print(" km/h, RPM: ");
				Serial.print(rpm);
				Serial.print(", Gear: ");
				Serial.println(gear);
				
				// ตั้งค่าให้อัพเดทจอ LCD
				dataUpdated = true;
				hasReceivedData = true;
			}
		}
    }

    void displayData() {
		// เคลียร์พื้นที่ความเร็ว
		clearTextArea(50, 100, 280, 80, Datum::left_top, gfx);
		
		// แสดงความเร็ว
		gfx->setTextSize(4);
		gfx->setTextColor(CYAN);
		gfx->setCursor(50, 50);
		gfx->print("SPEED");
		
		gfx->setTextSize(6);
		gfx->setTextColor(WHITE);
		gfx->setCursor(50, 100);
		gfx->print(speed);
		gfx->setTextSize(3);
		gfx->print(" km/h");
		
		// เคลียร์พื้นที่ RPM
		clearTextArea(50, 250, 280, 50, Datum::left_top, gfx);
		
		// แสดง RPM
		gfx->setTextSize(4);
		gfx->setTextColor(YELLOW);
		gfx->setCursor(50, 200);
		gfx->print("RPM");
		
		gfx->setTextSize(5);
		gfx->setTextColor(WHITE);
		gfx->setCursor(50, 250);
		gfx->print(rpm);
		
		// เคลียร์พื้นที่เกียร์
		clearTextArea(500, 100, 60, 80, Datum::left_top, gfx);
		
		// แสดงเกียร์
		gfx->setTextSize(4);
		gfx->setTextColor(MAGENTA);
		gfx->setCursor(450, 50);
		gfx->print("GEAR");
		
		gfx->setTextSize(8);
		gfx->setTextColor(GREEN);
		gfx->setCursor(500, 100);
		gfx->print(gear);
		
		// แสดงกรอบรอบข้อมูล
		gfx->drawRect(30, 30, 350, 180, WHITE);  // กรอบความเร็ว
		gfx->drawRect(30, 180, 350, 120, WHITE); // กรอบ RPM
		gfx->drawRect(420, 30, 150, 180, WHITE); // กรอบเกียร์
    }

    void loop() {
		read(); // อ่านข้อมูลจาก Serial
		
		// อัพเดทจอ LCD เมื่อมีข้อมูลใหม่
		if (dataUpdated) {
			displayData();
			dataUpdated = false;
		}
    }

    void idle() {
		if (!hasReceivedData) {
			// แสดงข้อความรอข้อมูลหากยังไม่ได้รับข้อมูล
			static unsigned long lastBlink = 0;
			static bool showText = true;
			
			if (millis() - lastBlink > 1000) {
				gfx->fillScreen(BLACK);
				if (showText) {
					gfx->setTextSize(3);
					gfx->setTextColor(WHITE);
					gfx->setCursor(200, 200);
					gfx->print("Waiting for SimHub...");
				}
				showText = !showText;
				gfx->fillScreen(BLACK);
				lastBlink = millis();
			}
		}
    }

};
#endif