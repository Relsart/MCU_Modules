#include "RingBuffer.h"
#include <malloc.h>
#include <string.h>

RingBuffer::RingBuffer(uint32_t size) : bufferSize(size)
{
    m_buffer = reinterpret_cast<uint8_t*>(malloc(bufferSize));
};

RingBuffer::~RingBuffer()
{
    free(m_buffer);
}

uint32_t RingBuffer::unreadedSize()
{
    if (!m_buffer || bufferSize == 0)
        return 0;

    // Get rx/tx indexes:
    auto tx_ = tx.load();
    auto rx_ = rx.load();

    if (tx_ > rx_)                              // Tx after Rx (not all data was read)
        return tx_ - rx_;                       // Take size of unreaded data
    else if (tx == rx)                          // Tx == Rx can be in 2 cases:
        return (full.load()) ? bufferSize : 0;  // buffer full or empty
    else                                        // Tx before Rx: the data went in a ring, through the end of the array to the beginning
        return (bufferSize - rx_) + tx_;        // Get the summ of two parts
}

bool RingBuffer::write(const void* data, uint32_t len)
{
    if (!data || !m_buffer || (bufferSize == 0) || (len == 0) || (len > bufferSize))
        return false;

    auto tx_ = tx.load();
    if ((tx_ + len) > bufferSize)     // Fragment doesn't fit in end part- go to beginning (ring)
    {
        const uint32_t rest = bufferSize - tx_;                                      // Free bytes in end
        memcpy(&m_buffer[tx_], data, rest);                                           // Write first part to the end
        memcpy(&m_buffer[0], &reinterpret_cast<const uint8_t*>(data)[rest], len - rest);    // write second part to the beginning
    }
    else    // Fit to end part
    {
        memcpy(&m_buffer[tx_], data, len);    // Simple copy
    }
    
    tx_ += len;         // Increment
    tx_ %= bufferSize;  // Recalc, if there was a ring (from end to beginning)

    if (tx_ == rx.load())
        full.store(true);
    else 
        full.store(false);
    tx.store(tx_);
    return true;
}

bool RingBuffer::write(uint8_t data)
{
    if (!m_buffer || (bufferSize == 0))
        return false;

    auto tx_ = tx.load();
    m_buffer[tx_++] = data;
    tx_ %= bufferSize;

    if (tx_ == rx.load())
        full.store(true);
    else 
        full.store(false);

    tx.store(tx_);
    return true;
}

uint32_t RingBuffer::tracelessRead(uint8_t* data, uint32_t readSize)
{
    if (!data || !m_buffer || bufferSize == 0)
        return 0;

    uint32_t usag = unreadedSize();
    if (usag == 0)
        return 0;   // No data to read

    if (readSize > usag)
        readSize = usag;

    auto rx_ = rx.load();

    if ((rx_ + readSize) > bufferSize)    // Ring read
    {
        const uint32_t rest = bufferSize - rx_;
        memcpy(data, &m_buffer[rx_], rest);       // Read from end
        memcpy(&reinterpret_cast<uint8_t*>(data)[rest], &m_buffer[0], readSize - rest);   // Read from beginning
    }
    else                            // Simple copy
    {
        memcpy(data, &m_buffer[rx_], readSize);
    }
    // No index offset!
    return readSize;
}

void RingBuffer::moveRx(uint32_t bias)
{
    if (bias == 0)
        return;
    auto rx_ = rx.load();
    rx_ += bias;
    rx_ %= bufferSize;
    full.store(false);
    rx.store(rx_);
}

uint32_t RingBuffer::read(uint8_t* data, uint32_t readSize)
{
    uint32_t len = tracelessRead(data, readSize);
    moveRx(len);
    return len;
}

bool RingBuffer::read(uint8_t& data)
{
    if (!m_buffer || bufferSize == 0)
        return false;

    uint32_t usag = unreadedSize();
    if (usag == 0)
        return false;

    auto rx_ = rx.load();
    data = m_buffer[rx_++];
    rx_ %= bufferSize;

    full.store(false);
    rx.store(rx_);
    return true;
}
