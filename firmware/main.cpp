// FIXME (aw): copyright?
#include <stdlib.h>

#include "FreeRTOS.h"

#include <kernel/dpl/DebugP.h>

#include <iec61851_fsm.hpp>

#include "bsp_drivers/rpmsg.hpp"
#include "bsp_drivers/task_config.h"

#include "bsp_drivers/charger_lock.hpp"
#include "bsp_drivers/control_pilot.hpp"
#include "bsp_drivers/power_switch.hpp"
#include "bsp_drivers/rcd.hpp"

static constexpr uint32_t CHORE_INTERVAL_MS = 1000;

void handle_incoming_message(const HighToLow& in, iec61851::FSM& fsm) {
    if (in.which_message == HighToLow_set_pwm_tag) {
        auto& set_pwm = in.message.set_pwm;
        switch (set_pwm.state) {
        case PWMState_F:
            fsm.set_pwm_f();
            break;
        case PWMState_OFF:
            fsm.set_pwm_off();
            break;
        case PWMState_ON:
            fsm.set_pwm_on(set_pwm.duty_cycle);
            break;
        default:
            // NOT ALLOWED
            break;
        }
    } else if (in.which_message == HighToLow_allow_power_on_tag) {
        fsm.allow_power_on(in.message.allow_power_on);
    } else if (in.which_message == HighToLow_enable_tag) {
        fsm.enable();
    } else if (in.which_message == HighToLow_disable_tag) {
        fsm.disable();
    } else if (in.which_message == HighToLow_heartbeat_tag) {
        DebugP_log("Received a heartbeat from the CPU\r\n");
    }
}

void push_event(const iec61851::Event& event, RPMsg& link) {
    auto convert = [](const iec61851::Event& event) {
        using ET = iec61851::Event;
        switch (event) {
        case ET::CarPluggedIn:
            return IEC61851Event_CAR_PLUGGED_IN;
        case ET::CarRequestedPower:
            return IEC61851Event_CAR_REQUESTED_POWER;
        case ET::CarRequestedStopPower:
            return IEC61851Event_CAR_REQUESTED_STOP_POWER;
        case ET::CarUnplugged:
            return IEC61851Event_CAR_UNPLUGGED;
        case ET::EF_To_BCD:
            return IEC61851Event_EF_TO_BCD;
        case ET::Error_DF:
            return IEC61851Event_ERROR_DF;
        case ET::Error_E:
            return IEC61851Event_ERROR_E;
        case ET::Error_OverCurrent:
            return IEC61851Event_ERROR_OVER_CURRENT;
        case ET::Error_RCD:
            return IEC61851Event_ERROR_RCD;
        case ET::Error_Relais:
            return IEC61851Event_ERROR_RELAIS;
        case ET::Error_VentilationNotAvailable:
            return IEC61851Event_ERROR_VENTILATION_NOT_AVAILABLE;
        case ET::EvseReplugFinished:
            return IEC61851Event_EVSE_REPLUG_FINISHED;
        case ET::EvseReplugStarted:
            return IEC61851Event_EVSE_REPLUG_STARTED;
        case ET::BCD_To_EF:
            return IEC61851Event_BCD_TO_EF;
        case ET::PermanentFault:
            return IEC61851Event_PERMANENT_FAULT;
        case ET::PowerOff:
            return IEC61851Event_POWER_OFF;
        case ET::PowerOn:
            return IEC61851Event_POWER_ON;
        default:
            return IEC61851Event_PERMANENT_FAULT;
        }
    };

    LowToHigh out;
    out.which_message = LowToHigh_event_tag;
    out.message.event = convert(event);
    link.send_msg(out);
}

void main_task(void* args) {
    DebugP_log("Hello from ti am62x charger firmware!\r\n");

    //
    // initialize necessary classes
    //

    // RPMsg link to high level
    // first wait for RPMsg linux connection
    DebugP_assert(RPMsg::wait_for_linux(10 * 1000));
    RPMsg rpmsg_link;
    DebugP_log("RPMsg link init done ...\r\n");

    // power switch
    PowerSwitch power_switch;

    // adc sampler
    Sampler sampler;

    // control pilot (creates pwm, and needs reference to sampler, in order to trigger adc readout)
    ControlPilot control_pilot(sampler);

    // rcd
    RCD rcd;

    // charger lock
    ChargerLock charger_lock;

    // clock
    iec61851::IClock clock{ClockP_usecToTicks(1000), ClockP_getTicks};

    // hal interface, needed by iec61851 finite state machine
    iec61851::HAL hal{
        rcd, charger_lock, power_switch, control_pilot, clock,
    };

#if 1
    // finite state machine, remote controlled from EVerest
    iec61851::FSM fsm{hal, [&rpmsg_link](const iec61851::Event& ev) { push_event(ev, rpmsg_link); }};
#else
    // This is a very simple charging logic that does not implement all cases
    // of IEC61851-1. It can be used to do basic charging on the microcontroller
    // only. You should not use this, use the remote control version in combination
    // with EVerest above.
    iec61851::FSM fsm{hal, [&hal](const iec61851::Event& ev) {
        float constexpr max_current = 16*0.6/100.;

        if (ev == iec61851::Event::CarPluggedIn) hal.cp.set_pwm(max_current);
        else if (ev == iec61851::Event::CarRequestedPower) hal.power_switch.on(true);
        else if (ev == iec61851::Event::CarRequestedStopPower) hal.power_switch.off();
        else if (ev == iec61851::Event::CarUnplugged) hal.cp.set_pwm(1.);
    }};
    fsm.enable();
#endif

    // start necessary tasks
    TaskHandle_t sampling_task_handle;
    auto status = xTaskCreate(Sampler::sampling_task_trampoline, "sampling_task", SAMPLING_TASK_STACK_SIZE, &sampler,
                              SAMPLING_TASK_PRIORITY, &sampling_task_handle);
    DebugP_assert(status == pdPASS);

    uint32_t last_chore_ts = ClockP_getTicks();

    const uint32_t chore_interval_ticks = ClockP_usecToTicks(CHORE_INTERVAL_MS * 1000);

    while (true) {
        // main loop

        constexpr uint16_t FSM_WAIT_INTERVAL_MS = 50;
        HighToLow input;

        // 1. wait on message with specific timeout
        const auto status = rpmsg_link.get_msg(input, FSM_WAIT_INTERVAL_MS);

        // 2. if message, handle it
        if (status == RPMsgRcvStatus::TIMEOUT) {
            // no new input message
        } else if (status == RPMsgRcvStatus::CORRUPT_MESSAGE) {
            // FIXME (aw): proper error handling?
        } else if (status == RPMsgRcvStatus::OK) {
            // got new input
            handle_incoming_message(input, fsm);
        }

        // 3. run state machine update
        fsm.run();

        // 4. do chore
        uint32_t current_ts = ClockP_getTicks();
        if (chore_interval_ticks < (current_ts - last_chore_ts)) {
            auto cp_signal = sampler.get_latest_cp_signal();
            DebugP_log("CP: hi: %f, low: %f, valid: %d\r\n", cp_signal.high, cp_signal.low, cp_signal.valid);
            //DebugP_log("Debug: hi_trg: %d, low_trg: %d, adcs: %d, sigs: %d, msgs: %d\r\n", ovfls, matchs, adcs, sigs,
            //           messages_received);

            last_chore_ts = current_ts;
        }
    }

    vTaskDelete(NULL);
}
