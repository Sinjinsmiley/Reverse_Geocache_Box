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
|*| [+] Up to 8 sequential targets can be configured!
|*| [+] Each target has a clue of up to 32 characters that is shown
|*|     (scrolling if needed) along with the distance to it.
|*| [+] Says whether each check is hotter or colder than the last one.
|*| [+] Shows progress through the targets ("Location 2 of 4").
|*| [+] Shows a message from the giver when the box unlocks.
|*| [+] Waits for a steady GPS fix, and gives up with a message if
|*|     there is no signal.
|*| [+] All configurable target(s), clue(s), and the unlock message are
|*|     stored in EEPROM.
|*| [+] Detects if EEPROM has never been programmed and automatically
|*|     updates it the first time if needed.
|*| [+] Only modified values are updated in EEPROM - saves write cycles
|*| [+] Displays battery voltage and cpu temperature using the ATMega328's
|*|     internal ADC and diode!
|*| [+] Clock mode, showing UK time (GMT or BST)
|*| [+] Menu system
|*| 
|*| TODO:
|*| [ ] give the user a direction heading
|*| [ ] menu for user-configurable target(s)
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
// write new target(s), clue(s), and the unlock message to the EEPROM,
// unlock the box so the gift can be put inside, and show the clue and
// distance for the first target. Never returns.
//
void program_box(LiquidCrystal &lcd)
{
    // start from blank data, replacing any old target(s), clue(s), and message
    config_data.init();

    // Set at least one target, up to MAX_TARGETS, each with a clue of up
    // to MAX_CLUE_LEN characters. Clues longer than 16 characters scroll
    // across the display. Targets must be reached in order: 0, 1, 2, ...
    static const double LONDON_LAT = 51.509865, LONDON_LON = -0.118092;
    config_data.set_target_lat(LONDON_LAT, 0);
    config_data.set_target_lon(LONDON_LON, 0);
    config_data.set_clue(0, "Mind the gap near the Thames");

    // remember how many targets were set so progress can be shown
    config_data.set_total_targets();

    // Set the message shown when the box unlocks, up to MAX_CLUE_LEN
    // characters over the two lines. Leave it empty for no message.
    config_data.set_unlock_message("Happy birthday! Enjoy your gift");

    // open the box and leave it marked as unlocked so the
    // next normal power on runs the locking countdown
    unlock_box();

    config_data.save();

    // wait until we get a valid satellite lock:
    wait_for_gps(lcd);

    // show how many targets were saved, then the first clue and distance
    char line1[32] = "";
    static char const fmtP[] PROGMEM = "Saved %d targets";
    char fmt[20];
    strcpy_P(fmt, fmtP);
    sprintf(line1, fmt, config_data.get_num_targets());
    update_lcd(lcd, line1);
    smartDelay(2000);

    static char const no_hint[] PROGMEM = "";
    show_target(lcd, get_target_miles(), no_hint);

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
    // EEPROM with the target(s) and clue(s) that are set
    // in program_box() above
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
    // wait until we get a steady satellite lock:
    wait_for_gps(lcd);

    // ------------------------------------------------------------
    // update the display with the clue for the target, the
    // distance to it, and whether it is hotter or colder
    update_target_display(lcd);

#endif

} // setup
