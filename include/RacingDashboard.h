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
	bool hasReceivedData = false;
	
			// ตัวแปรสำหรับเก็บข้อมูลจาก SimHub
		int speed = 0;
		int rpm = 0;
		String gear = "N";
	
	// ตัวแปรสำหรับตรวจสอบว่ามีการอัพเดทข้อมูลใหม่หรือไม่
	bool dataUpdated = false;
	
	// LVGL UI objects
	lv_obj_t *main_screen;
	lv_obj_t *speed_container;
	lv_obj_t *rpm_container;
	lv_obj_t *gear_container;
	lv_obj_t *speed_label;
	lv_obj_t *speed_value_label;
	lv_obj_t *speed_unit_label;
	lv_obj_t *rpm_label;
	lv_obj_t *rpm_value_label;
	lv_obj_t *gear_label;
	lv_obj_t *gear_value_label;
	lv_obj_t *waiting_label;
	
	// สไตล์
	lv_style_t style_container;
	lv_style_t style_title;
	lv_style_t style_value;
	lv_style_t style_unit;
	lv_style_t style_gear;
	
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
		
		// แสดงข้อความเริ่มต้น
		showWaitingMessage();
	}
	
	void setupStyles() {
		// Container style
		lv_style_init(&style_container);
		lv_style_set_border_color(&style_container, lv_color_white());
		lv_style_set_border_width(&style_container, 2);
		lv_style_set_bg_color(&style_container, lv_color_black());
		lv_style_set_bg_opa(&style_container, LV_OPA_COVER);
		
		// Title style
		lv_style_init(&style_title);
		lv_style_set_text_font(&style_title, &lv_font_montserrat_28);
		lv_style_set_text_color(&style_title, lv_color_white());
		
		// Value style
		lv_style_init(&style_value);
		lv_style_set_text_font(&style_value, &lv_font_montserrat_48);
		lv_style_set_text_color(&style_value, lv_color_white());
		
		// Unit style
		lv_style_init(&style_unit);
		lv_style_set_text_font(&style_unit, &lv_font_montserrat_24);
		lv_style_set_text_color(&style_unit, lv_color_white());
		
		// Gear style
		lv_style_init(&style_gear);
		lv_style_set_text_font(&style_gear, &lv_font_montserrat_48);
		lv_style_set_text_color(&style_gear, lv_color_hex(0x00FF00)); // Green
	}
	
	void createUI() {
		// Speed container
		speed_container = lv_obj_create(main_screen);
		lv_obj_add_style(speed_container, &style_container, 0);
		lv_obj_set_size(speed_container, 350, 180);
		lv_obj_set_pos(speed_container, 30, 30);
		
		// Speed title
		speed_label = lv_label_create(speed_container);
		lv_label_set_text(speed_label, "SPEED");
		lv_obj_add_style(speed_label, &style_title, 0);
		lv_obj_set_style_text_color(speed_label, lv_color_hex(0x00FFFF), 0); // Cyan
		lv_obj_align(speed_label, LV_ALIGN_TOP_LEFT, 20, 10);
		
		// Speed value
		speed_value_label = lv_label_create(speed_container);
		lv_label_set_text(speed_value_label, "0");
		lv_obj_add_style(speed_value_label, &style_value, 0);
		lv_obj_align(speed_value_label, LV_ALIGN_LEFT_MID, 20, 20);
		
		// Speed unit
		speed_unit_label = lv_label_create(speed_container);
		lv_label_set_text(speed_unit_label, "km/h");
		lv_obj_add_style(speed_unit_label, &style_unit, 0);
		lv_obj_align_to(speed_unit_label, speed_value_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
		
		// RPM container
		rpm_container = lv_obj_create(main_screen);
		lv_obj_add_style(rpm_container, &style_container, 0);
		lv_obj_set_size(rpm_container, 350, 120);
		lv_obj_set_pos(rpm_container, 30, 230);
		
		// RPM title
		rpm_label = lv_label_create(rpm_container);
		lv_label_set_text(rpm_label, "RPM");
		lv_obj_add_style(rpm_label, &style_title, 0);
		lv_obj_set_style_text_color(rpm_label, lv_color_hex(0xFFFF00), 0); // Yellow
		lv_obj_align(rpm_label, LV_ALIGN_TOP_LEFT, 20, 10);
		
		// RPM value
		rpm_value_label = lv_label_create(rpm_container);
		lv_label_set_text(rpm_value_label, "0");
		lv_obj_add_style(rpm_value_label, &style_value, 0);
		lv_obj_align(rpm_value_label, LV_ALIGN_LEFT_MID, 20, 10);
		
		// Gear container
		gear_container = lv_obj_create(main_screen);
		lv_obj_add_style(gear_container, &style_container, 0);
		lv_obj_set_size(gear_container, 150, 180);
		lv_obj_set_pos(gear_container, 420, 30);
		
		// Gear title
		gear_label = lv_label_create(gear_container);
		lv_label_set_text(gear_label, "GEAR");
		lv_obj_add_style(gear_label, &style_title, 0);
		lv_obj_set_style_text_color(gear_label, lv_color_hex(0xFF00FF), 0); // Magenta
		lv_obj_align(gear_label, LV_ALIGN_TOP_MID, 0, 10);
		
		// Gear value
		gear_value_label = lv_label_create(gear_container);
		lv_label_set_text(gear_value_label, "N");
		lv_obj_add_style(gear_value_label, &style_gear, 0);
		lv_obj_align(gear_value_label, LV_ALIGN_CENTER, 0, 20);
		
		// Waiting message (hidden initially)
		waiting_label = lv_label_create(main_screen);
		lv_label_set_text(waiting_label, "Waiting for SimHub...");
		lv_obj_set_style_text_font(waiting_label, &lv_font_montserrat_32, 0);
		lv_obj_set_style_text_color(waiting_label, lv_color_white(), 0);
		lv_obj_align(waiting_label, LV_ALIGN_CENTER, 0, 0);
	}
	
	void showWaitingMessage() {
		// ซ่อน UI elements หลัก
		lv_obj_add_flag(speed_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rpm_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(gear_container, LV_OBJ_FLAG_HIDDEN);
		
		// แสดง waiting message
		lv_obj_clear_flag(waiting_label, LV_OBJ_FLAG_HIDDEN);
	}
	
	void showDashboard() {
		// ซ่อน waiting message
		lv_obj_add_flag(waiting_label, LV_OBJ_FLAG_HIDDEN);
		
		// แสดง UI elements หลัก
		lv_obj_clear_flag(speed_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rpm_container, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(gear_container, LV_OBJ_FLAG_HIDDEN);
	}

	// Called when new data is coming from computer
	void read() {
		// ตรวจสอบว่ามีข้อมูลส่งมาจาก SimHub หรือไม่
		if (Serial.available() > 0) {
			// อ่านข้อมูลที่เข้ามาทีละบรรทัด (จนกว่าจะเจอเครื่องหมาย '\n')
			String incomingString = Serial.readStringUntil('\n');

			// แยกข้อมูล (Parse) จาก String ที่ได้รับ
			// รูปแบบที่คาดหวังจาก SimHub คือ "S:0;R:0;G:4;"

			// ค้นหาตำแหน่งของตัวอักษรนำหน้าแต่ละค่า
			int sPos = incomingString.indexOf("S:");
			int rPos = incomingString.indexOf("R:");
			int gPos = incomingString.indexOf("G:");

			// ตัด String เพื่อเอาเฉพาะค่าของแต่ละตัวแปร
			if (sPos != -1 && rPos != -1 && gPos != -1) {
				// หา semicolon หลังจากแต่ละค่า
				int sEnd = incomingString.indexOf(';', sPos);
				int rEnd = incomingString.indexOf(';', rPos);
				int gEnd = incomingString.indexOf(';', gPos);
				
				if (sEnd != -1 && rEnd != -1 && gEnd != -1) {
					String speedStr = incomingString.substring(sPos + 2, sEnd);
					String rpmStr = incomingString.substring(rPos + 2, rEnd);
					String gearStr = incomingString.substring(gPos + 2, gEnd);

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
    }

    void displayData() {
		// แสดง dashboard หากได้รับข้อมูลแล้ว
		if (hasReceivedData) {
			showDashboard();
		}
		
		// อัพเดท speed value
		lv_label_set_text_fmt(speed_value_label, "%d", speed);
		
		// อัพเดท RPM value
		lv_label_set_text_fmt(rpm_value_label, "%d", rpm);
		
		// อัพเดท Gear value
		lv_label_set_text(gear_value_label, gear.c_str());
    }

    void update() {
		// อ่านข้อมูลจาก Serial
		read();
		
		// อัพเดทจอ LCD เมื่อมีข้อมูลใหม่
		if (dataUpdated) {
			displayData();
			dataUpdated = false;
		}
		
		// จัดการ blinking animation เมื่อยังไม่ได้รับข้อมูล
		if (!hasReceivedData) {
			static unsigned long lastBlink = 0;
			static bool visible = true;
			
			if (millis() - lastBlink > 1000) {
				if (visible) {
					lv_obj_clear_flag(waiting_label, LV_OBJ_FLAG_HIDDEN);
				} else {
					lv_obj_add_flag(waiting_label, LV_OBJ_FLAG_HIDDEN);
				}
				visible = !visible;
				lastBlink = millis();
			}
		}
		
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