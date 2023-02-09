#ifndef DRIVERS_RCD_HPP
#define DRIVERS_RCD_HPP

#include <iec61851_hal.hpp>

class RCD : public iec61851::IRCD {
public:
    void enable() override{};
    void disable() override{};
    bool got_fired() override {
        return false;
    }
    void reset() override{};
};

#endif // DRIVERS_RCD_HPP
