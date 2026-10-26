/*\
|*| eeprom_data.h
|*| 
|*| (c) 2022 Trent M. Wyatt.
|*| companion file for Reverse Geocache Box project
|*| 
\*/

#if !defined(EEPROM_DATA)
#define EEPROM_DATA

#include <EEPROM.h>

// Redefine as needed (and memory allows)
#define   MAX_TARGETS    8

// Redefine as needed (and EEPROM size allows)
// Also used for the length of the unlock message
#define   MAX_CLUE_LEN   32

// The persistent values in EEPROM
class eeprom_data_t 
{
private:
    // changed whenever the layout of the data changes so that
    // older data is reset instead of being read incorrectly
    static const 
    uint8_t Signature {0xA7};

    uint8_t signature {0};
    uint8_t dirty {0};
    uint8_t locked {0};
    uint8_t total_targets {0};      // number of targets when the box was programmed
    double  last_miles {0.0};       // distance at the last check, 0 if none yet
    struct {
        double      lat {0.0};
        double      lon {0.0};
    } targets[MAX_TARGETS];

public:

    eeprom_data_t()
    {
    }
    
    // reset to blank data with no targets or clues
    void init()
    {
        *this = eeprom_data_t();
        signature = Signature;
        dirty = true;

        for (int num = 0; num < MAX_TARGETS; ++num) {
            set_clue(num, "");
        }
        set_unlock_message("");
    }

    bool isValid() const
    {
        return Signature == signature;
    }

    bool isDirty() const
    {
        return dirty;
    }

    bool isLocked() const
    {
        return locked;
    }

    void setLocked()
    {
        if (!locked) {
            locked = true;
            dirty = true;
        }
    }

    void setUnlocked()
    {
        if (locked) {
            locked = false;
            dirty = true;
        }
    }

    // ----------------------------------------------------------------------
    // Targets

    int get_num_targets() const {
        unsigned num = 0;

        for (; num < ARRAYSZ(targets); ++num) {
            if (targets[num].lat == 0.0 && targets[num].lon == 0.0)
                break;
        }
        
        return num;
    }

    bool remove_target(int num) {
        int const count = get_num_targets();
        if (num >= count)
            return false;

        dirty = true;
        char clue[MAX_CLUE_LEN + 1];
        for (; num < count - 1; num++) {
            targets[num] = targets[num + 1];
            get_clue(num + 1, clue);
            set_clue(num, clue);
        }

        for (; num < int(ARRAYSZ(targets)); ++num) {
            targets[num].lat = 0.0;
            targets[num].lon = 0.0;
            set_clue(num, "");
        }

        // there is no previous distance to the new current target
        last_miles = 0.0;
        
        return true;
    }

    // remember how many targets there are now, for showing progress
    void set_total_targets()
    {
        total_targets = get_num_targets();
        dirty = true;
    }

    int get_total_targets() const
    {
        return total_targets;
    }

    // ----------------------------------------------------------------------
    // Distance at the last check, for hotter/colder

    double get_last_miles() const
    {
        return last_miles;
    }

    void set_last_miles(double const miles)
    {
        if (last_miles != miles) {
            last_miles = miles;
            dirty = true;
        }
    }

    double get_target_lat(int cur=0) const
    {
        return targets[cur].lat;
    }
    
    void set_target_lat(double const latitude, int cur)
    {
        if (targets[cur].lat != latitude) {
            targets[cur].lat = latitude;
            dirty = true;
        }
    }

    double get_target_lon(int cur = 0) const
    {
        return targets[cur].lon;
    }
    
    void set_target_lon(double const longitude, int cur)
    {
        if (targets[cur].lon != longitude) {
            targets[cur].lon = longitude;
            dirty = true;
        }
    }

    // ----------------------------------------------------------------------
    // Clues and the unlock message
    // These are kept in the EEPROM just after the rest of this data, a
    // clue for each target and then the unlock message, and are only read
    // when needed so they don't take up any SRAM. Unlike the rest of the
    // data they are written right away. Longer text is cut to MAX_CLUE_LEN
    // chars, and buf must hold MAX_CLUE_LEN + 1 chars.

    // num is the target number, or MAX_TARGETS for the unlock message
    static int text_address(int num)
    {
        return sizeof(eeprom_data_t) + num * (MAX_CLUE_LEN + 1);
    }

    static void read_text(int addr, char buf[])
    {
        for (int i = 0; i < MAX_CLUE_LEN; ++i) {
            buf[i] = EEPROM.read(addr + i);
            if (buf[i] == '\0')
                break;
        }
        buf[MAX_CLUE_LEN] = '\0';
    }

    static void write_text(int addr, char const *text)
    {
        int i = 0;
        for (; i < MAX_CLUE_LEN && text[i] != '\0'; ++i) {
            EEPROM.update(addr + i, text[i]);
        }
        EEPROM.update(addr + i, '\0');
    }

    void get_clue(int num, char buf[]) const
    {
        read_text(text_address(num), buf);
    }

    void set_clue(int num, char const *clue)
    {
        write_text(text_address(num), clue);
    }

    // shown when the last target is reached and the box unlocks
    void get_unlock_message(char buf[]) const
    {
        read_text(text_address(MAX_TARGETS), buf);
    }

    void set_unlock_message(char const *msg)
    {
        write_text(text_address(MAX_TARGETS), msg);
    }

    // ----------------------------------------------------------------------
    // Loading from EEPROM and Saving to EEPROM

    bool load()
    {
        EEPROM.get(0, *this);

        if (!isValid())
        {
            init();
            save();
        }

        dirty = false;

        return isValid();
    }

    void save()
    {
//        if (dirty) {
            dirty = false;
            EEPROM.put(0, *this);
//        }
    }

};

#endif // EEPROM_DATA
