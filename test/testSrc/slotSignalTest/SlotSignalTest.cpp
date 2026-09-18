#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <vector>
#include <string>
#include "Signal.h"
#include "EventLoop.h"

using namespace ::testing;

namespace driver {
static uint64_t FakeMsTick = 0;
static uint64_t FakeMsTicksPeriod = 100;
uint64_t getMsTicks()
{
    FakeMsTick += FakeMsTicksPeriod;
    return FakeMsTick;
}
}   // namespace driver

/**
 * @brief Slot testing class
 * @details With saving received data
 */
template<typename Type>
class TestSlot : public SlotInterface<Type>
{
public:
    std::vector<Type> savedData;
    void run(Type var, uint32_t) override
    {
        savedData.push_back(var);
    }
};

/* Behaviour of one signal, connected to several slots */
TEST(SlotSignalTest, SignalConnectTest_0)
{
    TestSlot<uint8_t> slot1;
    TestSlot<uint8_t> slot2;
    Signal<uint8_t> signal;

    // Each of 2 connected slots has received the data:
    signal.connect(&slot1);
    signal.connect(&slot2);
    signal.activ(123);
    ASSERT_EQ(signal.totalSlotsInUsage(), 2);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_TRUE(signal.isConnectedTo(&slot1));
    ASSERT_TRUE(signal.isConnectedTo(&slot2));
    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    ASSERT_EQ(slot1.savedData.at(0), 123);
    ASSERT_EQ(slot2.savedData.at(0), 123);
    slot1.savedData.clear();
    slot2.savedData.clear();

    // One slot was disconnected:
    signal.disconnect(&slot1);
    signal.activ(5);
    ASSERT_EQ(signal.totalSlotsInUsage(), 1);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_FALSE(signal.isConnectedTo(&slot1));
    ASSERT_TRUE(signal.isConnectedTo(&slot2));
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.at(0), 5);
    slot1.savedData.clear();
    slot2.savedData.clear();

    // Both slots were disconnected:
    signal.disconnect(&slot2);
    signal.activ(1);
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
    ASSERT_FALSE(signal.isConnected());
    ASSERT_FALSE(signal.isConnectedTo(&slot1));
    ASSERT_FALSE(signal.isConnectedTo(&slot2));
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Another 2 slots were connected:
    TestSlot<uint8_t> slot3;
    TestSlot<uint8_t> slot4;
    signal.connect(&slot3);
    signal.connect(&slot4);
    signal.activ(31);
    ASSERT_EQ(signal.totalSlotsInUsage(), 2);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_FALSE(signal.isConnectedTo(&slot1));
    ASSERT_FALSE(signal.isConnectedTo(&slot2));
    ASSERT_TRUE(signal.isConnectedTo(&slot3));
    ASSERT_TRUE(signal.isConnectedTo(&slot4));
    ASSERT_EQ(slot3.savedData.size(), 1);
    ASSERT_EQ(slot4.savedData.size(), 1);
    ASSERT_EQ(slot3.savedData.at(0), 31);
    ASSERT_EQ(slot4.savedData.at(0), 31);
    slot3.savedData.clear();
    slot4.savedData.clear();

    // Disconnect one slot (not first in slots list):
    signal.disconnect(&slot4);
    signal.activ(11);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_FALSE(signal.isConnectedTo(&slot1));
    ASSERT_FALSE(signal.isConnectedTo(&slot2));
    ASSERT_TRUE(signal.isConnectedTo(&slot3));
    ASSERT_FALSE(signal.isConnectedTo(&slot4));
    ASSERT_EQ(slot3.savedData.size(), 1);
    ASSERT_EQ(slot3.savedData.at(0), 11);
    slot3.savedData.clear();

    // All signals were disconnected:
    signal.disconnectAll();
    ASSERT_FALSE(signal.isConnected());
    ASSERT_FALSE(signal.isConnectedTo(&slot1));
    ASSERT_FALSE(signal.isConnectedTo(&slot2));
    ASSERT_FALSE(signal.isConnectedTo(&slot3));
    ASSERT_FALSE(signal.isConnectedTo(&slot4));
    signal.activ(15);   // And nothing was happend:
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);
    ASSERT_EQ(slot3.savedData.size(), 0);
    ASSERT_EQ(slot4.savedData.size(), 0);

    // Connect 4 slots:
    signal.connect(&slot1);
    signal.connect(&slot2);
    signal.connect(&slot3);
    signal.connect(&slot4);
    ASSERT_EQ(signal.totalSlotsInUsage(), 4);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_TRUE(signal.isConnectedTo(&slot1));
    ASSERT_TRUE(signal.isConnectedTo(&slot2));
    ASSERT_TRUE(signal.isConnectedTo(&slot3));
    ASSERT_TRUE(signal.isConnectedTo(&slot4));
    signal.activ(44);
    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    ASSERT_EQ(slot3.savedData.size(), 1);
    ASSERT_EQ(slot4.savedData.size(), 1);
    ASSERT_EQ(slot1.savedData.at(0), 44);
    ASSERT_EQ(slot2.savedData.at(0), 44);
    ASSERT_EQ(slot3.savedData.at(0), 44);
    ASSERT_EQ(slot4.savedData.at(0), 44);
    slot1.savedData.clear();
    slot2.savedData.clear();
    slot3.savedData.clear();
    slot4.savedData.clear();

    // Repeatedly connects the same slot:
    signal.disconnectAll();
    signal.connect(&slot1);
    signal.connect(&slot1); // Again!
    ASSERT_TRUE(signal.isConnected());
    ASSERT_TRUE(signal.isConnectedTo(&slot1));
    ASSERT_FALSE(signal.isConnectedTo(&slot2));
    ASSERT_FALSE(signal.isConnectedTo(&slot3));
    ASSERT_FALSE(signal.isConnectedTo(&slot4));
    signal.activ(9);
    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot1.savedData.at(0), 9);
    
    // Disconnect all:
    signal.disconnectAll();
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
}

/* Behaviour of one slot, subscribed to several signals */
TEST(SlotSignalTest, SignalConnectTest_1)
{
    TestSlot<uint8_t> slot;
    Signal<uint8_t> signal1;
    Signal<uint8_t> signal2;

    // Two signals send data to one slot:
    signal1.connect(&slot);
    signal2.connect(&slot);
    ASSERT_EQ(signal1.totalSlotsInUsage(), 2);

    signal1.activ(12);
    signal2.activ(34);
    ASSERT_EQ(slot.savedData.size(), 2);
    ASSERT_EQ(slot.savedData.at(0), 12);
    ASSERT_EQ(slot.savedData.at(1), 34);
    slot.savedData.clear();

    // One slot was disconnected:
    signal1.disconnect(&slot);
    signal2.activ(45);
    ASSERT_EQ(slot.savedData.size(), 1);
    ASSERT_EQ(slot.savedData.at(0), 45);

    // Disconnect all:
    signal1.disconnectAll();
    signal2.disconnectAll();
    ASSERT_EQ(signal1.totalSlotsInUsage(), 0);
}

/* User-defined data type for slot */
TEST(SlotSignalTest, SignalConnectTest_2)
{
    struct UserData
    {
        uint8_t age;
        uint32_t height;
        float salary;
    };
    TestSlot<UserData> slot1;
    TestSlot<UserData> slot2;
    Signal<UserData> signal;

    // Two signals send data to one slot:
    signal.connect(&slot1);
    signal.connect(&slot2);
    ASSERT_EQ(signal.totalSlotsInUsage(), 2);
    UserData data
    {
        .age = 18, 
        .height = 175, 
        .salary = 192.95
    };

    signal.activ(data);

    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    ASSERT_EQ(slot1.savedData.at(0).age, 18);
    ASSERT_EQ(slot1.savedData.at(0).height, 175);
    ASSERT_FLOAT_EQ(slot1.savedData.at(0).salary, 192.95);
    ASSERT_EQ(slot2.savedData.at(0).age, 18);
    ASSERT_EQ(slot2.savedData.at(0).height, 175);
    ASSERT_FLOAT_EQ(slot2.savedData.at(0).salary, 192.95);

    // Disconnect all:
    signal.disconnectAll();
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
}

/* User-defined data type for slot by address */
TEST(SlotSignalTest, SignalConnectTest_3)
{
    struct UserData
    {
        uint8_t age;
        uint32_t height;
        float salary;
    };
    TestSlot<const UserData*> slot1;
    TestSlot<const UserData*> slot2;
    Signal<const UserData*> signal;

    // Two signals send data to one slot:
    signal.connect(&slot1);
    signal.connect(&slot2);
    ASSERT_EQ(signal.totalSlotsInUsage(), 2);
    UserData data
    {
        .age = 18, 
        .height = 175, 
        .salary = 192.95
    };

    signal.activ(&data);

    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    ASSERT_EQ(slot1.savedData.at(0)->age, 18);
    ASSERT_EQ(slot1.savedData.at(0)->height, 175);
    ASSERT_FLOAT_EQ(slot1.savedData.at(0)->salary, 192.95);
    ASSERT_EQ(slot2.savedData.at(0)->age, 18);
    ASSERT_EQ(slot2.savedData.at(0)->height, 175);
    ASSERT_FLOAT_EQ(slot2.savedData.at(0)->salary, 192.95);

    // Disconnect all:
    signal.disconnectAll();
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
}

/* Behaviour of signal, received bad requests */
TEST(SlotSignalTest, BadRequests)
{
    TestSlot<uint8_t> slot1;
    TestSlot<uint8_t> slot2;
    Signal<uint8_t> signal;

    // No slot address:
    signal.connect(nullptr);
    ASSERT_FALSE(signal.isConnected()); 
    ASSERT_FALSE(signal.isConnectedTo(nullptr));
    signal.disconnect(nullptr);
    signal.disconnectAll(); // It happened nothing bad
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
}

/* Testing offline signals: */
TEST(SlotSignalTest, OfflineHandlingMode)
{
    TestSlot<uint8_t> slot1;
    TestSlot<uint8_t> slot2;
    SignalMainLoop<uint8_t> signal;

    signal.connect(&slot1);
    signal.connect(&slot2);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_EQ(signal.totalSlotsInUsage(), 2);
    signal.activ(123);
    // No slot activations yet:
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Enter in Main Loop (one iteration for test mode):
    EventLoop::getInstance().loop();

    // The data has received:
    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    ASSERT_EQ(slot1.savedData.at(0), 123);
    ASSERT_EQ(slot2.savedData.at(0), 123);
    slot1.savedData.clear();
    slot2.savedData.clear();

    EventLoop::getInstance().loop();
    // No more data:
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Disconnect the signal:
    signal.disconnectAll();
    signal.activ(123);

    EventLoop::getInstance().loop();
    // No data:
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);
    EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
}

/* Testing timer signals: */
TEST(SlotSignalTest, TimerSignalsTest)
{
    TestSlot<uint8_t> slot1;
    TestSlot<uint8_t> slot2;
    SignalTime<uint8_t> signal(1000);   // Period - one second

    signal.connect(&slot1);
    signal.connect(&slot2);
    ASSERT_TRUE(signal.isConnected());
    ASSERT_EQ(signal.totalSlotsInUsage(), 2);

    // No data yet:
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Waiting for 10 cycles (fake time period == 100ms)
    for (int i = 0; i < 9; i++)
        EventLoop::getInstance().loop();

    // No data yet:
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Time up! It was handler invoking (input data doesn't matter):
    EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);

    // Waiting for next 10 cycles
    for (int i = 0; i < 9; i++)
        EventLoop::getInstance().loop();

    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    
    // Time up:
    EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 2);
    ASSERT_EQ(slot2.savedData.size(), 2);
    slot1.savedData.clear();
    slot2.savedData.clear();

    // Disable timer and no invokings:
    signal.setPeriod(0);
    for (int i = 0; i < 30; i++)
        EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Enable timer for another period:
    signal.setPeriod(500);
    for (int i = 0; i < 4; i++)
        EventLoop::getInstance().loop();    // Not now yet...
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);
    EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 1);
    ASSERT_EQ(slot2.savedData.size(), 1);
    slot1.savedData.clear();
    slot2.savedData.clear();

    // Disconnect one slot:
    signal.disconnect(&slot1);
    ASSERT_EQ(signal.totalSlotsInUsage(), 1);
    for (int i = 0; i < 4; i++)
        EventLoop::getInstance().loop();    // Not now yet...
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);
    EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 1);
    slot1.savedData.clear();
    slot2.savedData.clear();

    // Disconnect last slot:
    signal.disconnect(&slot2);
    for (int i = 0; i < 40; i++)
        EventLoop::getInstance().loop();
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);

    // Manual invoking has no result for timed signal:
    signal.activ(1);
    ASSERT_EQ(slot1.savedData.size(), 0);
    ASSERT_EQ(slot2.savedData.size(), 0);
    ASSERT_EQ(signal.totalSlotsInUsage(), 0);
}

/* Testing the Slots Storage overflowing case: */
TEST(SlotSignalTest, OverflowTest)
{
    const uint32_t maxSlotsNum = 500;   // The limit of total Slots
    TestSlot<uint8_t> slots[maxSlotsNum];
    Signal<uint8_t> signal;

    // Connect and activate all possible slots:
    for (int i = 0; i < maxSlotsNum; i++)
        signal.connect(&slots[i]);

    signal.activ(5);
    for (int i = 0; i < maxSlotsNum; i++)
    {
        ASSERT_EQ(slots[i].savedData.size(), 1);
        ASSERT_EQ(slots[i].savedData.at(0), 5);
        slots[i].savedData.clear();
    }
    ASSERT_EQ(signal.totalSlotsInUsage(), maxSlotsNum);

    // One more slot:
    TestSlot<uint8_t> oneMoreSlot;
    signal.connect(&oneMoreSlot);
    ASSERT_EQ(signal.totalSlotsInUsage(), maxSlotsNum);
    signal.activ(7);

    // These are worked:
    for (int i = 0; i < maxSlotsNum; i++)
    {
        ASSERT_EQ(slots[i].savedData.size(), 1);
        ASSERT_EQ(slots[i].savedData.at(0), 7);
        slots[i].savedData.clear();
    }
    // And excess slot is no:
    ASSERT_EQ(oneMoreSlot.savedData.size(), 0);

    // Now free one place and re-connect:
    signal.disconnect(&slots[10]);
    ASSERT_EQ(signal.totalSlotsInUsage(), maxSlotsNum - 1);
    signal.connect(&oneMoreSlot);
    ASSERT_EQ(signal.totalSlotsInUsage(), maxSlotsNum);
    signal.activ(8);
    for (int i = 0; i < maxSlotsNum; i++)
    {
        if (i == 10)    // This slot was released
        {
            ASSERT_EQ(slots[i].savedData.size(), 0);
        }
        else
        {
            ASSERT_EQ(slots[i].savedData.size(), 1);
            ASSERT_EQ(slots[i].savedData.at(0), 8);
            slots[i].savedData.clear();
        }
    }
    ASSERT_EQ(oneMoreSlot.savedData.size(), 1);
    ASSERT_EQ(oneMoreSlot.savedData.at(0), 8);
}
