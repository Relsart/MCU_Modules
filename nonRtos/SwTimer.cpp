#include "SwTimer.h"

SoftTimer::SoftTimer() : m_timeSignal(0)
{
    m_timeSignal.connect(this);
}

void SoftTimer::start(uint32_t timeMs, WorkingMode mode)
{
    m_mode = mode;
    m_timeSignal.setPeriod(timeMs);
}

void SoftTimer::stop()
{
    m_timeSignal.setPeriod(0);
}

void SoftTimer::run(uint8_t, uint32_t)
{
    if (m_mode == WorkingMode::Once)
        m_timeSignal.setPeriod(0);
    if (m_callback)
        m_callback();
}
