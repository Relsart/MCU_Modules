#pragma once

#include <stdint.h>
#include "EventLoop.h"

/**
 * @brief Signal template, processed by timeout
 * @tparam Signal data type
 */
template<typename Type>
class SignalTime : public SignalInterface
{
private:
    Type m_data;                    // Data buffer
    uint32_t m_quantity;            // Data size (in data units)
    uint64_t m_nextTimestamp = 0;   // Time of next invoking
    uint64_t m_lastTimestamp = 0;   // Time of last invoking
    uint32_t m_period;              // Handling period

    /**
     * @brief Invoke all slots, subscribed to this signal
     */
    void runSlots() override
    {
        uint64_t thisTime = getMsTime();
        if (thisTime >= m_nextTimestamp || m_lastTimestamp > thisTime)
        {
            m_lastTimestamp = thisTime;
            m_nextTimestamp = thisTime + m_period;
            auto slot = reinterpret_cast<SlotInterface<Type>*>(getNextSlot());
            while (slot)
            {
                slot->run(m_data, m_quantity);
                slot = reinterpret_cast<SlotInterface<Type>*>(getNextSlot(reinterpret_cast<uint32_t*>(slot)));
            }
        }
    }

public:
    /**
     * @brief Constrictor
     * @param [in] time Invoking period (in milliseconds)
     */
    SignalTime(uint32_t time) : m_period(time)
    {
        if (m_period != 0)
        {
            EventLoop::getInstance().connect(this);
            m_nextTimestamp = getMsTime() + m_period;   // Get next activation time
        }
    }

    /**
     * @brief Connect signal
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
        if (!isConnected())
            EventLoop::getInstance().disconnect(this);
    }

    /**
     * @brief Set handling period (in milliseconds)
     */
    void setPeriod(uint32_t time)
    {
        if (time != 0 && m_period == 0)
            EventLoop::getInstance().connect(this);
        else if (time == 0 && m_period != 0)
            EventLoop::getInstance().disconnect(this);

        m_period = time;
        m_nextTimestamp = (m_period != 0) ? getMsTime() + m_period : 0;
    }

    /**
     * @brief Process handling of singal
     * @param [in] _data Data
     * @param [in] _quantity Data size (in data units)
     */
    void activ(Type data, uint32_t quantity = 1)
    {
        m_data = data;
        m_quantity = quantity;
        runSlots();
    } 
};

/**
 * @brief Signal template, handling in main loop cycle (EventLoop)
 * @tparam Signal data type
 */
template<typename Type>
class SignalMainLoop : public SignalInterface
{
private:
    Type m_data;           // Data buffer
    uint32_t m_quantity;   // Data size (in data units)

    #ifdef WITH_ETL
    etl::atomic<bool> m_activated {false};
    #else
    std::atomic_bool m_activated {false};
    #endif

    /**
     * @brief Invoke all slots, subscribed to this signal
     */
    void runSlots () override
    {
        if (m_activated.load())
        {
            auto cmd = reinterpret_cast<SlotInterface<Type>*>(getNextSlot());
            while (cmd)
            {
                cmd->run(m_data, m_quantity);
                cmd = reinterpret_cast<SlotInterface<Type>*>(getNextSlot(reinterpret_cast<uint32_t*>(cmd)));
            }
            m_activated.store(false);
        }
    }

public:  
    /**
     * @brief Constructor
     */
    SignalMainLoop()
    {
        EventLoop::getInstance().connect(this);
    }

    /**
     * @brief Destructor
     * @details All local signals must disconnect themselves from the Eventloop before death
     */
    virtual ~SignalMainLoop()
    {
        EventLoop::getInstance().disconnect(this);
    }

    /**
     * @brief Connect signal
     * @param [in] slot slot pointer
     */
    void connect(SlotInterface<Type>* h)
    {
        addSlot(reinterpret_cast<uint32_t*>(h));
    }

    /**
     * @brief Process handling of singal
     * @param [in] data Data
     * @param [in] quantity Data size (in data units)
     */
    void activ(Type data, uint32_t quantity = 1)
    {
        m_data = data;
        m_quantity = quantity;
        m_activated.store(true);
    } 
};
