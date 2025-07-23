#ifndef __RACINGDASHBOARD_H__
#define __RACINGDASHBOARD_H__

#include <Arduino.h>
#include "lvgl.h"
#include <Arduino_GFX_Library.h>
#include <map>

#define DEVICE_NAME "ESP32 Display"

// Waveshare ESP32-S3-Touch-LCD-4.3 specifications
#define PIXEL_WIDTH 800
#define PIXEL_HEIGHT 480
#define PIXEL_PER_MM 4.0f

#define TFT_BL 2 // backlight pin


LV_FONT_DECLARE(segment_font_72);
LV_FONT_DECLARE(segment_font_48);
LV_FONT_DECLARE(segment_font_96);
LV_FONT_DECLARE(segment_font_120);
LV_FONT_DECLARE(segment_font_216);
LV_FONT_DECLARE(segment_font_360);

// Arduino GFX as low-level driver for LVGL
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

// LVGL display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[PIXEL_WIDTH * 10];

std::map<String, String> prevData;
std::map<String, int32_t> prevColor;



class RacingDashboard {
private:
	int cellTitleHeight = 0;
	bool hasReceivedData = true;
	
	// ตัวแปรสำหรับเก็บข้อมูลจาก SimHub
	int speed = 0;
	int rpm = 0;
	int maxRpm = 8000;
	float fuel = 0.0;
	String gear = "N";
	String lapTime = "00:00.000"; // เพิ่มตัวแปรสำหรับ lap time
	
	// ตัวแปรสำหรับตรวจสอบว่ามีการอัพเดทข้อมูลใหม่หรือไม่
	bool dataUpdated = false;
	
	// LVGL UI objects
	lv_obj_t *main_screen;
	lv_obj_t *rpm_bar_container;
	lv_obj_t *rpm_bar;
	lv_obj_t *rpm_overlay_lines[10]; // สำหรับเส้นแบ่ง % ทุก 10%
	lv_obj_t *gear_container;
	lv_obj_t *speed_container;
	lv_obj_t *rpm_container;
	lv_obj_t *fuel_container;
	lv_obj_t *laptime_container; // เพิ่ม container สำหรับ lap time
	lv_obj_t *gear_label;
	lv_obj_t *gear_value_label;
	lv_obj_t *speed_label;
	lv_obj_t *speed_value_label;
	lv_obj_t *speed_unit_label;
	lv_obj_t *rpm_label; // เพิ่ม label สำหรับ RPM
	lv_obj_t *rpm_value_label;
	lv_obj_t *fuel_label;
	lv_obj_t *fuel_value_label;
	lv_obj_t *laptime_label; // เพิ่ม label สำหรับ lap time
	lv_obj_t *laptime_value_label;
	lv_obj_t *waiting_label;
	
	// สไตล์
	lv_style_t style_container;
	lv_style_t style_rpm_bar;
	lv_style_t style_gear;
	lv_style_t style_speed;
	lv_style_t style_rpm;
	lv_style_t style_fuel;
	lv_style_t style_laptime; // เพิ่มสไตล์สำหรับ lap time
	lv_style_t style_waiting;
	
public:
	void setup() {
		// Initialize Arduino GFX first
		gfx->begin();

	#ifdef TFT_BL
		pinMode(TFT_BL, OUTPUT);
		digitalWrite(TFT_BL, HIGH);
	#endif
	
		// Initialize LVGL
		lv_init();
		
		// Initialize display buffer
		lv_disp_draw_buf_init(&draw_buf, buf, NULL, PIXEL_WIDTH * 10);
		
		// Initialize display driver
		static lv_disp_drv_t disp_drv;
		lv_disp_drv_init(&disp_drv);
		disp_drv.hor_res = PIXEL_WIDTH;
		disp_drv.ver_res = PIXEL_HEIGHT;
		disp_drv.flush_cb = my_disp_flush;
		disp_drv.draw_buf = &draw_buf;
		lv_disp_drv_register(&disp_drv);

		// สร้าง main screen
		main_screen = lv_obj_create(NULL);
		lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);
		lv_scr_load(main_screen);
		
		// กำหนดสไตล์
		setupStyles();
		
		// สร้าง UI elements
		createUI();
		
		// แสดง dashboard ตั้งแต่เริ่มต้น
		showDashboard();
	}
	
	void setupStyles() {
		// Container style
		lv_style_init(&style_container);
		lv_style_set_bg_color(&style_container, lv_color_black());
		lv_style_set_bg_opa(&style_container, LV_OPA_COVER);
		
		// RPM Bar style
		lv_style_init(&style_rpm_bar);
		lv_style_set_bg_color(&style_rpm_bar, lv_color_black());
		lv_style_set_bg_opa(&style_rpm_bar, LV_OPA_COVER);

		
		// Gear style - ใหญ่และเด่น
		lv_style_init(&style_gear);
		lv_style_set_text_font(&style_gear, &segment_font_360);
		lv_style_set_text_color(&style_gear, lv_color_hex(0x00FF00)); // Green
		
		// Speed style
		lv_style_init(&style_speed);
		lv_style_set_text_font(&style_speed, &segment_font_120);
		lv_style_set_text_color(&style_speed, lv_color_white());
		
		// RPM style
		lv_style_init(&style_rpm);
		lv_style_set_text_font(&style_rpm, &segment_font_120);
		lv_style_set_text_color(&style_rpm, lv_color_hex(0xFF8800)); // Orange
		
		// Fuel style
		lv_style_init(&style_fuel);
		lv_style_set_text_font(&style_fuel, &lv_font_montserrat_32);
		lv_style_set_text_color(&style_fuel, lv_color_hex(0x00AAFF)); // Blue
		
		// Lap Time style
		lv_style_init(&style_laptime);
		lv_style_set_text_font(&style_laptime, &lv_font_montserrat_32);
		lv_style_set_text_color(&style_laptime, lv_color_white());
		
		// Waiting style
		lv_style_init(&style_waiting);
		lv_style_set_text_font(&style_waiting, &lv_font_montserrat_32);
		lv_style_set_text_color(&style_waiting, lv_color_white());
	}
	
	void createUI() {
		// RPM Bar container - บนสุดของหน้าจอ เต็มจอ
		rpm_bar_container = lv_obj_create(main_screen);
		lv_obj_add_style(rpm_bar_container, &style_rpm_bar, 0);
		lv_obj_set_size(rpm_bar_container, PIXEL_WIDTH, 120);
		lv_obj_set_pos(rpm_bar_container, 0, 0);
		
		// เอากรอบออกจาก RPM bar container
		lv_obj_set_style_border_width(rpm_bar_container, 0, 0);
		lv_obj_set_style_pad_all(rpm_bar_container, 0, 0);
		
		// RPM Bar - เต็มจอ ไม่มี overflow
		rpm_bar = lv_bar_create(rpm_bar_container);
		lv_obj_set_size(rpm_bar, PIXEL_WIDTH, 80);
		lv_obj_align(rpm_bar, LV_ALIGN_CENTER, 0, 0);
		lv_bar_set_range(rpm_bar, 0, 100); // ใช้ percentage
		lv_bar_set_value(rpm_bar, 0, LV_ANIM_OFF);
		
		// ปิด rounded corners
		lv_obj_set_style_radius(rpm_bar, 0, 0);
		lv_obj_set_style_radius(rpm_bar, 0, LV_PART_INDICATOR);
		
		// สร้าง overlay lines สำหรับแบ่ง % ทุก 10%
		for(int i = 1; i < 10; i++) { // i = 1-9 คือ 10%-90%
			rpm_overlay_lines[i] = lv_obj_create(rpm_bar_container);
			lv_obj_set_size(rpm_overlay_lines[i], 2, 80); // เส้นบาง ๆ ความสูงเท่า RPM bar
			
			// คำนวณตำแหน่ง X สำหรับแต่ละ %
			int x_pos = (PIXEL_WIDTH * i) / 10; // แบ่ง 10 ส่วน
			lv_obj_set_pos(rpm_overlay_lines[i], x_pos, 20); // y = 20 เพื่อให้อยู่กลาง container
			
			// สไตล์เส้นแบ่ง
			lv_obj_set_style_bg_color(rpm_overlay_lines[i], lv_color_white(), 0);
			lv_obj_set_style_bg_opa(rpm_overlay_lines[i], LV_OPA_70, 0); // โปร่งใสเล็กน้อย
			lv_obj_set_style_border_width(rpm_overlay_lines[i], 0, 0);
			lv_obj_set_style_radius(rpm_overlay_lines[i], 0, 0);
		}
		
		
		// Gear container - ใหญ่และอยู่กลาง (ปรับตำแหน่งลงมา)
		gear_container = lv_obj_create(main_screen);
		lv_obj_add_style(gear_container, &style_container, 0);
		lv_obj_set_size(gear_container, 300, 300); // ขยายขนาดให้ใหญ่ขึ้น
		lv_obj_set_pos(gear_container, 490, 130); // ปรับตำแหน่ง X ให้กลาง
		
		
		// Gear value - ใหญ่ที่สุด
		gear_value_label = lv_label_create(gear_container);
		lv_label_set_text(gear_value_label, "N");
		lv_obj_add_style(gear_value_label, &style_gear, 0);
		lv_obj_align(gear_value_label, LV_ALIGN_CENTER, 0, 0);
		
		// RPM container - ด้านซ้ายบน
		rpm_container = lv_obj_create(main_screen);
		lv_obj_add_style(rpm_container, &style_container, 0);
		lv_obj_set_size(rpm_container, 250, 140); // เพิ่มความกว้างจาก 180 เป็น 250
		lv_obj_set_pos(rpm_container, 15, 130); // ย้ายขึ้นมาด้านบน
		
		// Speed container - ด้านซ้ายกลาง (ระนาบเดียวกับ RPM)
		speed_container = lv_obj_create(main_screen);
		lv_obj_add_style(speed_container, &style_container, 0);
		lv_obj_set_size(speed_container, 200, 140); // ลดความกว้างเล็กน้อย
		lv_obj_set_pos(speed_container, 280, 130); // ย้ายมาระนาบเดียวกับ RPM
		

		// Speed value
		speed_value_label = lv_label_create(speed_container);
		lv_label_set_text(speed_value_label, "0");
		lv_obj_add_style(speed_value_label, &style_speed, 0);
		lv_obj_align(speed_value_label, LV_ALIGN_CENTER, 0, -15);
		
		// Speed label
		speed_label = lv_label_create(speed_container);
		lv_label_set_text(speed_label, "SPEED");
		lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(speed_label, lv_color_white(), 0);
		lv_obj_align(speed_label, LV_ALIGN_BOTTOM_MID, 0, -5);
		
		// RPM value
		rpm_value_label = lv_label_create(rpm_container);
		lv_label_set_text_fmt(rpm_value_label, "%d", 0);
		lv_obj_add_style(rpm_value_label, &style_rpm, 0);
		lv_obj_align(rpm_value_label, LV_ALIGN_CENTER, 0, -15);
		
		// RPM label
		rpm_label = lv_label_create(rpm_container);
		lv_label_set_text(rpm_label, "RPM");
		lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(rpm_label, lv_color_hex(0xFF8800), 0); // Orange เหมือนสี RPM
		lv_obj_align(rpm_label, LV_ALIGN_BOTTOM_MID, 0, -5);
		
		// Fuel container - อยู่แนวเดียวกับ speed แต่ข้างล่าง
		fuel_container = lv_obj_create(main_screen);
		lv_obj_add_style(fuel_container, &style_container, 0);
		lv_obj_set_size(fuel_container, 200, 140);
		lv_obj_set_pos(fuel_container, 280, 290); // อยู่แนวเดียวกับ speed แต่ข้างล่าง
		
		// Fuel title
		fuel_label = lv_label_create(fuel_container);
		lv_label_set_text(fuel_label, "FUEL");
		lv_obj_set_style_text_font(fuel_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(fuel_label, lv_color_hex(0x00AAFF), 0); // Blue
		lv_obj_align(fuel_label, LV_ALIGN_BOTTOM_MID, 0, -5);
		
		// Fuel value
		fuel_value_label = lv_label_create(fuel_container);
		lv_label_set_text(fuel_value_label, "0.0");
		lv_obj_add_style(fuel_value_label, &style_fuel, 0);
		lv_obj_align(fuel_value_label, LV_ALIGN_CENTER, 0, -15);
		
		// Lap Time container - ด้านซ้าย
		laptime_container = lv_obj_create(main_screen);
		lv_obj_add_style(laptime_container, &style_container, 0);
		lv_obj_set_size(laptime_container, 250, 140);
		lv_obj_set_pos(laptime_container, 15, 290); // ด้านซ้าย
		
		// Lap Time title
		laptime_label = lv_label_create(laptime_container);
		lv_label_set_text(laptime_label, "LAP TIME");
		lv_obj_set_style_text_font(laptime_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(laptime_label, lv_color_white(), 0);
		lv_obj_align(laptime_label, LV_ALIGN_BOTTOM_MID, 0, -5);
		
		// Lap Time value
		laptime_value_label = lv_label_create(laptime_container);
		lv_label_set_text(laptime_value_label, "00:00.000");
		lv_obj_add_style(laptime_value_label, &style_laptime, 0);
		lv_obj_align(laptime_value_label, LV_ALIGN_CENTER, 0, -15);
		

		// Waiting message (hidden initially)
		waiting_label = lv_label_create(main_screen);
		lv_label_set_text(waiting_label, "Waiting for SimHub...");
		lv_obj_add_style(waiting_label, &style_waiting, 0);
		lv_obj_align(waiting_label, LV_ALIGN_CENTER, 0, 0);
	}
	
	void showWaitingMessage() {
		// ซ่อน UI elements หลัก
		lv_obj_add_flag(rpm_bar_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(gear_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(speed_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rpm_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(fuel_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(laptime_container, LV_OBJ_FLAG_HIDDEN); // ซ่อน lap time container
		
		// ซ่อน RPM overlay lines
		for(int i = 1; i < 10; i++) {
			lv_obj_add_flag(rpm_overlay_lines[i], LV_OBJ_FLAG_HIDDEN);
		}
		
		// แสดง waiting message
		lv_obj_clear_flag(waiting_label, LV_OBJ_FLAG_HIDDEN);
	}
	
	void showDashboard() {
		// ซ่อน waiting message
		lv_obj_add_flag(waiting_label, LV_OBJ_FLAG_HIDDEN);
		
		// แสดง UI elements หลัก
		lv_obj_clear_flag(rpm_bar_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(gear_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(speed_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rpm_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(fuel_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(laptime_container, LV_OBJ_FLAG_HIDDEN); // แสดง lap time container
		
		// แสดง RPM overlay lines
		for(int i = 1; i < 10; i++) {
			lv_obj_clear_flag(rpm_overlay_lines[i], LV_OBJ_FLAG_HIDDEN);
		}
	}

	// Called when new data is coming from computer
	void read() {
		// ตรวจสอบว่ามีข้อมูลส่งมาจาก SimHub หรือไม่
		if (Serial.available() > 0) {
			static char buffer[128];
			static int bufferIndex = 0;
			
			// อ่านข้อมูลทีละไบต์จนครบบรรทัด
			while (Serial.available() > 0) {
				char c = Serial.read();
				
				if (c == '\n' || c == '\r') {
					if (bufferIndex > 0) {
						buffer[bufferIndex] = '\0'; // null terminate
						
						// Parse ข้อมูลแบบ manual parsing (เร็วกว่า String operations)
						parseData(buffer);
						
						bufferIndex = 0; // reset buffer
					}
					break;
				} else if (bufferIndex < 127) {
					buffer[bufferIndex++] = c;
				}
			}
		}
    }
	
	// Fast parsing function
	void parseData(char* data) {
		char* ptr = data;
		char tempStr[16];
		
		// Parse format: "S:120;R:6500;MR:8000;F:45.5;G:4;L:01:23.456;"
		while (*ptr) {
			if (*ptr == 'S' && *(ptr+1) == ':') {
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				speed = atoi(tempStr);
			}
			else if (*ptr == 'R' && *(ptr+1) == ':') {
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				rpm = atoi(tempStr);
			}
			else if (*ptr == 'M' && *(ptr+1) == 'R' && *(ptr+2) == ':') {
				ptr += 3;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				maxRpm = atoi(tempStr);
			}
			else if (*ptr == 'F' && *(ptr+1) == ':') {
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				fuel = atof(tempStr);
			}
			else if (*ptr == 'G' && *(ptr+1) == ':') {
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				gear = String(tempStr);
			}
			else if (*ptr == 'L' && *(ptr+1) == ':') { // parsing สำหรับ lap time (L:mm:ss.fff)
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				lapTime = String(tempStr);
			}

			else {
				ptr++;
			}
		}
		
		// แสดงผลลัพธ์ที่ได้ใน Serial Monitor เพื่อตรวจสอบความถูกต้อง
		Serial.print("Speed: ");
		Serial.print(speed);
		Serial.print(" km/h, RPM: ");
		Serial.print(rpm);
		Serial.print("/");
		Serial.print(maxRpm);
		Serial.print(", Fuel: ");
		Serial.print(fuel);
		Serial.print("L, Gear: ");
		Serial.print(gear);
		Serial.print(", Lap Time: ");
		Serial.println(lapTime);
		
		// ตั้งค่าให้อัพเดทจอ LCD
		dataUpdated = true;
		hasReceivedData = true;
	}

    void displayData() {
		// Dashboard แสดงตลอดเวลา ไม่ต้องตรวจสอบ hasReceivedData แล้ว
		
		// คำนวณ RPM percentage
		int rpmPercentage = (rpm * 100) / maxRpm;
		if (rpmPercentage > 100) rpmPercentage = 100;
		
		// อัพเดท RPM bar
		lv_bar_set_value(rpm_bar, rpmPercentage, LV_ANIM_OFF);
		
		// เปลี่ยนสี RPM bar ตาม percentage (racing style)
		lv_color_t barColor;
		if (rpmPercentage < 50) {
			// เขียว (0-50%)
			barColor = lv_color_hex(0x00FF00);
		} else if (rpmPercentage < 70) {
			// เหลือง (50-70%)
			barColor = lv_color_hex(0xFFFF00);
		} else if (rpmPercentage < 85) {
			// ส้ม (70-85%)
			barColor = lv_color_hex(0xFF8800);
		} else {
			// แดง (85-100%)
			barColor = lv_color_hex(0xFF0000);
		}
		lv_obj_set_style_bg_color(rpm_bar, barColor, LV_PART_INDICATOR);
		
		// อัพเดท gear value
		lv_label_set_text(gear_value_label, gear.c_str());
		
		// อัพเดท speed value
		lv_label_set_text_fmt(speed_value_label, "%d", speed);
		
		// อัพเดท RPM value
		lv_label_set_text_fmt(rpm_value_label, "%d", rpm);
		
		// อัพเดท fuel value
		lv_label_set_text_fmt(fuel_value_label, "%.1f", fuel);

		// อัพเดท lap time value
		lv_label_set_text(laptime_value_label, lapTime.c_str());
    }

    void update() {
		// อ่านข้อมูลจาก Serial
		read();
		
		// อัพเดทจอ LCD เมื่อมีข้อมูลใหม่
		if (dataUpdated) {
			displayData();
			dataUpdated = false;
		}
		
		// Dashboard จะแสดงตลอดเวลา - ไม่ต้องใช้ blinking animation อีกแล้ว
		// Blinking animation ถูกปิดเพราะเราแสดง dashboard ตลอดเวลา
		
		// Handle LVGL tasks (เรียกครั้งเดียวต่อ update cycle)
		lv_timer_handler();
    }
	
	// Display flush callback function
	static void my_disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p) {
		uint32_t w = (area->x2 - area->x1 + 1);
		uint32_t h = (area->y2 - area->y1 + 1);

		// ใช้ Arduino GFX เป็น low-level driver
		gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p, w, h);
		
		// บอก LVGL ว่า flush เสร็จแล้ว
		lv_disp_flush_ready(disp);
	}

};
#endif