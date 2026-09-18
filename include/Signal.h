#pragma once

#include <stdint.h>
#include "SignalInterface.h"
#ifndef WITH_RTOS
#include "../nonRtos/LoopSignals.h"
#endif

/**
 * @brief Signal template, handling immediately
 * @tparam Signal data type
 */
template<typename Type>
class Signal : public SignalInterface
{
private:
    Type m_data;            // Data buffer
    uint32_t m_datasize;    // Size of data (in data-type units)

    /**
     * @brief Invoke all slots, subscribed to this signal
     */
    void runSlots() override
    {
        auto cmd = reinterpret_cast<SlotInterface<Type>*>(getNextSlot());
        while (cmd)
        {
            cmd->run(m_data, m_datasize);
            cmd = reinterpret_cast<SlotInterface<Type>*>(getNextSlot(reinterpret_cast<uint32_t*>(cmd)));
        }
    }  

public:
    /**
     * @brief Connect slot
     * @param [in] slot slot pointer
     */
    void connect(SlotInterface<Type>* slot)
    {
        addSlot(reinterpret_cast<uint32_t*>(slot));
    }

    /**
     * @brief Disconnect (remove) slot
     * @param [in] slot slot pointer
     */
    void disconnect(SlotInterface<Type>* slot)
    {
        delSlot(reinterpret_cast<uint32_t*>(slot));
    }

    /**
     * @brief Process handling of singal
     * @param [in] d Data
     * @param [in] q Data size (in data units)
     */
    void activ(Type data, uint32_t datasize = 1)
    {
        m_data = data;
        m_datasize = datasize;
        runSlots();
    }
};
