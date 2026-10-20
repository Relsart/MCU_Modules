#pragma once
#include <stdint.h>
#include <string.h>
#include "Loger.h"
#include "Interface.h"

/**
 * @brief Data parsing interface
 * @tparam Derived class, implementing functions getPackSize, checkPackCrc and handleMessage
 * @tparam Size of internal parser buffer in bytes
 */
template <typename Derived, size_t BuffSize>
class ParserInterface
{
protected:
    /**
     * @brief Derived link-getters (for better code readability)
     */
    __attribute__((always_inline)) inline Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }
    __attribute__((always_inline)) inline const Derived& derived() const
    {
        return *static_cast<const Derived*>(this);
    }

public:
    /**
     * @brief Constructor
     * @param [in] magic pointer to the Preamble (magic) value
     * @param [in] magicSize Size of Preamble in bytes
     * @param [in] headerSize Size of package header (without magic), can be 0 if there is no header in the package
     */
    ParserInterface(const uint8_t* magic, uint32_t magicSize, uint32_t headerSize) : 
        m_MagicPtr(magic), 
        m_MagicSize(magicSize), 
        m_HeaderSize(headerSize)
    {}

private:
    uint8_t m_buffer[BuffSize]{0};  // Working buffer

    /**
     * @brief Parser state
     */
    enum class ParserState : uint8_t
    {
        SearchForPreamble,  // Searching for Magic (beginning of new package)
        ReadHeader,         // Reading package header
        ReadPayload         // Reading payload data and checking the checksum
    };
    ParserState m_parserState = ParserState::SearchForPreamble;
    const uint8_t* m_MagicPtr;      // Magic (preamble) value
    const uint32_t m_MagicSize;     // Preamble size
    const uint32_t m_HeaderSize;    // Header size
    uint32_t m_packSize = 0;        // Current package calculated size

protected:

    /**
     * @brief Start parsing cycle process
     * @details Must be implemented as a deferred handler, do not invoke it from ISR!
     * @param [in] rbuf data receiver ring buffer link
     */
    void parseNewData(i_face::I_RingBuffer& rbuf)
    {
        while (rbuf.getOccupiedSize() > 0)
        {
            switch (m_parserState)
            {
            case ParserState::SearchForPreamble:
            {
                if (rbuf.getOccupiedSize() < m_MagicSize)
                    return; // Preamble hasn't been fully get yet

                // Peek and check preamble:
                rbuf.peek(m_buffer, m_MagicSize);
                if (memcmp(m_buffer, m_MagicPtr, m_MagicSize) == 0)
                    m_parserState = ParserState::ReadHeader;
                else
                    rbuf.drop(1);   

                break;
            }
            case ParserState::ReadHeader:
            {
                size_t packHeadSize = m_MagicSize + m_HeaderSize;   // Size of package beginning: preamble + header
                if (rbuf.getOccupiedSize() < packHeadSize)
                    return;  // Header hasn't been fully get yet

                // Get header to the main buffer (without dropping it from the ringbuffer):
                if (m_HeaderSize > 0)
                    rbuf.peek(m_buffer, packHeadSize);

                // Invoke the derived class method for package size calculating: 
                m_packSize = derived().getPackSize(&m_buffer[m_MagicSize], m_HeaderSize);
                if (m_packSize == 0 || m_packSize > BuffSize)
                {
                    // Header failed- move by 1 byte and search for the new package (preamble)
                    rbuf.drop(1);
                    m_parserState = ParserState::SearchForPreamble;
                } 
                else
                {
                    m_parserState = ParserState::ReadPayload;
                }
                break;
            }
            case ParserState::ReadPayload:
            {
                // m_packSize includes all package size: preamble, header and the payload:
                if (rbuf.getOccupiedSize() < m_packSize)
                    return; // Package hasn't been fully get yet

                // Read the entire packet into the working buffer:
                rbuf.pop(m_buffer, m_packSize);

                if (derived().checkPackCrc(m_buffer, m_packSize))
                    derived().handleMessage(m_buffer, m_packSize);
                else
                    Log(lmSystem, Error) << "Parse error: checksumm failed!";

                m_parserState = ParserState::SearchForPreamble;
                break;
            }
            default:
                break;
            }
        }
    }
};
