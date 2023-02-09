#ifndef DRIVERS_RPMSG_HPP
#define DRIVERS_RPMSG_HPP

#include "FreeRTOS.h"
#include "semphr.h"

#include <drivers/ipc_rpmsg.h>

#include <high_to_low.pb.h>
#include <low_to_high.pb.h>

extern uint32_t messages_received;

static constexpr size_t MAX_MESSAGES_BACKLOG = 16;

enum class RPMsgRcvStatus {
    OK,
    TIMEOUT,
    CORRUPT_MESSAGE
};

class RPMsg {
public:
    RPMsg();

    static bool wait_for_linux(uint32_t timeout_ms);

    RPMsgRcvStatus get_msg(HighToLow& msg, uint32_t timeout_ms);
    bool send_msg(LowToHigh& msg, uint32_t timeout_ms = MSG_SEND_TIMEOUT_MS);

private:
    RPMessage_Object rpmsg_ep;
    static void receive_notify_cb(RPMessage_Object* obj, void* args);

    static constexpr uint32_t MSG_SEND_TIMEOUT_MS = 10;

    bool remote_peer_detected{false};
    uint16_t remote_core_id;
    uint16_t remote_end_point;

    SemaphoreHandle_t pending_msgs_semaphore;
    StaticSemaphore_t pending_msgs_semaphore_buffer;
};

#endif // DRIVERS_RPMSG_HPP
