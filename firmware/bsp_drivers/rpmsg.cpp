#include "../bsp_drivers/rpmsg.hpp"

#include <string.h>

#include <kernel/dpl/ClockP.h>

#include <nanopb/pb_decode.h>
#include <nanopb/pb_encode.h>

#define RPMSG_CHAR_DEV_ENDPOINT_ID  (14U)
#define RPMSG_CHAR_DEV_CHANNEL_NAME "rpmsg_chrdev"

uint32_t messages_received = 0;

RPMsg::RPMsg() {
    // trying to announce rpmsg endpoint
    DebugP_assert(SystemP_SUCCESS ==
                  RPMessage_announce(CSL_CORE_ID_A53SS0_0, RPMSG_CHAR_DEV_ENDPOINT_ID, RPMSG_CHAR_DEV_CHANNEL_NAME));

    pending_msgs_semaphore = xSemaphoreCreateCountingStatic(MAX_MESSAGES_BACKLOG, 0, &pending_msgs_semaphore_buffer);

    // constructing message object for receiving data
    RPMessage_CreateParams rpmsg_char_params;
    RPMessage_CreateParams_init(&rpmsg_char_params);
    rpmsg_char_params.localEndPt = RPMSG_CHAR_DEV_ENDPOINT_ID;
    rpmsg_char_params.recvNotifyCallback = RPMsg::receive_notify_cb;
    rpmsg_char_params.recvNotifyCallbackArgs = this;

    DebugP_assert(SystemP_SUCCESS == RPMessage_construct(&rpmsg_ep, &rpmsg_char_params));
}

// FIXME (aw): in which context will this function be called?
void RPMsg::receive_notify_cb(RPMessage_Object* obj, void* args) {
    auto ctx = static_cast<RPMsg*>(args);

    BaseType_t wakeup;
    auto status = xSemaphoreGiveFromISR(ctx->pending_msgs_semaphore, &wakeup);

    if (status != pdTRUE) {
        // FIXME (aw): too much messages, what to do?
        // probably just read it out?  But need to stay in sync with message count?
    }
    messages_received++;

    portYIELD_FROM_ISR(wakeup);
}

RPMsgRcvStatus RPMsg::get_msg(HighToLow& msg, uint32_t timeout_ms) {
    if (pdTRUE != xSemaphoreTake(pending_msgs_semaphore, pdMS_TO_TICKS(timeout_ms))) {
        return RPMsgRcvStatus::TIMEOUT;
    }

    uint8_t msg_buffer[HighToLow_size];
    uint16_t msg_buffer_len = sizeof(msg_buffer);

    uint16_t remote_core_id_;
    uint32_t remote_end_point_;
    auto status = RPMessage_recv(&rpmsg_ep, msg_buffer, &msg_buffer_len, &remote_core_id_, &remote_end_point_, 0);

    // FIXME (aw): what to do here, technically this couldn't fail
    DebugP_assert(status == SystemP_SUCCESS);

    if (!remote_peer_detected) {
        remote_core_id = remote_core_id_;
        remote_end_point = remote_end_point_;
        remote_peer_detected = true;
    } else {
        // could check for correct endpoint ...
    }

    auto istream = pb_istream_from_buffer(msg_buffer, msg_buffer_len);

    return (pb_decode(&istream, HighToLow_fields, &msg)) ? RPMsgRcvStatus::OK : RPMsgRcvStatus::CORRUPT_MESSAGE;
}

bool RPMsg::send_msg(LowToHigh& msg, uint32_t timeout_ms) {
    if (!remote_peer_detected) {
        // FIXME (aw): what to do here
        return false;
    }

    uint8_t encode_buf[LowToHigh_size];

    pb_ostream_t ostream = pb_ostream_from_buffer(encode_buf, sizeof(encode_buf));
    pb_encode(&ostream, LowToHigh_fields, &msg);

    // FIXME (aw): for now we're using a fixed timeout
    const auto timeout_ticks = ClockP_usecToTicks(timeout_ms * 1000);
    return SystemP_SUCCESS == RPMessage_send(encode_buf, ostream.bytes_written, CSL_CORE_ID_A53SS0_0, remote_end_point,
                                             RPMessage_getLocalEndPt(&rpmsg_ep), timeout_ticks);
}

bool RPMsg::wait_for_linux(uint32_t timeout_ms) {
    auto system_ticks = ClockP_usecToTicks(timeout_ms * 1000);
    return SystemP_SUCCESS == RPMessage_waitForLinuxReady(system_ticks);
}
