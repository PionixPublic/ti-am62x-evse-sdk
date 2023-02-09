#ifndef DRIVERS_CHARGER_LOCK_HPP
#define DRIVERS_CHARGER_LOCK_HPP

#include <iec61851_hal.hpp>

class ChargerLock : public iec61851::IChargerLock {
public:
    void lock() override{};
    void unlock() override{};
    bool is_locked() override {
        return false;
    }
};

#endif // DRIVERS_CHARGER_LOCK_HPP
