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

// The persistent values in EEPROM
class eeprom_data_t 
{
private:
    static const 
    uint8_t Signature {0xA5};

    uint8_t DefaultTries {50};

    uint8_t signature {0};
    uint8_t dirty {0};
    uint8_t tries {0};
    uint8_t locked {0};
    struct {
        double      lat {0.0};
        double      lon {0.0};
    } targets[MAX_TARGETS];

public:

    eeprom_data_t()
    {
    }
    
    void init()
    {
        signature = Signature;
        tries = DefaultTries;
        dirty = true;
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
        if (num >= get_num_targets())
            return false;

        dirty = true;
        for (; num < get_num_targets()-1; num++) {
            targets[num] = targets[num + 1];
        }

        for (; num < int(ARRAYSZ(targets)); ++num) {
            targets[num].lat = 0.0;
            targets[num].lon = 0.0;
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
    // Number of Tries:

    uint8_t get_default_tries() const
    {
        return DefaultTries;
    }

    void set_default_tries(uint8_t t)
    {
        DefaultTries = t;
    }

    uint16_t get_num_tries() const
    {
        return tries;
    }

    void set_tries(uint16_t const t)
    {
        if (tries != t) {
            tries = t;
            dirty = true;
        }
    }

    uint16_t reduce_tries()
    {
        if (tries > 0)
        {
            dirty = true;
            return --tries;
        }

        return 0;
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
