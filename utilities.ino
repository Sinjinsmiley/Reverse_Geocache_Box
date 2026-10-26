/*\
|*| utilities.ino
|*| 
|*| (c) 2022 Trent M. Wyatt.
|*| companion file for Reverse Geocache Box project
|*| 
\*/

#include "utilities.h"

// ----------------------------------------------------------------------
// delay the specified number of milliseconds while
// processing any received serial gps data
// 
void smartDelay(unsigned long ms/* = 0 */) {
    unsigned long start = millis();
    do
    {
        while (ss.available())
            gps.encode(ss.read());
    } while (millis() - start < ms);

} // smartDelay


// ----------------------------------------------------------------------
// open and configure the serial port:
// 
void init_serial() {
#if defined(SERIAL_OUTPUT)
#warning compiling with Serial port enabled.
    Serial.begin(115200);
#endif

} // init_serial


// ----------------------------------------------------------------------
// start software serial talking to the gps module:
// 
void init_gps() {
    ss.begin(4800);

} // init_gps


// ----------------------------------------------------------------------
// give up waiting for the gps: say why, then turn off. Never returns.
// 
void gps_give_up(LiquidCrystal &lcd) {
    if (gps.charsProcessed() < 10) {
        static const char line1[] PROGMEM = "No GPS data.";
        static const char line2[] PROGMEM = "check wiring.";
        update_lcd_P(lcd, line1, line2);
    }
    else {
        static const char line1[] PROGMEM = "No GPS signal.";
        static const char line2[] PROGMEM = "Try outside.";
        update_lcd_P(lcd, line1, line2);
    }
    smartDelay(DisplayTime * 1000);
    turn_off(lcd);

    // without a power switch we are still running, and
    // the callers need the gps, so stop here
    while (true) {
    }

} // gps_give_up


// ----------------------------------------------------------------------
// see if the gps has sent a recent, believable date and time. Before the
// gps module knows the time it sends empty fields, which TinyGPSPlus reads
// as valid zeros (the year 2000, month 0).
// 
bool gps_time_ok() {
    return gps.date.isValid() && gps.time.isValid()
        && gps.date.age() < GpsMaxAge && gps.time.age() < GpsMaxAge
        && gps.date.year() >= GpsMinYear && gps.date.year() <= GpsMaxYear
        && gps.date.month() >= 1 && gps.date.month() <= 12
        && gps.date.day() >= 1 && gps.date.day() <= 31;

} // gps_time_ok


// ----------------------------------------------------------------------
// wait for the gps to send the date and time. This only needs one
// satellite so it works in places where a position fix might not.
// Gives up and turns off after GpsFixTimeout seconds.
// 
void wait_for_gps_time(LiquidCrystal &lcd) {
    if (gps_time_ok()) {
        return;
    }

    static const char line1[] PROGMEM = "Getting time...";
    static const char line2[] PROGMEM = "";
    update_lcd_P(lcd, line1, line2);

    unsigned long const start = millis();
    while (!gps_time_ok()) {
        unsigned long const elapsed = millis() - start;
        if (elapsed >= GpsFixTimeout * 1000UL) {
            gps_give_up(lcd);
        }

        char secs[8];
        sprintf(secs, "%3us", unsigned(elapsed / 1000));
        lcd.setCursor(12, 1);
        lcd.print(secs);

        smartDelay(1000);
    }

} // wait_for_gps_time


// ----------------------------------------------------------------------
// wait until we get a steady lock on the satellites, showing the number
// of satellites and time waited. Gives up and turns off if there is no
// fix after GpsFixTimeout seconds.
// 
void wait_for_gps(LiquidCrystal &lcd) {
    // kept between calls so that we only wait for a steady fix once
    static bool          ready = false;
    static bool          have_fix = false;
    static unsigned long fix_time = 0;      // when we first got a fix

    if (ready) {
        return;
    }

    unsigned long const start = millis();
    bool shown_fix = false;
    bool first = true;

    while (true) {
        unsigned long const elapsed = millis() - start;
        bool const timed_out = elapsed >= GpsFixTimeout * 1000UL;

        // TinyGPSPlus keeps the last location (and isValid() stays true)
        // after the signal is lost, so only count a recent one
        bool const fresh = gps.location.isValid() && gps.location.age() < GpsMaxAge;

        if (!fresh) {
            // lost the fix, so a new one has to settle again
            have_fix = false;
        }

        if (fresh) {
            if (!have_fix) {
                have_fix = true;
                fix_time = millis();
            }

            // the hdop is updated even without a fix and an empty value
            // reads as 0, so 0 means unknown rather than perfect
            int32_t const hdop = gps.hdop.isValid() ? gps.hdop.value() : 0;
            bool const accurate = hdop > 0 && hdop <= MaxHdop;
            bool const settled = millis() - fix_time >= GpsSettleTime * 1000UL;

            // use the fix once it is accurate and has settled, or use
            // it anyway if it doesn't get accurate before the timeout
            if ((accurate && settled) || timed_out) {
                ready = true;
                return;
            }
        }
        else if (timed_out) {
            gps_give_up(lcd);
        }

        // only redraw the whole display when the top line changes
        // so that the progress on the bottom line doesn't flicker
        char line2[32];
        sprintf(line2, "Sats: %-2d    %3us", int(gps.satellites.value()), unsigned(elapsed / 1000));
        if (first || have_fix != shown_fix) {
            static const char waiting[] PROGMEM = "Waiting on GPS..";
            static const char steadying[] PROGMEM = "Steadying GPS...";
            char line1[17];
            strcpy_P(line1, have_fix ? steadying : waiting);
            update_lcd(lcd, line1, line2);
            first = false;
            shown_fix = have_fix;
        }
        else {
            lcd.setCursor(0, 1);
            lcd.print(line2);
        }

        smartDelay(1000);
    }

} // wait_for_gps


// ----------------------------------------------------------------------
// Move the servo to a specified position
void moveServo(ServoPos_t pos) {
    PWMServo servo;         // The servo that locks/unlocks the box
    servo.attach(SrvPin);   // start generating pwm on the pin as an output
    int delta = 0;

    switch (pos) {
        case SrvClose:  // SrvClose := 60
            delta = (SrvOpen > SrvClose) ? -1 : +1;
            for (unsigned newpos = SrvOpen; newpos != SrvClose; newpos += delta) {
                servo.write(newpos);
                smartDelay(16);
            }
            break;

        case SrvOpen:   // SrvOpen := 110
            delta = (SrvOpen > SrvClose) ? +1 : -1;
            for (unsigned newpos = SrvClose; newpos != SrvOpen; newpos += delta) {
                servo.write(newpos);
                smartDelay(16);
            }
            break;
    }

    smartDelay(100);
    servo.detach();         // set the pin as high-z input and stop generating pwm

} // moveServo


// ----------------------------------------------------------------------
// lock the bax
// 
void lock_box() {
    moveServo(SrvClose);
    config_data.setLocked();

} // lock_box


// ----------------------------------------------------------------------
// unlock the box
// 
void unlock_box() {
    moveServo(SrvOpen);
    config_data.setUnlocked();

} // unlock_box


// ----------------------------------------------------------------------
// turn the box off
// 
void turn_off(LiquidCrystal &lcd) {
    lcd_off(lcd);
    pinMode(PwrPin, OUTPUT);
    digitalWrite(PwrPin, HIGH);

} // turn_off


// ----------------------------------------------------------------------
// get the distance in meters from the current location to
// the specified location
// 
double get_distance(double latitude, double longitude) {
    double distanceMeters = 0.0;

    if (gps.location.isValid()) {
        double const lat = gps.location.lat();
        double const lon = gps.location.lng();

        distanceMeters = TinyGPSPlus::distanceBetween(lat, lon, latitude, longitude);
    }

    return distanceMeters;

} // get_distance


// ----------------------------------------------------------------------
// get the distance in miles from the current location to the current
// target, rounded down to two decimal places
// 
double get_target_miles() {
    double distanceMeters = get_distance(config_data.get_target_lat(0), config_data.get_target_lon(0));
    double distanceMiles = distanceMeters / 1000.0 * 0.6214;

    return double(uint32_t(distanceMiles * 100)) / 100.0;

} // get_target_miles


// ----------------------------------------------------------------------
// update the lcd display with the two lines of text from SRAM
// 
void update_lcd(LiquidCrystal &lcd, char const *line1/* = "" */, char const *line2/* = "" */) {
#if defined(SERIAL_OUTPUT)
    Serial.print(F("\n"));
    Serial.println(line1);
    Serial.println(line2);
#endif

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    smartDelay(200);

} // update_lcd

// ----------------------------------------------------------------------
// update the lcd display with the two lines of text from PROGMEM flash memory
// 
void update_lcd_P(LiquidCrystal &lcd, char const *line1, char const *line2) {
    char data1[32], data2[32];
    strncpy_P(data1, line1, sizeof(data1));
    strncpy_P(data2, line2, sizeof(data2));
    update_lcd(lcd, data1, data2);

} // update_lcd_P


// ----------------------------------------------------------------------
// show the two lines of text from SRAM for at least ms milliseconds.
// If line1 is too long for the display it scrolls across the top line,
// all the way through at least once.
// 
void scroll_lcd(LiquidCrystal &lcd, char const *line1, char const *line2, unsigned long ms) {
    update_lcd(lcd, line1, line2);

    int const extra = int(strlen(line1)) - 16;
    if (extra <= 0) {
        smartDelay(ms);
        return;
    }

    unsigned long const start = millis();
    do {
        for (int pos = 0; pos <= extra; ++pos) {
            char window[17];
            strncpy(window, line1 + pos, 16);
            window[16] = '\0';
            lcd.setCursor(0, 0);
            lcd.print(window);

            // pause at each end so the start and end are easy to read
            smartDelay((pos == 0 || pos == extra) ? ScrollPause : ScrollStep);
        }
    } while (millis() - start < ms);

} // scroll_lcd


// ----------------------------------------------------------------------
// show up to 32 chars of text over both lines, breaking
// the line at a space where possible
// 
void show_wrapped(LiquidCrystal &lcd, char const *text) {
    int const len = strlen(text);
    int brk = len;      // where line 1 ends
    int next = len;     // where line 2 starts

    if (len > 16) {
        brk = next = 16;

        // break at the last space that fits on line 1, if the rest fits on line 2
        for (int i = 16; i > 0; --i) {
            if (text[i] == ' ' && len - (i + 1) <= 16) {
                brk = i;
                next = i + 1;
                break;
            }
        }
    }

    char line1[17], line2[17];
    strncpy(line1, text, brk);
    line1[brk] = '\0';
    strncpy(line2, text + next, 16);
    line2[16] = '\0';
    update_lcd(lcd, line1, line2);

} // show_wrapped


// ----------------------------------------------------------------------
// turn the lcd display off
// 
void lcd_off(LiquidCrystal &lcd) {
    lcd.noDisplay();
    smartDelay(200);

} // lcd_off


// ----------------------------------------------------------------------
// display the current latitude and longitude. for debugging/testing
// 
void update_local_display(LiquidCrystal &lcd) {
//    double next_lat = config_data.get_target_lat(0);
//    double next_lon = config_data.get_target_lon(0);

    double cur_lat = gps.location.lat();
    double cur_lon = gps.location.lng();

    char buff1[64], buff2[64], tmp[16];
    dtostrf(cur_lat, 14, 7, tmp);
    strcpy(buff1, tmp);
    dtostrf(cur_lon, 14, 7, tmp);
    strcpy(buff2, tmp);
    update_lcd(lcd, buff1, buff2);
    smartDelay(DisplayTime * 1000);    

} // update_local_display


// ----------------------------------------------------------------------
// show the clue for the current target on the top line, scrolling if
// needed, and the distance to it on the bottom line with the hint (from
// PROGMEM) on the right. e.g. "3.42 mi   Hotter"
// 
void show_target(LiquidCrystal &lcd, double miles, char const *hint) {
    char clue[MAX_CLUE_LEN + 1];
    config_data.get_clue(0, clue);

    // drop the decimals for far away targets so the line still fits
    char line2[32];
    dtostrf(miles, 1, (miles >= 1000.0) ? 0 : 2, line2);
    static char const mi[] PROGMEM = " mi";
    strcat_P(line2, mi);

    char hint_buf[8];
    strcpy_P(hint_buf, hint);
    int len = strlen(line2);
    int const pad = 16 - len - int(strlen(hint_buf));
    for (int i = 0; i < pad; ++i) {
        line2[len++] = ' ';
    }
    line2[len] = '\0';
    strcat(line2, hint_buf);

    scroll_lcd(lcd, clue, line2, DisplayTime * 1000);

} // show_target


// ----------------------------------------------------------------------
// Handle arriving at our next target location!
// 
void target_success(LiquidCrystal &lcd) {
    // We successfully reached the target!
    static const char line1[] PROGMEM = "Congratulations!";

    // remove the current target from the list of targets
    config_data.remove_target(0);
    
    // see if this is the last target
    if (config_data.get_num_targets() == 0) {
        static const char line2[] PROGMEM = " Box Unlocked!";
        update_lcd_P(lcd, line1, line2);
        unlock_box();
        config_data.save();
        smartDelay(DisplayTime * 1000);

        // show the giver's message, if there is one
        char msg[MAX_CLUE_LEN + 1];
        config_data.get_unlock_message(msg);
        if (msg[0] != '\0') {
            show_wrapped(lcd, msg);
            smartDelay(DisplayTime * 1000);
        }

        return;
    }

    // say which location is next
    int const total = config_data.get_total_targets();
    char line2[32];
    static char const fmtP[] PROGMEM = "  On to %d of %d";
    char fmt[20];
    strcpy_P(fmt, fmtP);
    sprintf(line2, fmt, total - config_data.get_num_targets() + 1, total);
    char data1[17];
    strcpy_P(data1, line1);
    update_lcd(lcd, data1, line2);

    config_data.save();
    smartDelay(DisplayTime * 1000);

    // show where to go next. hotter/colder is measured from here.
    double const miles = get_target_miles();
    config_data.set_last_miles(miles);
    config_data.save();

    static char const no_hint[] PROGMEM = "";
    show_target(lcd, miles, no_hint);
}


// ----------------------------------------------------------------------
// update the display with the clue for the target, the distance to it,
// and whether it is hotter or colder than the last check. Handles
// arriving at target(s) and possibly unlocking the box.
// 
void update_target_display(LiquidCrystal &lcd) {

    // Use Clock Mode if all targets have been solved.
    // This only needs the time, not a position fix.
    if (config_data.get_num_targets() == 0) {
        // we have unlocked all of the targets.
        show_datetime(lcd);
        smartDelay(DisplayTime * 1000);
        turn_off(lcd);

        return;
    }

    // wait until we get a steady satellite lock:
    wait_for_gps(lcd);

    double const miles = get_target_miles();

    if (miles <= 0.01) {
        target_success(lcd);
        turn_off(lcd);

        return;
    }

    // show which location this is if there is more than one
    int const total = config_data.get_total_targets();
    if (total > 1) {
        char line1[32];
        static char const fmtP[] PROGMEM = "Location %d of %d";
        char fmt[20];
        strcpy_P(fmt, fmtP);
        sprintf(line1, fmt, total - config_data.get_num_targets() + 1, total);
        update_lcd(lcd, line1);
        smartDelay(ProgressTime * 1000);
    }

    // compare with the distance at the last check. The last distance only
    // moves on when the change is big enough to count, so that several
    // small steps in the same direction still add up to hotter or colder.
    static char const no_hint[] PROGMEM = "";
    static char const hotter[] PROGMEM = "Hotter";
    static char const colder[] PROGMEM = "Colder";
    static char const same[] PROGMEM = "Same";

    double const last = config_data.get_last_miles();
    double const margin = HotColdFeet / 5280.0;
    char const *hint = no_hint;

    if (last <= 0.0) {
        config_data.set_last_miles(miles);
    }
    else if (miles < last - margin) {
        hint = hotter;
        config_data.set_last_miles(miles);
    }
    else if (miles > last + margin) {
        hint = colder;
        config_data.set_last_miles(miles);
    }
    else {
        hint = same;
    }

    config_data.save();

    show_target(lcd, miles, hint);

    turn_off(lcd);

} // update_target_display


// ----------------------------------------------------------------------
// display the Vin voltage and processor temperature
// 
void show_temp_volt(LiquidCrystal &lcd) {
    char line1[32], line2[32], buf[8];

    // calculate Vcc in millivolts:
    signed long millivolts = readVcc();

    static char const fmt1[] PROGMEM = "volts: ";
    strcpy_P(line1, fmt1);

    dtostrf(millivolts/1000.0, 5, 2, buf);
    strcat(line1, buf);

    static char const fmt2[] PROGMEM = "temp: ";
    strcpy_P(line2, fmt2);

    bool bCelcius = false;

    // Read the processor temperature in celcius:
    double temp = temperature();

    // Convert to fahrenheit if desired:
    temp = bCelcius ? temp : (temp * 2.0) + 30.0;

    // Format string of 6 total digits (incl '.') with 2 decimal place
    dtostrf(temp, 6, 2, buf);
    strcat(line2, buf);

    update_lcd(lcd, line1, line2);

} //  show_temp_volt


// ----------------------------------------------------------------------
// show the current UK date and time
// used in clock mode
// 
void show_datetime(LiquidCrystal &lcd) {
    char line1[32], line2[32];
    static char const month[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    tm dt = get_uk_time(lcd);

    sprintf(line1, "%d %s %d", dt.tm_mday, month[dt.tm_mon], dt.tm_year + 1900);
    sprintf(line2, "%02d:%02d:%02d %s", dt.tm_hour, dt.tm_min, dt.tm_sec, dt.tm_isdst ? "BST" : "GMT");
    update_lcd(lcd, line1, line2);

} // show_datetime


// ----------------------------------------------------------------------
// check for the secret backdoor to open the box
// and access any special menus or features
// 
void check_secret(LiquidCrystal &lcd) {
    // set up one pin as an INPUT that defaults to HIGH. set up the other pin as an OUTPUT:
    pinMode(SecretPin1, INPUT_PULLUP);
    pinMode(SecretPin2, OUTPUT);

    // check to see if the input pin repeatedly follows the output pin:
    int i = 0;
    for (; i < 7; ++i) {
        digitalWrite(SecretPin2, LOW);
        if (digitalRead(SecretPin1))
            break;

        // so far it looks like the pins might be connected. test further.
        digitalWrite(SecretPin2, HIGH);
        if (!digitalRead(SecretPin1))
            break;
    }

    // return the pins to their original high impedence INPUT mode
    pinMode(SecretPin1, INPUT);
    pinMode(SecretPin2, INPUT);

    if (7 != i) {
        // the pins do not appear to be connected so just return and do nothing
        return;
    }
        
    // the pins are connected together!

    static const char line1[] PROGMEM = "-=   Secret   =-";
    static const char line2[] PROGMEM = "-=  Backdoor  =-";
    update_lcd_P(lcd, line1, line2);

    unlock_box();
    config_data.save();

    smartDelay(2000);

    // ----------------------------------------------------------------------
    // wait until we get a valid satellite lock:
    wait_for_gps(lcd);

    update_local_display(lcd);

    turn_off(lcd);
}


// ----------------------------------------------------------------------
// see if the box needs locking and if so
// display a countdown and then lock
// 
void check_developer_lock(LiquidCrystal &lcd) {
#if defined(DEVELOPER_MODE)
#warning compiling for DEVELOPER_MODE.
    static char const line1[] PROGMEM = "  Lock Stopped  ";
    static char const line2[] PROGMEM = " DEVELOPER_MODE ";
    update_lcd_P(lcd, line1, line2);
    smartDelay(2000);
    return;
#endif

    if (config_data.get_num_targets() == 0) {
        // the hunt is over, so just carry on to the clock
        if (config_data.get_total_targets() > 0) {
            return;
        }

        // the box has never been programmed
        static char const line1[] PROGMEM = "  Lock Stopped  ";
        static char const line2[] PROGMEM = "   No Targets   ";
        update_lcd_P(lcd, line1, line2);
        smartDelay(2000);
        return;
    }

    if (config_data.isLocked()) {
        return;
    }

    for (int i=10; i >= 0; i--) {
        char line1[32]="", line2[32]="";
        static char const fmt1[] PROGMEM = " Dev Lock Mode  ";
        strcpy_P(line1, fmt1);
        sprintf(line2, "Locking in: %2d..", i);
        update_lcd(lcd, line1, line2);
        smartDelay(850);
    }

    lock_box();

    config_data.setLocked();
    config_data.save();
}


// ----------------------------------------------------------------------
// day of the week for a date, 0 = Sunday (Sakamoto's method)
// 
int day_of_week(int year, int month, int day) {
    static const uint8_t offsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (month < 3) {
        year--;
    }
    return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;

} // day_of_week


// ----------------------------------------------------------------------
// number of days in a month, 1 = January
// 
int days_in_month(int year, int month) {
    static const uint8_t days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    bool const leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return (month == 2 && leap) ? 29 : days[month - 1];

} // days_in_month


// ----------------------------------------------------------------------
// see if UK clocks are on BST for a date and hour in GMT. They go forward
// at 01:00 GMT on the last Sunday in March and back at 01:00 GMT on the
// last Sunday in October.
// 
bool is_bst(int year, int month, int day, int hour) {
    if (month < 3 || month > 10) {
        return false;
    }
    if (month > 3 && month < 10) {
        return true;
    }

    int const last_sunday = 31 - day_of_week(year, month, 31);
    bool const after_change = day > last_sunday || (day == last_sunday && hour >= 1);

    return (month == 3) ? after_change : !after_change;

} // is_bst


// ----------------------------------------------------------------------
// get the UK time (GMT or BST) from the gps satellite time, which is in
// GMT. tm_isdst is set when it is BST.
// 
tm get_uk_time(LiquidCrystal &lcd) {
    wait_for_gps_time(lcd);

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();

    bool const bst = is_bst(year, month, day, hour);

    // BST is an hour ahead, which can move on to the next day. BST is
    // never in effect on 31 December so the year never changes.
    if (bst && ++hour == 24) {
        hour = 0;
        if (++day > days_in_month(year, month)) {
            day = 1;
            month++;
        }
    }

    tm t; // std C time and date struct

    t.tm_sec = gps.time.second();
    t.tm_min = gps.time.minute();
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_mon = month - 1;
    t.tm_year = year - 1900;
    t.tm_wday = day_of_week(year, month, day);
    t.tm_yday = 0;
    t.tm_isdst = bst;

    return t;

} // get_uk_time


// ----------------------------------------------------------------------
// test all of the features of the box
// 
void test_all(LiquidCrystal &lcd) {
    if (!config_data.load()) {
        static const char line1[] PROGMEM = "EEPROM load fail";
        static const char line2[] PROGMEM = "";
        update_lcd_P(lcd, line1, line2);
    }
    else {
        static const char line1[] PROGMEM = "Hello, world!";
        static const char line2[] PROGMEM = "line 2";
        update_lcd_P(lcd, line1, line2);
    }

    update_local_display(lcd);
    smartDelay(DisplayTime * 1000);

    show_datetime(lcd);
    smartDelay(2000);

    show_temp_volt(lcd);
    smartDelay(2000);

    uint32_t timer = millis() + DisplayTime * 1000;
    while (millis() < timer) {
        lock_box();
        smartDelay(1000);
        
        unlock_box();
        smartDelay(1000);
    }

    unlock_box();

    turn_off(lcd);

} // test_all
