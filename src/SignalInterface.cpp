#include "SignalInterface.h"

/**
 * @brief Manager of Slots Addresses
 * @todo Add diagnostic info & getters
 */
class SlotStorage
{
public:
    typedef SignalInterface::SlotInfo Slot;

private:
    static constexpr uint16_t m_storageSize = 500;
    Slot m_slotsArray[m_storageSize];       // Storage of all slots addresses (TODO: dynamic?)
    uint32_t m_lastSlotIndex = 0;           // index of first empty address in slotArray (for overflow control)
    Slot* m_releasedSlots = nullptr;        // List of released (disconnected) slots addresses. For re-using, against slotArray overflowing
    uint16_t slotsInUse = 0;
    
public:
    /**
     * @brief Release slot - put it to Released Slots list for re-using
     */
    void releaseSlot(Slot* slot)
    {
        if (!slot)
            return;
        slot->next = m_releasedSlots;           // Push disconnected slot at the beginning of the released slots list
        m_releasedSlots = slot;
        if (slotsInUse > 0) slotsInUse--;       // Update diagnostic info
    }

    /**
     * @brief Get new (or released) slot address
     */
    Slot* getSlot()
    {
        Slot* newSlot = nullptr;
        if (m_releasedSlots)   // Is there released slots addresses?
        {
            newSlot = m_releasedSlots;                  // Take one of them
            m_releasedSlots = m_releasedSlots->next;    // Shift list
        }
        else    // No released slots in list - take new
        {
            if (m_lastSlotIndex < m_storageSize)
                newSlot = &m_slotsArray[m_lastSlotIndex++];    // Take new element
        }
        
        if (newSlot) slotsInUse++; // Update diagnostic info
        return newSlot;
    }

    uint16_t getSlotsInUsageCount()
    {
        return slotsInUse;
    }

} slotStorage;


uint16_t SignalInterface::totalSlotsInUsage()
{
    return slotStorage.getSlotsInUsageCount();
}

bool SignalInterface::isConnected()
{
    return (m_slotList != nullptr);
}

bool SignalInterface::isConnectedTo(void* targetSlot)
{
    if (!targetSlot)
        return false;

    SlotInfo* currSlot = m_slotList;  // Current slot in cycle
    while (currSlot)
    {
        if (currSlot->slot == (uint32_t*)targetSlot)
            return true;
        currSlot = currSlot->next;
    }
    return false;
}

void SignalInterface::addSlot (uint32_t* slotAddr)
{
    if (!slotAddr)
        return;
    
    if (isConnectedTo(slotAddr))
        return;   // This signal has allready connected to this slot!

    SlotInfo* newElement = slotStorage.getSlot();
    if (!newElement)
        return;

    newElement->slot = slotAddr;
    newElement->next = nullptr;

    if (!m_slotList) // It's first slot, connecting to signal
    {
        m_slotList = newElement;
    }
    else    // Not first: add to the end of the list
    {
        SlotInfo* tempSlot = m_slotList;

        while (tempSlot->next)
        {
            tempSlot = tempSlot->next;
        }
        tempSlot->next = newElement;
    }
}

void SignalInterface::delSlot (uint32_t* slot)
{
    if (!m_slotList || !slot)
        return;

    SlotInfo* currSlot = m_slotList;  // Current slot in cycle
    SlotInfo* prevSlot = nullptr;   // Last cycle slot

    while (currSlot)
    {
        if (currSlot->slot == slot)
        {
            if (!prevSlot)
                m_slotList = currSlot->next;          // If was first element- move next to first position
            else
                prevSlot->next = currSlot->next;    // Else connect previous and next elements

            slotStorage.releaseSlot(currSlot);      // Save released address for re-using
            break;
        }
        prevSlot = currSlot;
        currSlot = currSlot->next;
    }
}

void SignalInterface::disconnectAll()
{
    if (!m_slotList) // No connected slots
        return;

    SlotInfo* currSlot = m_slotList; // Current slot in cycle
    while (currSlot)
    {
        m_slotList = currSlot->next;
        slotStorage.releaseSlot(currSlot);   // Save released address for re-using
        currSlot = m_slotList;
    }
}

uint32_t* SignalInterface::getNextSlot (uint32_t* slot)
{
    if (!m_slotList)
        return nullptr;

    if (!slot) 
    {
        m_lastGetSlot = m_slotList;    // If nullptr in args- return first slot
        return m_slotList->slot;
    }

    // In arguments last handled slot - return next or null (if it was last)
    if (slot == m_lastGetSlot->slot)
    {
        m_lastGetSlot = m_lastGetSlot->next;
        return (m_lastGetSlot) ? m_lastGetSlot->slot : nullptr;
    }

    m_lastGetSlot = m_slotList;
    while (m_lastGetSlot->slot != slot)
    {
        m_lastGetSlot = m_lastGetSlot->next;
        if (!m_lastGetSlot)
            return nullptr;
    }

    return m_lastGetSlot->slot;
}
