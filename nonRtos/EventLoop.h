#pragma once
#include <stdint.h>
#include "SignalInterface.h"

#ifdef WITH_ETL
#include "etl/list.h"
#include "etl/atomic.h"
#else
#include <list>
#include <atomic>
#endif

#ifndef UNIT_TEST
#include "driver/SysTimer.h"
#else
#include "Mocks.h"
#endif

#define getMsTime() driver::getMsTicks()    // Todo: to external callback!

/**
 * @brief   Main infinite loop events handler.
 * @details It invokes triggered offline-signals and time-signals. 
 */
class EventLoop 
{
private:
    /**
     * @brief Singleton
     */
    EventLoop(){}
    EventLoop(EventLoop&) = delete;

    #ifdef WITH_ETL // List of coinnected time- and offline-signals
    etl::list<SignalInterface*, 500> m_listSignal;    
    #else
    std::list <SignalInterface*> m_listSignal;
    #endif

public:
    static EventLoop& getInstance();

    /**
     * @brief Signal registration
     * @details invokes in time- and offline-signals constructors
     */
    void connect(SignalInterface* signal);

    /**
     * @brief Signal removal (disconnection)
     * @details Used only for TimeSignals
     */
    void disconnect(SignalInterface* signal);

    /**
     * @brief Infinite loop event handler
     */
    void loop();
};
