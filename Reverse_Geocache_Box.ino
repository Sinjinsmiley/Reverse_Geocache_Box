/*\
|*| Reverse_Geocache_Box.ino
|*| main file for Reverse Geocache Box project
|*| 
|*| (c) 2022 Trent M. Wyatt.
|*| 
|*| Features so far:
|*| 
|*| [+] Box only unlocks at a special location.
|*| [+] Written for Arduino Nano but this will work with any other 
|*|     Arduino that has enough pins.
|*| [+] Runs on 2 3.7V rechargeable lipo batteries
|*| [+] 5V LDO (low drop-out) regulator circuit
|*| [+] Uses solid state power button.  Turns itself off.  0.000A used when off.
|*| [+] 16x2 LCD display (4-bit parallel)
|*| [+] 4800 baud serial GPS module
|*| [+] Metal Gear Digital Servo controls the box lock mechanism
|*| [+] Hidden easter egg to unlock or get into internal debugger/setup mode
|*| [+] Displays messages on important dates.
|*| [+] Up to 8 special dates/times with messages can be stored!
|*| [+] Up to 8 sequential targets can be configured!
|*| [+] All configurable target(s), important date(s) and messages, and
|*|     remaining tries are stored in EEPROM.
|*| [+] Detects if EEPROM has never been programmed and automatically
|*|     updates it the first time if needed.
|*| [+] Only modified values are updated in EEPROM - saves write cycles
|*| [+] Displays battery voltage and cpu temperature using the ATMega328's
|*|     internal ADC and diode!
|*| [+] Clock mode
|*| [+] Menu system
|*| 
|*| TODO:
|*| [ ] give the user a direction heading
|*| [ ] menu for user-configurable target(s)
|*| [ ] menu for user-configurable important date(s) and messages
|*| [ ] alarms during clock mode
|*| 
\*/

// see "utilities.h" for pin assignments and change if needed
#include "utilities.h"

// ======================================================================
// Global variables

// The TinyGPSPlus object
TinyGPSPlus      gps;

// The serial connection to the GPS device
SoftwareSerial   ss(RXPin, TXPin);

// The persistent values in EEPROM
eeprom_data_t    config_data;

// ======================================================================

#if defined(PROGRAM_EEPROM)
#warning compiling for PROGRAM_EEPROM.

// ----------------------------------------------------------------------
// write new target(s), date(s), and number of tries to the EEPROM,
// unlock the box so the gift can be put inside, and show the distance
// to the first target without using up a try. Never returns.
//
void program_box(LiquidCrystal &lcd)
{
    // config_data has not been loaded from the EEPROM so it starts out
    // blank, and saving it replaces any old target(s) and date(s).

    // set the number of tries before init() copies it into the remaining tries
    config_data.set_default_tries(DefaultTries);
    config_data.init();

    // Set any special date(s), up to MAX_DATES. Date and time are gmt.
    datetime_t alert = datetime_t(2022, 12, 29, 20, 30, 0);
    config_data.add_alert(alert, "Alert", "One");

    // Set at least one target, up to MAX_TARGETS. Targets must be
    // reached in order: 0, 1, 2, ...
    static const double LONDON_LAT = 51.509865, LONDON_LON = -0.118092;
    config_data.set_target_lat(LONDON_LAT, 0);
    config_data.set_target_lon(LONDON_LON, 0);

    // open the box and leave it marked as unlocked so the
    // next normal power on runs the locking countdown
    unlock_box();

    config_data.save();

    // wait until we get a valid satellite lock:
    wait_for_gps(lcd);

    // show what was saved and how far away the first target is
    double distanceMiles = get_distance(
        config_data.get_target_lat(0), config_data.get_target_lon(0)) / 1000.0 * 0.6214;

    char line1[32] = "", line2[32] = "";
    static char const fmtP[] PROGMEM = "%d tgt %d tries";
    char fmt[16];
    strcpy_P(fmt, fmtP);
    sprintf(line1, fmt, config_data.get_num_targets(), config_data.get_num_tries());

    dtostrf(distanceMiles, 7, 2, line2);
    static char const miles[] PROGMEM = " mi away";
    strcat_P(line2, miles);

    update_lcd(lcd, line1, line2);
    smartDelay(DisplayTime * 1000);

    // remind the developer to turn this mode off, otherwise every
    // power on erases any progress and unlocks the box again
    static char const done1[] PROGMEM = "Done. Now remove";
    static char const done2[] PROGMEM = " PROGRAM_EEPROM ";
    update_lcd_P(lcd, done1, done2);
    smartDelay(DisplayTime * 1000);

    turn_off(lcd);

    // without a power switch we are still running so stop here
    // instead of falling through to the normal game
    while (true) {
    }

} // program_box

#endif

void loop()
{
}

void setup()
{
    init_serial();

    LiquidCrystal lcd(LcdRSPin, LcdEnPin, LcdD4Pin, LcdD5Pin, LcdD6Pin, LcdD7Pin);
    lcd.begin(16, 2);

    init_gps();

    // ------------------------------------------------------------
    // enable PROGRAM_EEPROM in utilities.h and run once to update the
    // EEPROM with the target(s), date(s), and number of tries that
    // are set in program_box() above
#if defined(PROGRAM_EEPROM)
    program_box(lcd);
#endif

#if defined(SELF_TEST)
#warning compiling for self test diagnostics.

    // ------------------------------------------------------------
    // wait until we get a valid satellite lock:
    wait_for_gps(lcd);

    // ------------------------------------------------------------
    // run the diagnostic/debugging tests:
    test_all(lcd);

#else

    // ------------------------------------------------------------
    // load the persistent data from the EEPROM:
    config_data.load();

    // ------------------------------------------------------------
    // check for the secret back-door:
    check_secret(lcd);

    // see if we need to lock the box
    check_developer_lock(lcd);

    // ------------------------------------------------------------
    // wait until we get a valid satellite lock:
    wait_for_gps(lcd);

    // ------------------------------------------------------------
    // check for special dates
    check_dates(lcd);

    // ------------------------------------------------------------
    // update the display with the target distance
    // and reduce the number of remaining tries
    update_target_display(lcd);

#endif

} // setup
