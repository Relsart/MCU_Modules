#pragma once
#include <stdint.h>
#include <stdio.h>
#include "terminal/LogGroups.h"

#ifdef WITH_ETL
#include "etl/delegate.h"
#else
#include <functional>
#endif

/**
 * @brief Log level
 */
enum LogLevel 
{
    Error,
    Warn,
    Info
};

class Log 
{
public:
    #ifdef WITH_ETL
    /**
     * @brief Callback for sending data (to UART for example..)
     * @param [in] Data
     * @param [in] Size (in bytes)
     */
    using DataSender = etl::delegate<void(void*, uint32_t)>;

    /**
     * @brief Callback for waiting port to be free
     * @param [in] Timeout (in milliseconds)
     * @return True- is Free; False- Unavailable 
     */
    using IsPortAvailable = etl::delegate<bool(uint32_t)>;

    /**
     * @brief Callback for system time getting
     */
    using GetSysTime = etl::delegate<uint64_t(void)>;
    #else
    using DataSender = std::function<void(void*, uint32_t)>;
    using IsPortAvailable = std::function<bool(uint32_t)>;
    using GetSysTime = std::function<uint64_t(void)>;
    #endif

    /**
     * @brief Notation modifier: for output integer values in hex or decimal
     * @note Decimal is default at the beginning of each log
     */
    enum class Base
    {
        Hex,
        Dec
    };

    /**
     * @brief Precision modifier: for output floating values
     * @note 3 digits is default at the beginning of each log
     */
    struct Precision
    {
        uint8_t value;
        Precision(uint8_t _prec){ value = _prec; }
    };

    /**
     * @brief Once specifier: for frequent log messages filtering
     * @note Time value in milliseconds
     */
    struct OnceSpec
    {
        explicit OnceSpec(float timeout = 0, const char* fname = 0, int lineno = 0)
            : m_timeoutMs(timeout), m_file(fname), m_line(lineno) {}
        uint32_t m_timeoutMs = 0;
        const char* m_file = 0;
        int m_line = 0;
    };
    #define Once(T) OnceSpec(T, __FILE__, __LINE__)

private:
    /**
     * @brief Colour schemes
     */
    struct Color
    {
        static const char* Default;
        static const char* Red;
        static const char* Green;
        static const char* Yellow;
        static const char* Blue;
    };

    static DataSender m_sender;                 // Data sender function (to UART)
    static IsPortAvailable m_portIsBusy;       // Function for checking (and waiting) if target receiver is free
    static GetSysTime m_getSysTimeMs;           // System timer (in milliseconds) getter
    static char* line;                          // String-buffer
    static const uint32_t m_BufferMaxSize;      // Full size of output buffer
    static bool busy;                           // Log busy flag
    static bool logEnable;                      // Logging enable (common flags for all grupped logs)
    bool m_thisLogEnable = false;               // This log enabled (groups/level checked)
    static uint32_t failcounter;                // Counter of missed (not sended) messages (for debug diagnostic)
    static constexpr auto titleInf = "[INF] ";  // Headlines of Info, Warning and Error message
    static constexpr auto titleWarn = "[WRN] ";
    static constexpr auto titleErr = "[ERR] ";
    static constexpr uint8_t bitsInByte = 8;
    static uint8_t maskModule[lmEnd/bitsInByte+1];  // Bitmask for log modules masking

    uint32_t m_buffEndIndex = 0;    // Currrent message size (index of first free byte)
    Base intBase;
    uint8_t floatPrecision;
    uint64_t m_onceHash = 0;        // Hash sum for Once spec
    uint32_t m_onceTimeSetMs;       // Once Spec time interval set in milliseconds

    /**
     * @brief Converts float (double) values to string and adds to log buffer
     * @param [in] val Value
     * @todo Precision to args?
     */
    template <typename Type>
    void putRealToBuffer(Type val);

    /**
     * @brief Converts integer values to string and adds to log buffer
     * @param [in] val Value
     * @param [in] isSigned Is signed flag
     */
    template <typename Type>
    void putIntToBuffer(Type val, bool isSigned);

    /**
     * @brief Converts 64-bit integer values to string and adds to log buffer
     * @param [in] val Value
     * @param [in] isSigned Is signed flag
     */
    template <typename Type>
    void putInt64ToBuffer(Type val, bool isSigned);
    
    /**
     * @brief Puts substring to message buffer
     */
    void putStringToBuffer(const char* str);

    /**
     * @brief Checking: is message group allowed
     */
    static bool isGroupAllowed (LogGroup module);

    /**
     * @brief Checking for log creation conditions: enabling, uart stream available
     */
    bool checkLogAvailable ();

    /**
     * @brief Checking for Once specifier timeout condition
     * @param [in] hash unique log id
     * @param [out] skippedMsgCount link to skipped messages counter variable
     * @return True: time elapsed, print log message (+ skipped counter) 
     */
    bool checkOnceTimeout(const uint64_t& hash, uint32_t& skippedMsgCount);

public:
    static constexpr auto endl = "\r\n";  // New line
  
    /**
     * @brief Constructors, destructor
     */
    Log ();
    Log (LogGroup module, LogLevel level);
    ~Log ();

    /**
     * @brief Subscribe for data sender callback (z.b. UART)
     */
    static void setTransmitter(DataSender callback);

    /**
     * @brief Subscribe for sender availability checking callback (z.b. UART DMA)
     */
    static void setPortChecking(IsPortAvailable callback);

    /**
     * @brief Subscribe for system time getter (z.b. driver::getMsTicks())
     */
    static void setSysTimeGetter(GetSysTime callback);

    /**
     * @brief Operator overloadings
     */
    Log& operator<< (const char* str);
    Log& operator<< (char ch);
    Log& operator<< (float val);
    Log& operator<< (double val);
    Log& operator<< (int val);
    Log& operator<< (uint8_t val);
    Log& operator<< (uint16_t val);
    Log& operator<< (uint32_t val);
    Log& operator<< (uint64_t val);
    Log& operator<< (int8_t val);
    Log& operator<< (int16_t val);
    Log& operator<< (int32_t val);
    Log& operator<< (int64_t val);
    Log& operator<< (Base basemod);
    Log& operator<< (Precision prec);
    Log& operator<< (const OnceSpec& once);

    /**
     * @brief Logs Groups switching on/off
     */
    static void groupOn(LogGroup module);
    static void groupOff(LogGroup module);

    /**
     * @brief Logs switching on/off
     */
    static void enable();
    static void disable();
};
