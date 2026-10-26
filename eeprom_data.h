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
#define   MAX_CLUE_LEN   32

// The persistent values in EEPROM
class eeprom_data_t 
{
private:
    // changed whenever the layout of the data changes so that
    // older data is reset instead of being read incorrectly
    static const 
    uint8_t Signature {0xA6};

    uint8_t signature {0};
    uint8_t dirty {0};
    uint8_t locked {0};
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
        
        return true;
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
    // Clues
    // The clues are kept in the EEPROM just after the rest of this data
    // and are only read when needed so they don't take up any SRAM.
    // Unlike the rest of the data they are written right away.

    static int clue_address(int num)
    {
        return sizeof(eeprom_data_t) + num * (MAX_CLUE_LEN + 1);
    }

    // copy the clue for a target into buf, which must hold MAX_CLUE_LEN + 1 chars
    void get_clue(int num, char buf[]) const
    {
        int const addr = clue_address(num);
        for (int i = 0; i < MAX_CLUE_LEN; ++i) {
            buf[i] = EEPROM.read(addr + i);
            if (buf[i] == '\0')
                break;
        }
        buf[MAX_CLUE_LEN] = '\0';
    }

    // set the clue for a target. Longer clues are cut to MAX_CLUE_LEN chars.
    void set_clue(int num, char const *clue)
    {
        int const addr = clue_address(num);
        int i = 0;
        for (; i < MAX_CLUE_LEN && clue[i] != '\0'; ++i) {
            EEPROM.update(addr + i, clue[i]);
        }
        EEPROM.update(addr + i, '\0');
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
