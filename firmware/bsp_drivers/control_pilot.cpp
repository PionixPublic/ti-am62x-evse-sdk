#include "../bsp_drivers/control_pilot.hpp"

#include <drivers/pinmux.h>

#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/dpl/TimerP.h>

#include "../bsp_drivers/task_config.h"
#include "../bsp_drivers/timer_pwm_priv.h"

uint32_t ovfls = 0;
uint32_t matchs = 0;

void pwm_timer_isr(void* arg) {
    auto ctx = reinterpret_cast<ControlPilot*>(arg);
    auto irq_status = CSL_REG32_RD_OFF_RAW(ctx->_PWM_TIMER_BASE, TIMER_IRQSTATUS_OFFSET);

    if (irq_status & TIMER_IRQSTATUS_OVERFLOW_FLAG) {
        ovfls++;

        ctx->sampler.trigger_high();

        CSL_REG32_WR_OFF_RAW(ctx->_PWM_TIMER_BASE, TIMER_IRQSTATUS_OFFSET, TIMER_IRQSTATUS_OVERFLOW_FLAG);

    } else if (irq_status & TIMER_IRQSTATUS_MATCH_FLAG) {
        matchs++;

        ctx->sampler.trigger_low();

        CSL_REG32_WR_OFF_RAW(ctx->_PWM_TIMER_BASE, TIMER_IRQSTATUS_OFFSET, TIMER_IRQSTATUS_MATCH_FLAG);
    }
}

static HwiP_Object pwm_int_ctx;

static uint32_t get_default_tclr(uint32_t* timer_base_address) {
    // NOTE (aw): the update of the TCLR register needs like 4 functional clock cycles
    // so reading it directly after setting, might yield an old value
    // if the sysconfig timer initialization took off right before this function, we might
    // get in trouble

    constexpr uint32_t USECS_PER_SECS = 1000000;
    ClockP_usleep((4 * USECS_PER_SECS) / PWM_TIMER_INPUT_CLK_HZ + 1);

    auto tclr = CSL_REG32_RD_OFF_RAW(timer_base_address, TIMER_TCLR_OFFSET);

    // default settings
    // no pwm toggle (so it can be set statically to zero and one)
    tclr &= ~TIMER_TCLR_TRG_MASK;
    // toggle mode (instead of pulse)
    tclr &= ~TIMER_TCLR_PT_MASK;
    tclr |= TIMER_TCLR_PT_TOGGLE;
    // pwm out mode
    tclr &= ~TIMER_TCLR_GPO_CFG_MASK;
    tclr |= TIMER_TCLR_GPO_CFG_PWM;
    // set compare enable
    tclr &= ~TIMER_TCLR_CE_MASK;
    tclr |= (0x1u << TIMER_TCLR_CE_SHIFT);
    // timer is running (so we get the interrupts to sample)
    tclr &= ~TIMER_TCLR_START_MASK;
    tclr |= (0x1u << TIMER_TCLR_START_SHIFT);
    // default output is zero
    tclr &= ~TIMER_TCLR_SCPWM_MASK;

    return tclr;
}

// if the return value is 0, pwm should be completely on
// if the return value is TIMER_TICKS_PER_FULL_PWM_PERIOD, pwm should be completely off
uint32_t ControlPilot::calculate_match_ticks(uint32_t duty_cycle) {
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }

    return ((100 - duty_cycle) * TIMER_TICKS_PER_FULL_PWM_PERIOD) / 100;
    ;
}

ControlPilot::ControlPilot(Sampler& sampler_) :
    sampler(sampler_),
    _PWM_TIMER_BASE(reinterpret_cast<uint32_t*>(gTimerBaseAddr[PWM_TIMER])),
    DEFAULT_TCLR(get_default_tclr(_PWM_TIMER_BASE)) {

    // FIXME (aw): we need to decide, whether sysconfig should setup parts of the driver, or if we should do the
    // complete setup
    // FOR now we leave it up to sysconfig

    // enable overflow and match interrupt
    CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_IRQSTATUS_SET_OFFSET,
                         TIMER_IRQSTATUS_MATCH_FLAG | TIMER_IRQSTATUS_OVERFLOW_FLAG);

    // setup the load register;
    CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_TLDR_OFFSET, 0xFFFFFFFF - TIMER_TICKS_PER_FULL_PWM_PERIOD + 1);

    // setup the match register;
    CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_TMAR_OFFSET, 0xFFFFFFFF - calculate_match_ticks(50) + 1);

    // trigger counter reload (any write value will do)
    CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_TTGR_OFFSET, 0xFFFFFFFF);

    // write back
    CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_TCLR_OFFSET, DEFAULT_TCLR);

    // enable io muxing
    Pinmux_PerCfg_t io_timer_pwm_pmx[] = {
        {PIN_MCU_MCAN1_TX, PIN_MODE(1)},
        {PINMUX_END, static_cast<uint32_t>(PINMUX_END)},
    };

    Pinmux_config(io_timer_pwm_pmx, PINMUX_DOMAIN_ID_MCU);

    // enable interrupts
    HwiP_Params pwm_int_params;
    HwiP_Params_init(&pwm_int_params);
    pwm_int_params.intNum = TIMER2_INT_NUM;
    pwm_int_params.callback = pwm_timer_isr;
    pwm_int_params.args = this;
    pwm_int_params.isPulse = 0;
    pwm_int_params.priority = 4; // FIXME (aw): which priority?

    auto status = HwiP_construct(&pwm_int_ctx, &pwm_int_params);
    DebugP_assertNoLog(status == SystemP_SUCCESS);
};

void ControlPilot::set_pwm(float duty_cycle) {
    if (duty_cycle < 0) {
        duty_cycle = 0;
    }

    auto tclr = DEFAULT_TCLR;

    auto match_ticks = calculate_match_ticks(100 * duty_cycle);

    // FIXME (aw): where shall we sample high and low when pwm is 0 or 100?
    if (match_ticks == 0) {
        // should be completely on
        tclr |= (0x1u << TIMER_TCLR_SCPWM_SHIFT);
    } else if (match_ticks >= TIMER_TICKS_PER_FULL_PWM_PERIOD) {
        // should be completely off
        // we can leave tclr as it is
    } else {

        // should be running normally, enable triggering of output
        tclr |= TIMER_TCLR_TRG_OVERFLOW_AND_MATCH;
        CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_TMAR_OFFSET, 0xFFFFFFFF - match_ticks + 1);
    }

    CSL_REG32_WR_OFF_RAW(_PWM_TIMER_BASE, TIMER_TCLR_OFFSET, tclr);
}

iec61851::CPSignal ControlPilot::get_cp_signal() {
    return sampler.get_latest_cp_signal();
}

void ControlPilot::enable() {
    set_pwm(1.0);
}

void ControlPilot::disable() {
    set_pwm(0.0);
}
