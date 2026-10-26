/*\
|*| utilities.h
|*| 
|*| (c) 2022 Trent M. Wyatt.
|*| companion file for Reverse Geocache Box project
|*| 
\*/

#if !defined(UTILITIES_H)
#define  UTILITIES_H

#pragma   pack(0)

#define   ARRAYSZ(A)    (sizeof(A)/sizeof(*(A)))

#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <TinyGPSPlus.h>
#include <PWMServo.h>
#include <CPUVolt.h>
#include <CPUTemp.h>
#include <time.h>

#include "eeprom_data.h"
#include "utilities.h"
#include "menu.h"

// ======================================================================
// Manifest Constants to Control Compile-Time Options

// SERIAL_OUTPUT
// This enables the output of serial debug output.
// If SERIAL_OUTPUT is enabled all LCD output will go
// to the serial output instead of the lcd display.
// Note: Using the Serial library adds considerable flash and SRAM usage.
// Disable if not needed for production.

// DEVELOPER_MODE
// This disables locking the box

// SELF_TEST
// This runs a set of routines to exercise all of the various
// subsystems in the project for testing and debugging.

// PROGRAM_EEPROM
// This writes the target(s) and clue(s)
// set in program_box() (Reverse_Geocache_Box.ino) to the EEPROM,
// unlocks the box, and then halts. Upload with this enabled, power
// the box on once, then disable it and upload again.

// ======================================================================

// uncomment to enable serial debug output
//#define SERIAL_OUTPUT

// comment out to enable production mode
//#define DEVELOPER_MODE

// uncomment to enable SELF_TEST mode
//#define SELF_TEST

// uncomment to program the EEPROM with new target(s) and clue(s)
//#define PROGRAM_EEPROM

// external global variables
extern    SoftwareSerial   ss;
extern    TinyGPSPlus      gps;
extern    eeprom_data_t    config_data;

// ======================================================================
// Magic Numbers - cryptic numbers we don't want in the code

enum MagicNumber_t {
    // Pin Assignments
    // Adjust per project as your build requires.

    DbgLedPin  = LED_BUILTIN,   // debugging LED
    SecretPin1 = A1,            // hidden input pin attached to one hinge on the box lid
    SecretPin2 = A2,            // hidden output pin attached to the other hinge on the box lid
    SrvPin     = SERVO_PIN_A,   // pin 9 from PWMServo.h
    RXPin      = 4,             // receive from GPS module
    TXPin      = 3,             // transmit to GPS module
    PwrPin     = 2,             // raise HIGH to power off

    // LCD display pins (4-bit parallel mode)
    LcdRSPin   = 12,            // LCD register select
    LcdEnPin   = 11,            // LCD enable
    LcdD4Pin   = 5,             // LCD data bit 4
    LcdD5Pin   = 6,             // LCD data bit 5
    LcdD6Pin   = 7,             // LCD data bit 6
    LcdD7Pin   = 8,             // LCD data bit 7

    DisplayTime = 7,            // time to display before turning off

    GpsTimeout = 30000,         // timeout on no gps activity

    ScrollStep = 350,           // ms between each step of a scrolling clue
    ScrollPause = 1500,         // ms to pause at each end of a scrolling clue
};

enum ServoPos_t {
    // Servo positions
    SrvClose = 60,              // servo position for "box close"
    SrvOpen = 110,              // servo position for "box open"
};

void smartDelay(unsigned long ms = 0);
void init_serial();
void init_gps();
void wait_for_gps(LiquidCrystal &lcd);
void moveServo(uint8_t const pos);
void lock_box();
void unlock_box();
void turn_off(LiquidCrystal &lcd);
double get_distance(double latitude, double longitude);
double get_target_miles();
void clear_lcd(LiquidCrystal &lcd);
void update_lcd(LiquidCrystal &lcd, char const *line1 = "", char const *line2 = "");
void update_lcd_P(LiquidCrystal &lcd, char const *line1, char const *line2);
void scroll_lcd(LiquidCrystal &lcd, char const *line1, char const *line2, unsigned long ms);
void lcd_off(LiquidCrystal &lcd);
void update_local_display(LiquidCrystal &lcd);
void show_target(LiquidCrystal &lcd);
void update_target_display(LiquidCrystal &lcd);
void show_temp_volt(LiquidCrystal &lcd);
void show_datetime(LiquidCrystal &lcd);
void check_secret(LiquidCrystal &lcd);
void check_developer_lock(LiquidCrystal &lcd);
tm convert_cst(LiquidCrystal &lcd, int gmt_offset);
void test_all(LiquidCrystal &lcd);

#endif // UTILITIES_H
