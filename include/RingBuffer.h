#pragma once
#include <stdint.h>
#include <atomic>

/**
 * @brief Ring buffer class
 * @details For simple bytes data
 */
class RingBuffer
{
private:
    uint8_t* m_buffer;                  // Data buffer
    const uint32_t bufferSize;          // Data buffer size
    std::atomic<uint32_t> rx {0} ;      // Rx index (index of last readed byte)
    std::atomic<uint32_t> tx {0} ;      // Tx index (index of last writed byte)
    std::atomic<bool> full {false};     // Buffer is full. Needed in case, when rx == tx : full or empty?

public:
    /**
     * @brief Constructor
     * @param [in] size buffer size in bytes
     */
    RingBuffer(uint32_t size);

    /**
     * @brief Destructor
     */
    ~RingBuffer();

    /**
     * @brief Get received (unreaded) byte size
     * @return Lenght of data in buffer
     */
    uint32_t unreadedSize();

    /**
     * @brief Write data to buffer
     * @param [in] data data
     * @param [in] len data lenght
     * @return Result of operation
     */
    bool write(const void* data, uint32_t len);

    /**
     * @brief Write one byte to buffer
     * @return Result of operation
     */
    bool write(uint8_t data);

    /**
     * @brief Traceless read data from buffer (without rx index offset)
     * @return Size of readed data
     */
    uint32_t tracelessRead(uint8_t* data, uint32_t readSize);

    /**
     * @brief Manual rx index offset
     * @param [in] bias Offset lenght
     */
    void moveRx(uint32_t bias);

    /**
     * @brief Read data from buffer
     * @param [out] data data pointer
     * @param [in] readSize max data lenght
     * @return Size of readed data
     */
    uint32_t read(uint8_t* data, uint32_t readSize);

    /**
     * @brief Read one byte from buffer
     * @param [out] data pointer for byte saving
     * @return Result of operation
     */
    bool read (uint8_t& data);
};

/**
 * @brief Ring buffer for structured messages
 * @tparam Type of data units
 */
template<typename Type>
class MessageRingBuffer
{
private:
    RingBuffer m_buffer;
    const uint32_t m_messageSize;

public:
    MessageRingBuffer(uint32_t size) : m_buffer(size * sizeof(Type)), m_messageSize(sizeof(Type))
    {}

    /**
     * @brief Get count of received (unreaded) messages
     */
    uint32_t unreadedSize()
    {
        return m_buffer.unreadedSize() / m_messageSize;
    }

    /**
     * @brief Write message to the buffer
     */
    bool write(const Type& message)
    {
        return m_buffer.write(&message, m_messageSize);
    }

    /**
     * @brief Read message from the buffer
     * @param [out] message link for messsage saving
     */
    bool read(Type& message)
    {
        if (m_buffer.unreadedSize() < m_messageSize)
            return false;
        return m_buffer.read(reinterpret_cast<uint8_t*>(&message), m_messageSize) == m_messageSize;
    }
};
