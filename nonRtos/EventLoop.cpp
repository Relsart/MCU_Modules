#include "EventLoop.h"

EventLoop& EventLoop::getInstance()
{
    static EventLoop self;
    return self;
}

void EventLoop::connect(SignalInterface* signal)
{
    m_listSignal.push_back(signal);
}

void EventLoop::disconnect(SignalInterface* signal)
{
    m_listSignal.remove(signal);
}

void EventLoop::loop()
{
    #ifdef UNIT_TEST
    static const bool loopCondition = false;  // For unit test- only one iteration
    #else
    static const bool loopCondition = true;
    #endif

    do
    {
        SignalInterface* signal = nullptr;
        for (auto iter = m_listSignal.begin(); iter !=m_listSignal.end(); iter++)
        {
            signal = *iter;
            if (signal != nullptr)
            {
                signal->runSlots();
            }
        }
        // Here may be WDT timer resetting..
    } while (loopCondition);
}