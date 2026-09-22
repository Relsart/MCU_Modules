#pragma once
#include "RingBuffer.h"
#include "Signal.h"
#include "TermCmdInterface.h"
#include "terminal/TerminalInit.h"
#ifdef WITH_RTOS
#include <FreeRTOS.h>
#include <queue.h>
#include "task/TaskData.h"
#endif
/**
 * @brief The handler for terminal commands. The command is called by name, further can be command line arguments.
 * 
 * @details Slot for UART driver signal
 * 
 * @details Creating new command:
 *          - Create a command class inheriting the CmdInterface. Set the command id (name) and description in the constructor.
 *          - Implement the exec function (uint32_t argc, char** arg), which accepts arguments and executes the command.
 *          - Add a pointer to the command object in the Terminal::listCmd[] array Terminal::listCmd[]
 */
namespace console {

class Terminal : public SlotInterface <RingBuffer*>
{
private:
    /**
     * @brief Singleton
     */
    Terminal();
    Terminal(Terminal&) = delete;

    /**
     * @brief Control symbols:
     */
    static constexpr uint8_t enter = 0x0D;      // Enter (/r) symbol
    static constexpr uint8_t backspace = 0x7F;  // Backspace symbol
    static constexpr uint8_t cancel = 0x03;     // <Ctrl+C> symbol
    CmdInterface* currentCmd = nullptr;         // Еhe command being executed

    static constexpr uint16_t m_BufferSize = 200;               // Size of the buffer string
    static constexpr uint16_t m_LastCharPos = m_BufferSize - 1; // Last char index (in the buffer string)
    static constexpr uint8_t m_MaxArgCounter = 10;

    char m_bufferStr[m_BufferSize];     // Terminal line buffer
    char* m_arg[10];                    // Command arguments buffer
    uint8_t m_endLine = 0;              // End Of String index

    /**
     * @brief Working with terminal command line:
     */
    void read(RingBuffer* buffer);
    void parse();          // Parsing and handling input string (after 'Enter' was sent)
    void setArguments();   // Setting command line arguments

    #ifdef WITH_RTOS
    QueueHandle_t m_newDataQueue;   // Queue: handle new input data
    static void handleTask(void*);
    #endif

public:
    /**
     * @brief Get class instance
     */
    static Terminal& getInstance();

    /**
     * @brief Handle data from UART
     */
    void run (RingBuffer* buffer, uint32_t quantity) override;
};

}   // namespace console
