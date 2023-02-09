#ifndef LIB_IEC61851_HAL_HPP
#define LIB_IEC61851_HAL_HPP

#include <stdint.h>

namespace iec61851 {

struct IRCD {
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual bool got_fired() = 0;
    virtual void reset() = 0;

    virtual ~IRCD(){};
};

struct IChargerLock {
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool is_locked() = 0;

    virtual ~IChargerLock(){};
};

struct IPowerSwitch {
    virtual bool on(bool use_three_phases) = 0;
    virtual bool off() = 0;
    virtual bool is_on() = 0;
    virtual bool reset_emergency_switch() = 0;

    virtual ~IPowerSwitch(){};
};

struct CPSignal {
    bool valid{false};
    float high;
    float low;
};

struct IControlPilot {
    virtual void set_pwm(float duty_cycle) = 0;
    virtual CPSignal get_cp_signal() = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual ~IControlPilot(){};
};

struct IClock {
    // FIXME (aw): should be const somehow
    uint32_t ticks_per_ms;
    uint32_t (*get_current_ticks)();
};

struct HAL {
    IRCD& rcd;
    IChargerLock& lock;
    IPowerSwitch& power_switch;
    IControlPilot& cp;
    IClock& clock;
};

} // namespace iec61851

#endif // LIB_IEC61851_HAL_HPP
