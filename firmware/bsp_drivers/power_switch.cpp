#include "power_switch.hpp"

#include <ti_drivers_config.h>

PowerSwitch::PowerSwitch() :
    enable_gpio_base_addr(reinterpret_cast<uint32_t>(AddrTranslateP_getLocalAddr(AM62_R_ENABLE_BASE_ADDR))) {
    GPIO_setDirMode(enable_gpio_base_addr, AM62_R_ENABLE_PIN, AM62_R_ENABLE_DIR);
    off();
}

bool PowerSwitch::on(bool use_three_phases) {
    GPIO_pinWriteHigh(enable_gpio_base_addr, AM62_R_ENABLE_PIN);
    return is_on();
}

bool PowerSwitch::off() {
    GPIO_pinWriteLow(enable_gpio_base_addr, AM62_R_ENABLE_PIN);
    return !is_on();
}
bool PowerSwitch::is_on() {
    return GPIO_pinOutValueRead(enable_gpio_base_addr, AM62_R_ENABLE_PIN);
}
bool PowerSwitch::reset_emergency_switch() {
    return true;
}
