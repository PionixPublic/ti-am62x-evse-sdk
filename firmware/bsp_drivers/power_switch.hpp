#ifndef DRIVERS_POWER_SWITCH_HPP
#define DRIVERS_POWER_SWITCH_HPP

#include <iec61851_hal.hpp>

class PowerSwitch : public iec61851::IPowerSwitch {
public:
    PowerSwitch();
    bool on(bool use_three_phases) override;
    bool off() override;
    bool is_on() override;
    bool reset_emergency_switch() override;

private:
    uint32_t enable_gpio_base_addr;
};

#endif // DRIVERS_POWER_SWITCH_HPP
