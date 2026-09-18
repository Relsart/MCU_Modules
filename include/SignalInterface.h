#pragma once
#include <stdint.h>

/**
 * @brief Slot interface template
 * @tparam Data type of slot for exchanging with signal
 */
template<typename Type>
class SlotInterface
{
public:
    /**
     * @brief  Signal data handler
     * @param [in] data Incoming data
     * @param [in] quantity Data size (in Type units)
     */
    virtual void run (Type data, uint32_t quantity) = 0;

    virtual ~SlotInterface () {}
};

/**
 * @brief Signals base interface
 */
class SignalInterface
{
public:
    friend class EventLoop;
    virtual ~SignalInterface () {}

    /**
     * @brief Slots list element struct
     */
    struct SlotInfo
    {
        uint32_t* slot = nullptr; // Current slot address
        SlotInfo* next = nullptr; // Next slot address
    };

    /**
     * @brief Is signals connect to any slot
     */
    bool isConnected();

    /**
     * @brief Is signals connect to target slot (against dublication)
     * @param [in] slot Traget slot addresss
     * @return True == this signal has connected to target slot, else False
     */
    bool isConnectedTo(void* slot);

    /**
     * @brief Disconnect al connected slots
     */
    void disconnectAll();

    /**
     * @brief Get total count of connected slots (in usage)
     * @details Diagnostic feature
     */
    static uint16_t totalSlotsInUsage();

private:
    SlotInfo* m_slotList = nullptr;     // First slot address (list of addresses)
    SlotInfo* m_lastGetSlot = nullptr;  // Last handled slot address
          
protected:
    /**
     * @brief Connect stot to signal (invokes in connect() method).
     */
    void addSlot (uint32_t* slot);

    /**
     * @brief Disconnect slot from signal
     */
    void delSlot (uint32_t* slot);

    /**
     * @brief Get next slot address
     * @param [in] slot Current slot address. If NULL- it will return first slot address
     * @return Next slot address or NULL
     */
    uint32_t* getNextSlot (uint32_t* slot = nullptr);

    /**
     * @brief Handle all connected signals
     * @details Slots are handled in the order they are added
     */
    virtual void runSlots () = 0;
};
