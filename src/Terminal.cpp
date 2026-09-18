#include "Terminal.h"
#include "Loger.h"
#include <cstring>

namespace console {

Terminal::Terminal()
{
    #ifdef WITH_RTOS
    m_newDataQueue = xQueueCreate(1, sizeof(RingBuffer*));
    xTaskCreate(handleTask, "Terminal", task::StackSizes[task::Tasks::Terminal], this, task::TerminalTaskPriority, &task::tasksHandlers[task::Tasks::Terminal]);
    #endif
}

Terminal& Terminal::getInstance ()
{
    static Terminal self;
    return self;
}

void Terminal::run (RingBuffer* incomingData, uint32_t)
{
    #ifdef WITH_RTOS
    /* --------------- I S R --------------- */

    if (incomingData && m_newDataQueue)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        BaseType_t xStatus = xQueueSendToBackFromISR(m_newDataQueue, &incomingData, &xHigherPriorityTaskWoken);
        if (xStatus == pdPASS)
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    #else
    /* For Non-RTOS version it is Main Loop signal handler */
    read(incomingData);
    #endif
}

void Terminal::read(RingBuffer* incomingData)
{
    uint8_t data;
    /* Read incoming data from the ring buffer */
    while (incomingData->read(data))
    {
        switch (data)
        {
        case enter:     // 'Enter' symbol: parse and consume the string
        {
            parse();
            Log() << Log::endl << ">> ";
            m_endLine = 0;
        }
        break;
        case backspace: // TODO: make
            break;
        case cancel:    // (Ctrl + C) Loging disable, switch to Command line mode
        {
            Log() << Log::endl << ">> ";
            Log::disable();
            break;
        }
        default:        // Other symbols: put them to buffer and output to the console:
        {
            // Symbol is valid (ASCII) and there is place in the buffer:
            if ((data >= 0x20) && (data <= 0x7E) && (m_endLine < m_LastCharPos))
            {
                m_bufferStr[m_endLine++] = data;
                Log() << (char)data;
            }
        }
        break;
        }
    }
}

void Terminal::parse ()
{
    if (m_endLine == 0)
        return;

    m_bufferStr[m_endLine] = '\0';      // Set NULL to the end of the string
    char* commandName = m_bufferStr;    // First substring is a command name
    uint32_t argc = 0;                  // Arguments counter
    // Replace spaces with zeros and count the number of arguments:
    for (uint32_t i = 0; i < m_endLine; i++)
    {
        if (m_bufferStr[i] == ' ')
        {
            m_bufferStr[i] = '\0';
            argc++;
        }
    }
    // Checking command name and invoking the appropriate command:
    if (strcmp (commandName, "help") == 0)
    {
        // TODO: make help
    }
    else
    {
        for (uint32_t i = 0; i < sizeof (listCmd)/sizeof (listCmd[0]); ++i)
        {
            if (strcmp (listCmd[i]->getName(), commandName) == 0)
            {
                Log::disable();
                setArguments();
                listCmd[i]->exec(argc, m_arg);
                return;
            }
        }
        Log () << Log::endl << "Invalid cmd";
    }
}


void Terminal::setArguments ()
{
    uint32_t col = 0;
    for (uint32_t i = 0; i < m_endLine; i++)
    {
        if (m_bufferStr[i] == '\0')
        {
            m_arg[col++] = &m_bufferStr[i+1];
            if (col >= m_MaxArgCounter)
                return;
        }
    }
    for (uint32_t i = col; i < m_MaxArgCounter; i++)
    {
        m_arg[i] = nullptr;
    }
}

#ifdef WITH_RTOS
void Terminal::handleTask(void* pvParameters)
{
    Terminal* instance = static_cast<Terminal*>(pvParameters);
    if (instance)
    {
        RingBuffer* rxBufferPtr;
        for (;;)
        {
            xQueueReceive(instance->m_newDataQueue, &rxBufferPtr, portMAX_DELAY);
            if (rxBufferPtr)
                instance->read(rxBufferPtr);
        }
    }
    vTaskDelete(nullptr); 
}
#endif

}   // namespace console
