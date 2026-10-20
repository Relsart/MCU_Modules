#pragma once
#include <stdint.h>
#include <string.h>
#include <etl/atomic.h>
#include <etl/string.h>
#include <etl/type_traits.h>
#include "Interface.h"  // <- From project headers!

/**
 * @brief Ring buffer class
 * @details For simple bytes data
 */
template <size_t BufferSize>
class RingBuffer : public i_face::I_RingBuffer
{
static_assert(BufferSize >= 2, "The minimal RingBuffer size is 2!");
static_assert((BufferSize & (BufferSize - 1)) == 0, "RingBuffer size must be Power of 2!");
private:
    uint8_t m_buffer[BufferSize]{0};
    etl::atomic<size_t> m_writeHead{0}; 
    etl::atomic<size_t> m_readTail{0};
    static constexpr size_t IndexMask = BufferSize - 1; // Optimisation mask instead of heavy % operator

public:
    /**
     * @brief Constructor, 
     */
    RingBuffer() = default;
    RingBuffer(const RingBuffer&) = delete;   // Disable inefficient buffer copying!
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * @brief Get received (occupied) byte size
     * @return Lenght of data in buffer
     */
    size_t getOccupiedSize() const override
    {
        return (m_writeHead.load(etl::memory_order_relaxed) - m_readTail.load(etl::memory_order_relaxed)) & IndexMask;
    }

    /**
     * @brief Get free (available) byte size
     * @details 1 less than the actual free size, it reserved as "busy" flag
     */
    size_t getFreeSize() const override
    {
        return IndexMask - getOccupiedSize();
    }

    /**
     * @brief Write data to buffer
     * @param [in] data data
     * @param [in] len data lenght
     * @return Result of operation
     */
    bool push(const uint8_t* data, size_t size) override
    {
        if (!data || size == 0 || size > getFreeSize()) return false;

        size_t head = m_writeHead.load(etl::memory_order_relaxed);
        size_t sizeToEnd = BufferSize - head;
        if (size <= sizeToEnd)
        {
            memcpy(&m_buffer[head], data, size);
        }
        else
        {
            memcpy(&m_buffer[head], data, sizeToEnd);
            memcpy(m_buffer, &data[sizeToEnd], size - sizeToEnd);
        }
        m_writeHead.store((head + size) & IndexMask, etl::memory_order_release); // Optimised with bitmask
        return true;
    }

    /**
     * @brief Write one byte to buffer
     * @return Result of operation
     */
    bool push(uint8_t data) override
    {
        if (getFreeSize() == 0)
            return false;   // No place!

        size_t head = m_writeHead.load(etl::memory_order_relaxed);
        m_buffer[head] = data;
        m_writeHead.store((head + 1) & IndexMask, etl::memory_order_release);
        return true;
    }

    /**
     * @brief Write data to buffer with overwriting old (unreaded) data if it's not enough free space
     * @return Is successfull?
     */
    bool pushOverwrite(const uint8_t* data, size_t size) override
    {
        if (!data || size == 0) return false;

        const size_t maxCapacity = BufferSize - 1;
        size_t head = m_writeHead.load(etl::memory_order_relaxed);

        // Scenario 1: Was received more data than can fit (the buffer will fill up completely)
        if (size >= maxCapacity) 
        {
            data += (size - maxCapacity); // Move the pointer to take as much as we can
            size = maxCapacity;

            size_t sizeToEnd = BufferSize - head;
            if (size <= sizeToEnd)
            {
                memcpy(&m_buffer[head], data, size);
            }
            else
            {
                memcpy(&m_buffer[head], data, sizeToEnd);
                memcpy(m_buffer, &data[sizeToEnd], size - sizeToEnd);
            }

            // Update the head, and put the tail right behind it (the buffer is full)
            size_t nextHead = (head + size) & IndexMask;
            size_t nextTail = (nextHead + 1) & IndexMask;
            // Important: First move the tail so that the reader does not go into the incorrect zone!
            m_writeHead.store(nextHead, etl::memory_order_release); 
            m_readTail.store(nextTail, etl::memory_order_relaxed);
            return true;
        }

        // Scenario 2: Data is buffered, but it may need to displace the old ones
        size_t freeSize = getFreeSize();
        size_t nextTail = m_readTail.load(etl::memory_order_relaxed);
        
        if (size > freeSize)
        {
            size_t overflow = size - freeSize;
            nextTail = (nextTail + overflow) & IndexMask;
        }

        // Standart data copying:
        size_t sizeToEnd = BufferSize - head;
        if (size <= sizeToEnd)
        {
            memcpy(&m_buffer[head], data, size);
        }
        else
        {
            memcpy(&m_buffer[head], data, sizeToEnd);
            memcpy(m_buffer, &data[sizeToEnd], size - sizeToEnd);
        }

        // First we update the tail (if there was an overflow), then we move the head:
        if (size > freeSize)
        {
            m_readTail.store(nextTail, etl::memory_order_relaxed);
        }
        
        m_writeHead.store((head + size) & IndexMask, etl::memory_order_release);
        return true;
    }

    /**
     * @brief Read data from buffer
     * @param [in] data data
     * @param [in] maxSize maximal size of data to get
     * @return Size of readed data
     */
    size_t pop(uint8_t* dest, size_t maxSize) override
    {
        if (!dest || maxSize == 0)
            return 0;

        size_t storedSize = getOccupiedSize();
        if (storedSize == 0)
            return 0;

        size_t bytesToRead = (storedSize < maxSize) ? storedSize : maxSize;
        size_t tail = m_readTail.load(etl::memory_order_relaxed);
        size_t sizeToEnd = BufferSize - tail;
        if (bytesToRead <= sizeToEnd)
        {
            memcpy(dest, &m_buffer[tail], bytesToRead);
        }
        else
        {
            memcpy(dest, &m_buffer[tail], sizeToEnd);
            memcpy(&dest[sizeToEnd], m_buffer, bytesToRead - sizeToEnd);
        }
        m_readTail.store((tail + bytesToRead) & IndexMask, etl::memory_order_release);
        return bytesToRead; 
    }

    #ifdef WITH_ETL
    /**
     * @brief Read data from buffer into etl::string container
     * @return Size of readed data
     */
    size_t pop(etl::istring* dest) override
    {
        if (!dest)
            return 0;

        /* How many bytes is it possible to write to the string: */
        size_t stringFreeSpace = dest->max_size() - dest->size();
        if (stringFreeSpace == 0)
            return 0;

        size_t storedSize = getOccupiedSize();
        if (storedSize == 0)
            return 0;

        /* Finally get the reading size: */
        size_t bytesToRead = (storedSize < stringFreeSpace) ? storedSize : stringFreeSpace;
        size_t tail = m_readTail.load(etl::memory_order_relaxed);
        size_t sizeToEnd = BufferSize - tail;
        if (bytesToRead <= sizeToEnd)
        {
            dest->append(reinterpret_cast<const char*>(&m_buffer[tail]), bytesToRead);
        }
        else
        {
            dest->append(reinterpret_cast<const char*>(&m_buffer[tail]), sizeToEnd);
            dest->append(reinterpret_cast<const char*>(m_buffer), bytesToRead - sizeToEnd);
        }
        m_readTail.store((tail + bytesToRead) & IndexMask, etl::memory_order_release);
        return bytesToRead; 
    }
    #endif

    /**
     * @brief Read one byte from the buffer
     * @param [out] data link for byte saving
     * @return Result of operation
     */
    bool pop(uint8_t& data) override
    {
        if (!getOccupiedSize())
            return false;

        size_t tail = m_readTail.load(etl::memory_order_relaxed);
        data = m_buffer[tail];
        m_readTail.store((tail + 1) & IndexMask, etl::memory_order_release);
        return true;
    }

    /**
     * @brief Read data from buffer without read pointer moving
     * @param [in] data data
     * @param [in] maxSize maximal size of data to get
     * @return Size of readed data
     */
    size_t peek(uint8_t* dest, size_t maxSize) override
    {
        if (!dest || maxSize == 0)
            return 0;

        size_t storedSize = getOccupiedSize();
        if (storedSize == 0)
            return 0;
        else if (storedSize < maxSize)
            maxSize = storedSize;

        size_t tail = m_readTail.load(etl::memory_order_relaxed); 
        size_t sizeToEnd = BufferSize - tail;
        if (maxSize <= sizeToEnd)
        {
            memcpy(dest, &m_buffer[tail], maxSize);
        }
        else
        {
            memcpy(dest, &m_buffer[tail], sizeToEnd);
            memcpy(&dest[sizeToEnd], m_buffer, maxSize - sizeToEnd);
        }
        return maxSize; 
    }

    /**
     * @brief Read one byte from buffer without read pointer moving
     * @return Result of operation
     */
    bool peek(uint8_t& data) override
    {
        if (!getOccupiedSize())
            return false;

        data = m_buffer[m_readTail.load(etl::memory_order_relaxed)];
        return true;
    }

    /**
     * @brief Drop (remove) data from buffer by moving its read pointer
     * @param [in] size number of bytes for skipping
     */
    void drop(size_t size) override
    {
        size_t storedSize = getOccupiedSize();
        size_t bytesToSkip = (storedSize < size) ? storedSize : size;
        size_t tail = m_readTail.load(etl::memory_order_relaxed);
        m_readTail.store((tail + bytesToSkip) & IndexMask, etl::memory_order_release);
    }

    /**
     * @brief Clear the buffer
     */
    void clear() override
    {
        m_readTail.store(0, etl::memory_order_relaxed);
        m_writeHead.store(0, etl::memory_order_relaxed);
    }
};

/**
 * @brief Ring buffer for structured messages
 * @tparam Type of data units
 */
template <typename Type, size_t BufferSize>
class MessageRingBuffer : public i_face::I_MessageRingBuffer<Type>
{
    static_assert(BufferSize >= 2, "The minimal MessageRingBuffer size is 2!");
    static_assert((BufferSize & (BufferSize - 1)) == 0, "MessageRingBuffer size must be Power of 2!");
    static_assert(etl::is_trivially_copyable<Type>::value, "Type must be trivially copyable for memcpy!");

private:
    Type m_buffer[BufferSize];  // Fized size objects array
    etl::atomic<size_t> m_writeHead{0}; 
    etl::atomic<size_t> m_readTail{0};
    static constexpr size_t IndexMask = BufferSize - 1;

public:
    MessageRingBuffer() = default;
    MessageRingBuffer(const MessageRingBuffer&) = delete;   
    MessageRingBuffer& operator=(const MessageRingBuffer&) = delete;

    /**
     * @brief Get received (occupied) byte size
     * @return Number of objects in the buffer
     */
    size_t getOccupiedSize() const override
    {
        return (m_writeHead.load(etl::memory_order_relaxed) - m_readTail.load(etl::memory_order_relaxed)) & IndexMask;
    }

    /**
     * @brief Get free (available) size in objects units
     * @details 1 less than the actual free size, it reserved as "busy" flag
     */
    size_t getFreeSize() const override
    {
        return IndexMask - getOccupiedSize();
    }

    /**
     * @brief Write one object to buffer
     * @return Result of operation
     */
    bool push(const Type& message) override
    {
        if (getFreeSize() == 0)
            return false;

        size_t head = m_writeHead.load(etl::memory_order_relaxed);
        m_buffer[head] = message; 
        m_writeHead.store((head + 1) & IndexMask, etl::memory_order_release);
        return true;
    }

    /**
     * @brief Write data to buffer with overwriting old (unreaded) object if it's not enough free space
     * @param [in] message object for saving
     * @return Is successfull?
     */
    bool pushOverwrite(const Type& message) override
    {
        size_t head = m_writeHead.load(etl::memory_order_relaxed);
        size_t freeSize = getFreeSize();
        
        if (freeSize == 0)
        {
            // Если места нет, атомарно сдвигаем хвост вперед на 1 объект
            size_t tail = m_readTail.load(etl::memory_order_relaxed);
            m_readTail.store((tail + 1) & IndexMask, etl::memory_order_relaxed);
        }

        m_buffer[head] = message;
        m_writeHead.store((head + 1) & IndexMask, etl::memory_order_release);
        return true;
    }

    /**
     * @brief Read one object from the buffer
     * @param [out] dest link for object saving
     * @return Result of operation
     */
    bool pop(Type& dest) override
    {
        if (getOccupiedSize() == 0)
            return false;

        size_t tail = m_readTail.load(etl::memory_order_relaxed);
        dest = m_buffer[tail];
        m_readTail.store((tail + 1) & IndexMask, etl::memory_order_release);
        return true;
    }

    /**
     * @brief Read one object from buffer without read pointer moving
     * @param [out] dest link for object saving
     * @return Result of operation
     */
    bool peek(Type& dest) const override
    {
        if (getOccupiedSize() == 0)
            return false;

        dest = m_buffer[m_readTail.load(etl::memory_order_relaxed)];
        return true;
    }

    /**
     * @brief Drop (remove) data from buffer by moving its read pointer
     * @param [in] size number of objects
     */
    void drop(size_t count) override
    {
        if (count == 0) return;

        size_t storedSize = getOccupiedSize();
        size_t toSkip = (storedSize < count) ? storedSize : count;
        
        if (toSkip == 0) return;

        size_t tail = m_readTail.load(etl::memory_order_relaxed);
        m_readTail.store((tail + toSkip) & IndexMask, etl::memory_order_release);
    }

    /**
     * @brief Clear the buffer
     */
    void clear() override
    {
        m_readTail.store(0, etl::memory_order_relaxed);
        m_writeHead.store(0, etl::memory_order_relaxed);
    }
};
