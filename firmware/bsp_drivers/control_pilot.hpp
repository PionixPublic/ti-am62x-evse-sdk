#ifndef DRIVERS_CONTROL_PILOT_HPP
#define DRIVERS_CONTROL_PILOT_HPP

#include "ti_dpl_config.h"

#include <iec61851_hal.hpp>

#include "sampler.hpp"

extern uint32_t matchs;
extern uint32_t ovfls;

extern "C" {
void pwm_timer_isr(void* arg);
}

class ControlPilot : public iec61851::IControlPilot {
public:
    ControlPilot(Sampler& sampler);

    void set_pwm(float duty_cycle) override;
    iec61851::CPSignal get_cp_signal() override;
    // FIXME (aw): enable and disable are unnecessary, we should have set_pwm
    // accepting an int between [0 .. 100], and just use that
    void enable() override;
    void disable() override;

private:
    static constexpr uint32_t PWM_FREQUENCY_HZ = 1000;
    static constexpr uint32_t TIMER_TICKS_PER_FULL_PWM_PERIOD =
        (PWM_TIMER_INPUT_CLK_HZ / PWM_TIMER_INPUT_PRE_SCALER) / PWM_FREQUENCY_HZ;

    Sampler& sampler;
    uint32_t* const _PWM_TIMER_BASE;

    const uint32_t DEFAULT_TCLR;
    static uint32_t calculate_match_ticks(uint32_t duty_cycle);

    friend void pwm_timer_isr(void* arg);
};

#endif // DRIVERS_CONTROL_PILOT_HPP
