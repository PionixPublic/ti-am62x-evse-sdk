// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "board_support_ACImpl.hpp"

#include <rproc_id.h>
#include <ti_rpmsg_char.h>

#include <high_to_low.pb.h>

#include <nanopb/pb_decode.h>
#include <nanopb/pb_encode.h>

const int MCU_ENDPOINT_NUMBER = 14;
char ENDPOINT_NAME[] = "my_ep_name";
const int MAX_RPMSG_BUFFER_SIZE = 512;

class RPMsgLink {
public:
    using InputCallback = std::function<void(const LowToHigh&)>;
    bool setup_connection();
    bool push_message(const HighToLow& out);
    void receive(InputCallback cb);
    ~RPMsgLink();

private:
    rpmsg_char_dev* rpmsg_ep{nullptr};
    std::mutex push_mtx;
    uint8_t out_buf[MAX_RPMSG_BUFFER_SIZE];
    uint8_t in_buf[MAX_RPMSG_BUFFER_SIZE];
};

bool RPMsgLink::push_message(const HighToLow& out) {
    const std::lock_guard<std::mutex> lck{push_mtx};

    pb_ostream_t ostream = pb_ostream_from_buffer(out_buf, sizeof(out_buf));
    pb_encode(&ostream, HighToLow_fields, &out);
    auto bytes_written = write(rpmsg_ep->fd, out_buf, ostream.bytes_written);
    return (bytes_written == ostream.bytes_written);
}

void RPMsgLink::receive(InputCallback cb) {
    EVLOG_debug << "RPMsgLink receive loop started";
    while (true) {
        auto bytes_read = read(rpmsg_ep->fd, in_buf, MAX_RPMSG_BUFFER_SIZE);

        if (bytes_read <= 0) {
            EVLOG_error << "RPMsgLink input error";
            return;
        }

        auto istream = pb_istream_from_buffer(in_buf, bytes_read);

        LowToHigh in;

        if (!pb_decode(&istream, LowToHigh_fields, &in)) {
            EVLOG_info << "RPMsgLink received corrupt data packet";
            continue;
        }

        cb(in);
    }
}

bool RPMsgLink::setup_connection() {
    auto ret = rpmsg_char_init(nullptr);

    if (ret) {
        EVLOG_critical << "rpmsg_char_init failed";
        return false;
    }

    // using the standard rpmsg_chrdev
    rpmsg_ep = rpmsg_char_open(M4F_MCU0_0, nullptr, MCU_ENDPOINT_NUMBER, ENDPOINT_NAME, 0);

    if (!rpmsg_ep) {
        EVLOG_critical << "rpmsg_char_open failed";
        return false;
    }

    EVLOG_info << "Succesfully open rpmsg_char device with endpoint: " << rpmsg_ep->endpt;

    // send an heartbeat right away
    HighToLow out;
    out.which_message = HighToLow_heartbeat_tag;
    push_message(out);

    return true;
}

RPMsgLink::~RPMsgLink() {
    if (rpmsg_ep) {
        // NOTE: should we care about the return value here
        rpmsg_char_close(rpmsg_ep);
    }
}

namespace module {
namespace board_support {

static RPMsgLink link;

void board_support_ACImpl::init() {
    if (!link.setup_connection()) {
        EVLOG_AND_THROW(Everest::EverestInternalError("Failed to setup RPMsg connection to MCU"));
    }
}

void board_support_ACImpl::ready() {
    std::thread incoming_message_thread(&RPMsgLink::receive, &link,
                                        [this](const LowToHigh& in) { this->handle_low_to_high_message(in); });

    // FIXME (aw): there is currently no proper teardown, so we only detach here
    incoming_message_thread.detach();
}

void board_support_ACImpl::handle_low_to_high_message(const LowToHigh& in) {
    auto convert = [](IEC61851Event event) {
        using ET = types::board_support::Event;
        switch (event) {
        case IEC61851Event_CAR_PLUGGED_IN:
            return ET::CarPluggedIn;
        case IEC61851Event_CAR_REQUESTED_POWER:
            return ET::CarRequestedPower;
        case IEC61851Event_CAR_REQUESTED_STOP_POWER:
            return ET::CarRequestedStopPower;
        case IEC61851Event_CAR_UNPLUGGED:
            return ET::CarUnplugged;
        case IEC61851Event_EF_TO_BCD:
            return ET::EFtoBCD;
        case IEC61851Event_ERROR_DF:
            return ET::ErrorDF;
        case IEC61851Event_ERROR_E:
            return ET::ErrorE;
        case IEC61851Event_ERROR_OVER_CURRENT:
            return ET::ErrorOverCurrent;
        case IEC61851Event_ERROR_RCD:
            return ET::ErrorRCD;
        case IEC61851Event_ERROR_RELAIS:
            return ET::ErrorRelais;
        case IEC61851Event_ERROR_VENTILATION_NOT_AVAILABLE:
            return ET::ErrorVentilationNotAvailable;
        case IEC61851Event_EVSE_REPLUG_FINISHED:
            return ET::EvseReplugFinished;
        case IEC61851Event_EVSE_REPLUG_STARTED:
            return ET::EvseReplugStarted;
        case IEC61851Event_BCD_TO_EF:
            return ET::BCDtoEF;
        case IEC61851Event_PERMANENT_FAULT:
            return ET::PermanentFault;
        case IEC61851Event_POWER_OFF:
            return ET::PowerOff;
        case IEC61851Event_POWER_ON:
            return ET::PowerOn;
        default:
            // FIXME (aw): what should be the proper default behavior?
            return ET::PermanentFault;
        }
    };

    switch (in.which_message) {
    case LowToHigh_event_tag:
        this->publish_event(convert(in.message.event));
        break;
    case LowToHigh_heartbeat_tag:
        EVLOG_debug << "Received heartbeat from MCU";
        break;
    default:
        EVLOG_critical << "Received unknown message via RPMsg";
        break;
    }
}

void board_support_ACImpl::handle_setup(bool& three_phases, bool& has_ventilation, std::string& country_code,
                                        bool& rcd_enabled){
    // FIXME (aw): what can be done here?
    // has_ventilation, country_code and rcd_enabled could be passed on to the M4 here.
    // three_phases is irrelevant as the HW cannot switch between single phase and three phase.
};

types::board_support::HardwareCapabilities board_support_ACImpl::handle_get_hw_capabilities() {
    // In a real world scenario it would be better to query them from the power path hardware,
    // on TIDA we cannot.
    types::board_support::HardwareCapabilities caps;

    caps.max_current_A_import = 16;
    caps.max_current_A_export = 0;
    caps.max_phase_count_import = 3;
    caps.max_phase_count_export = 3;
    caps.min_current_A_import = 6;
    caps.min_current_A_export = 0;
    caps.min_phase_count_import = 3;
    caps.min_phase_count_export = 3;
    caps.supports_changing_phases_during_charging = false;
    return caps;
};

void board_support_ACImpl::handle_enable(bool& value) {
    HighToLow out = HighToLow_init_zero;

    out.which_message = (value) ? HighToLow_enable_tag : HighToLow_disable_tag;
    link.push_message(out);
};

void board_support_ACImpl::handle_pwm_on(double& value) {
    HighToLow out = HighToLow_init_zero;

    out.which_message = HighToLow_set_pwm_tag;
    out.message.set_pwm.duty_cycle = value;
    out.message.set_pwm.state = PWMState_ON;
    link.push_message(out);
};

void board_support_ACImpl::handle_pwm_off() {
    HighToLow out = HighToLow_init_zero;

    out.which_message = HighToLow_set_pwm_tag;
    out.message.set_pwm.state = PWMState_OFF;
    link.push_message(out);
};

void board_support_ACImpl::handle_pwm_F() {
    HighToLow out = HighToLow_init_zero;

    out.which_message = HighToLow_set_pwm_tag;
    out.message.set_pwm.state = PWMState_F;
    link.push_message(out);
};

void board_support_ACImpl::handle_allow_power_on(bool& value) {
    HighToLow out = HighToLow_init_zero;

    out.which_message = HighToLow_allow_power_on_tag;
    out.message.allow_power_on = value;
    link.push_message(out);
};

bool board_support_ACImpl::handle_force_unlock() {
    // locking motor is not connected as of now, so we cannot do anything here.
    return true;
};

void board_support_ACImpl::handle_switch_three_phases_while_charging(bool& value){
    // This is not supported by TIDA hardware
};

void board_support_ACImpl::handle_evse_replug(int& value){
    // This is not supported by TIDA hardware
};

double board_support_ACImpl::handle_read_pp_ampacity() {
    // Due to a HW bug in TIDA rev1 this cannot be implemented
    return 16;
};

} // namespace board_support
} // namespace module
