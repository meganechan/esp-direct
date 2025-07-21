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
LV_FONT_DECLARE(segment_font_216);

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
	String lastLap = "00:00.000";
	String nextLap = "00:00.000";
	
	// ตัวแปรสำหรับตรวจสอบว่ามีการอัพเดทข้อมูลใหม่หรือไม่
	bool dataUpdated = false;
	
	// LVGL UI objects
	lv_obj_t *main_screen;
	lv_obj_t *rpm_bar_container;
	lv_obj_t *rpm_bar;
	lv_obj_t *gear_container;
	lv_obj_t *speed_container;
	lv_obj_t *rpm_container;
	lv_obj_t *fuel_container;
	lv_obj_t *lap_container;
	lv_obj_t *gear_label;
	lv_obj_t *gear_value_label;
	lv_obj_t *speed_label;
	lv_obj_t *speed_value_label;
	lv_obj_t *speed_unit_label;
	lv_obj_t *rpm_label;
	lv_obj_t *rpm_value_label;
	lv_obj_t *rpm_unit_label;
	lv_obj_t *fuel_label;
	lv_obj_t *fuel_value_label;
	lv_obj_t *fuel_unit_label;
	lv_obj_t *last_lap_label;
	lv_obj_t *last_lap_value_label;
	lv_obj_t *next_lap_label;
	lv_obj_t *next_lap_value_label;
	lv_obj_t *waiting_label;
	
	// สไตล์
	lv_style_t style_container;
	lv_style_t style_rpm_bar;
	lv_style_t style_gear;
	lv_style_t style_speed;
	lv_style_t style_rpm;
	lv_style_t style_fuel;
	lv_style_t style_lap;
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
		lv_style_set_text_font(&style_gear, &segment_font_216);
		lv_style_set_text_color(&style_gear, lv_color_hex(0x00FF00)); // Green
		
		// Speed style
		lv_style_init(&style_speed);
		lv_style_set_text_font(&style_speed, &segment_font_96);
		lv_style_set_text_color(&style_speed, lv_color_white());
		
		// RPM style
		lv_style_init(&style_rpm);
		lv_style_set_text_font(&style_rpm, &lv_font_montserrat_36);
		lv_style_set_text_color(&style_rpm, lv_color_hex(0xFF8800)); // Orange
		
		// Fuel style
		lv_style_init(&style_fuel);
		lv_style_set_text_font(&style_fuel, &lv_font_montserrat_32);
		lv_style_set_text_color(&style_fuel, lv_color_hex(0x00AAFF)); // Blue
		
		// Lap style
		lv_style_init(&style_lap);
		lv_style_set_text_font(&style_lap, &lv_font_montserrat_24);
		lv_style_set_text_color(&style_lap, lv_color_white());
		
		// Waiting style
		lv_style_init(&style_waiting);
		lv_style_set_text_font(&style_waiting, &lv_font_montserrat_32);
		lv_style_set_text_color(&style_waiting, lv_color_white());
	}
	
	void createUI() {
		// RPM Bar container - บนสุดของหน้าจอ
		rpm_bar_container = lv_obj_create(main_screen);
		lv_obj_add_style(rpm_bar_container, &style_rpm_bar, 0);
		lv_obj_set_size(rpm_bar_container, PIXEL_WIDTH, 70);
		lv_obj_set_pos(rpm_bar_container, 10, 10);
		
		// เอากรอบออกจาก RPM bar container
		lv_obj_set_style_border_width(rpm_bar_container, 0, 0);
		
		// RPM Bar
		rpm_bar = lv_bar_create(rpm_bar_container);
		lv_obj_set_size(rpm_bar, PIXEL_WIDTH - 40, 40); // ขยายให้เต็มพื้นที่มากขึ้น
		lv_obj_align(rpm_bar, LV_ALIGN_CENTER, 0, 0);
		lv_bar_set_range(rpm_bar, 0, 100); // ใช้ percentage
		lv_bar_set_value(rpm_bar, 0, LV_ANIM_OFF);
		
		
		// Gear container - ใหญ่และอยู่กลาง (ปรับตำแหน่งลงมา)
		gear_container = lv_obj_create(main_screen);
		lv_obj_add_style(gear_container, &style_container, 0);
		lv_obj_set_size(gear_container, 350, 220); // ขยายขนาดให้ใหญ่ขึ้น
		lv_obj_set_pos(gear_container, 225, 90); // ปรับตำแหน่ง X ให้กลาง
		
		
		// Gear value - ใหญ่ที่สุด
		gear_value_label = lv_label_create(gear_container);
		lv_label_set_text(gear_value_label, "N");
		lv_obj_add_style(gear_value_label, &style_gear, 0);
		lv_obj_align(gear_value_label, LV_ALIGN_CENTER, 0, 0);
		
		// Speed container - ด้านซ้าย (ปรับตำแหน่งลงมา)
		speed_container = lv_obj_create(main_screen);
		lv_obj_add_style(speed_container, &style_container, 0);
		lv_obj_set_size(speed_container, 180, 120); // ขยายขนาดให้ใหญ่ขึ้น
		lv_obj_set_pos(speed_container, 35, 90); // ปรับตำแหน่ง X
		

		// Speed value
		speed_value_label = lv_label_create(speed_container);
		lv_label_set_text(speed_value_label, "0");
		lv_obj_add_style(speed_value_label, &style_speed, 0);
		lv_obj_align(speed_value_label, LV_ALIGN_CENTER, 0, 0);
		
		// RPM container - ด้านขวา (ปรับตำแหน่งลงมา)
		rpm_container = lv_obj_create(main_screen);
		lv_obj_add_style(rpm_container, &style_container, 0);
		lv_obj_set_size(rpm_container, 180, 120);
		lv_obj_set_pos(rpm_container, 570, 90); // เพิ่ม Y position
		
		// RPM title
		rpm_label = lv_label_create(rpm_container);
		lv_label_set_text(rpm_label, "RPM");
		lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_20, 0);
		lv_obj_set_style_text_color(rpm_label, lv_color_hex(0xFF8800), 0); // Orange
		lv_obj_align(rpm_label, LV_ALIGN_TOP_MID, 0, 10);
		
		// RPM value
		rpm_value_label = lv_label_create(rpm_container);
		lv_label_set_text(rpm_value_label, "0");
		lv_obj_add_style(rpm_value_label, &style_rpm, 0);
		lv_obj_align(rpm_value_label, LV_ALIGN_CENTER, 0, 10);
		
		// RPM unit
		rpm_unit_label = lv_label_create(rpm_container);
		lv_label_set_text_fmt(rpm_unit_label, "/%d", maxRpm);
		lv_obj_set_style_text_font(rpm_unit_label, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_color(rpm_unit_label, lv_color_white(), 0);
		lv_obj_align_to(rpm_unit_label, rpm_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
		
		// Fuel container - ด้านบนขวา (ปรับตำแหน่งลงมา)
		fuel_container = lv_obj_create(main_screen);
		lv_obj_add_style(fuel_container, &style_container, 0);
		lv_obj_set_size(fuel_container, 160, 100);
		lv_obj_set_pos(fuel_container, 590, 220); // เพิ่ม Y position
		
		// Fuel title
		fuel_label = lv_label_create(fuel_container);
		lv_label_set_text(fuel_label, "FUEL");
		lv_obj_set_style_text_font(fuel_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(fuel_label, lv_color_hex(0x00AAFF), 0); // Blue
		lv_obj_align(fuel_label, LV_ALIGN_TOP_MID, 0, 5);
		
		// Fuel value
		fuel_value_label = lv_label_create(fuel_container);
		lv_label_set_text(fuel_value_label, "0.0");
		lv_obj_add_style(fuel_value_label, &style_fuel, 0);
		lv_obj_align(fuel_value_label, LV_ALIGN_CENTER, 0, 5);
		
		// Fuel unit
		fuel_unit_label = lv_label_create(fuel_container);
		lv_label_set_text(fuel_unit_label, "L");
		lv_obj_set_style_text_font(fuel_unit_label, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_color(fuel_unit_label, lv_color_white(), 0);
		lv_obj_align_to(fuel_unit_label, fuel_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
		
		
		// Lap container - ด้านล่าง (ปรับตำแหน่งลงมา)
		lap_container = lv_obj_create(main_screen);
		lv_obj_add_style(lap_container, &style_container, 0);
		lv_obj_set_size(lap_container, 700, 150);
		lv_obj_set_pos(lap_container, 50, 320); // เพิ่ม Y position
		
		// Last lap
		last_lap_label = lv_label_create(lap_container);
		lv_label_set_text(last_lap_label, "LAST LAP");
		lv_obj_set_style_text_font(last_lap_label, &lv_font_montserrat_20, 0);
		lv_obj_set_style_text_color(last_lap_label, lv_color_hex(0x00FFFF), 0); // Cyan
		lv_obj_align(last_lap_label, LV_ALIGN_TOP_LEFT, 20, 10);
		
		last_lap_value_label = lv_label_create(lap_container);
		lv_label_set_text(last_lap_value_label, "00:00.000");
		lv_obj_add_style(last_lap_value_label, &style_lap, 0);
		lv_obj_align_to(last_lap_value_label, last_lap_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
		
		// Next lap
		next_lap_label = lv_label_create(lap_container);
		lv_label_set_text(next_lap_label, "NEXT LAP");
		lv_obj_set_style_text_font(next_lap_label, &lv_font_montserrat_20, 0);
		lv_obj_set_style_text_color(next_lap_label, lv_color_hex(0xFFFF00), 0); // Yellow
		lv_obj_align(next_lap_label, LV_ALIGN_TOP_RIGHT, -20, 10);
		
		next_lap_value_label = lv_label_create(lap_container);
		lv_label_set_text(next_lap_value_label, "00:00.000");
		lv_obj_add_style(next_lap_value_label, &style_lap, 0);
		lv_obj_align_to(next_lap_value_label, next_lap_label, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);
		
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
		lv_obj_add_flag(lap_container, LV_OBJ_FLAG_HIDDEN);
		
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
		lv_obj_clear_flag(lap_container, LV_OBJ_FLAG_HIDDEN);
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
		
		// Parse format: "S:120;R:6500;MR:8000;F:45.5;G:4;L:01:23.456;N:01:20.123;"
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
				// อัพเดท RPM unit label
				lv_label_set_text_fmt(rpm_unit_label, "/%d", maxRpm);
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
			else if (*ptr == 'L' && *(ptr+1) == ':') {
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				lastLap = String(tempStr);
			}
			else if (*ptr == 'N' && *(ptr+1) == ':') {
				ptr += 2;
				int i = 0;
				while (*ptr && *ptr != ';' && i < 15) {
					tempStr[i++] = *ptr++;
				}
				tempStr[i] = '\0';
				nextLap = String(tempStr);
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
		Serial.print(", Last Lap: ");
		Serial.print(lastLap);
		Serial.print(", Next Lap: ");
		Serial.println(nextLap);
		
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
		
		// อัพเดท lap times
		lv_label_set_text(last_lap_value_label, lastLap.c_str());
		lv_label_set_text(next_lap_value_label, nextLap.c_str());
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