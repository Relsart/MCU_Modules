#pragma once
#include <stdint.h>
#include "Signal.h"
#include "driver/Interface.h"

/**
 * @brief Software timer
 * @details Works by mainloop timer signal
 */
class SoftTimer : public i_face::I_Timer, public SlotInterface <uint8_t>
{
public:
    /**
     * @brief Constructor
     */
    SoftTimer();

    /**
     * @brief Start the timer
     * @param [in] mode Once or Periodic
     * @param [in] timeMs Time value in milliseconds
     */
    void start(uint32_t timeMs, WorkingMode mode) override;

    /**
     * @brief Stop the timer
     */
    void stop() override;

private:
    SignalTime<uint8_t> m_timeSignal;   // Timer signal

    /**
     * @brief Signal handler
     */
    void run(uint8_t , uint32_t) override;
};
