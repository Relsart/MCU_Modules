#include "Loger.h"
#include <string.h>
#include "Converters.h"

#ifdef WITH_ETL
#include "etl/map.h"
#else
#include <map>  // TODO: 
#endif

namespace
{

char outstring[300];    // Main log message buffer
typedef uint64_t Hash_t;

// Element of one Once Specifier
struct __attribute__((packed)) OnceSpecCounter
{
    uint64_t lastTimeStamp;      // Timestamp of last invoking
    uint32_t skippedMsgCounter;  // Counter of skipped messages
};

etl::map<Hash_t, OnceSpecCounter, 100> onceSpecStorage;

// Make the unique hashsum (id) for OnceSpec log from its filename address and string number:
inline Hash_t MakeHash(const Log::OnceSpec& spec)
{
    Hash_t hash = reinterpret_cast<uint32_t>(spec.m_file) | static_cast<uint64_t>(spec.m_line) << 32;
    return hash;
}

}   // namespace

const char* Log::Color::Default = "\033[m";
const char* Log::Color::Red = "\033[1;31m";
const char* Log::Color::Green = "\033[1;32m";
const char* Log::Color::Yellow = "\033[1;33m";
const char* Log::Color::Blue = "\033[1;96m";

Log::DataSender Log::m_sender;
Log::IsPortAvailable Log::m_portIsBusy;
Log::GetSysTime Log::m_getSysTimeMs;

char* Log::line = nullptr;
bool Log::busy = false;
uint32_t Log::failcounter = 0;
const uint32_t Log::m_BufferMaxSize = sizeof(outstring);
bool Log::logEnable = true;
uint8_t Log::maskModule[]{0};

template <typename Type>
void Log::putIntToBuffer(Type val, bool isSigned)
{
    char str[50]{0};
    int size = 0;
    if (intBase == Base::Dec)
    {
        if (isSigned)
            size = sprintf(str, "%d", val);
        else
            size = sprintf(str, "%u", val);
    }
    else
    {
        str[0] = '0';   // Add hex preamble
        str[1] = 'x';
        size = sprintf(&str[2], "%02X", val) + 2; // + hex preamble size
    }
    
    if (size > (m_BufferMaxSize - m_buffEndIndex))
        size = m_BufferMaxSize - m_buffEndIndex;   // full message doesn't fit in a line
    memcpy(&line[m_buffEndIndex], str, size);
    m_buffEndIndex += size;
}

template <typename Type>
void Log::putInt64ToBuffer(Type val, bool isSigned)
{
    char str[50]{0};
    int size = 0;
    if (intBase == Base::Dec)
    {
        if (isSigned)
            size = conversions::IntToString(val, str, 10, true);
        else
            size = conversions::UintToString(val, str, 10, true);
    }
    else
    {
        str[0] = '0';   // Add hex preamble
        str[1] = 'x';
        size = conversions::UintToString(val, str, 16, true) + 2; // + hex preamble size
    }
    
    if (size > (m_BufferMaxSize - m_buffEndIndex))
        size = m_BufferMaxSize - m_buffEndIndex;   // full message doesn't fit in a line
    memcpy(&line[m_buffEndIndex], str, size);
    m_buffEndIndex += size;
}

template <typename Type>
void Log::putRealToBuffer(Type val)
{
    char str[50]{0};
    //int size = sprintf(str, "%.*f", floatPrecision, val);
    // ACHTUNG: sprintf can not working correctly with STM32 MCUs, so use the custom solution:
    int size = conversions::FloatToString(val, str, floatPrecision);

    if (size > (m_BufferMaxSize - m_buffEndIndex))
        size = m_BufferMaxSize - m_buffEndIndex;   // full message doesn't fit in a line
    memcpy(&line[m_buffEndIndex], str, size);
    m_buffEndIndex += size;
}

Log::Log()
{
    if (!line || !m_sender || !checkLogAvailable())
        return;

    m_thisLogEnable = true;
    m_buffEndIndex = 0;
    busy = true;
    putStringToBuffer(Log::Color::Default);
}

Log::Log(LogGroup module, LogLevel level)
{
    if (!line || !logEnable || !m_sender)
        return;

    if (!checkLogAvailable() || !isGroupAllowed(module))
        return;

    m_thisLogEnable = true;
    m_buffEndIndex = 0;
    busy = true;

    putStringToBuffer(endl); // Begin from newline
    switch (level)
    {
    case LogLevel::Info:
        putStringToBuffer(Log::Color::Green);
        putStringToBuffer(titleInf);
        break;
    case LogLevel::Warn:
        putStringToBuffer(Log::Color::Yellow);
        putStringToBuffer(titleWarn);
        break;
    case LogLevel::Error:
        putStringToBuffer(Log::Color::Red);
        putStringToBuffer(titleErr);
        break;
    default:
        putStringToBuffer(Log::Color::Default);
    }
    putStringToBuffer(moduleNames[module]);
    intBase = Base::Dec;
    floatPrecision = 3;
}

bool Log::checkLogAvailable()
{
    if (busy)
    {
        failcounter++;
        return false;
    }

    bool busy = true;
    if (m_portIsBusy)
        busy = m_portIsBusy(100);
    return (busy ? false : true);
}

void Log::putStringToBuffer(const char* str)
{
    if (!str)
        return;
    uint8_t offset = strlen(str);
    if ((m_buffEndIndex + offset) > m_BufferMaxSize)
        offset = m_BufferMaxSize - m_buffEndIndex;   // message doesn't fit into the rest of the buffer line

    if (offset > 0)
    {
        memcpy(&line[m_buffEndIndex], str, offset);
        m_buffEndIndex += offset;
    }
}

void Log::setTransmitter(DataSender callback)
{
    m_sender = callback;
    line = outstring;
}

void Log::setPortChecking(IsPortAvailable callback)
{
    m_portIsBusy = callback;
}

void Log::setSysTimeGetter(GetSysTime callback)
{
    m_getSysTimeMs = callback;
}

Log::~Log()
{
    if (m_onceHash != 0)  // This log has Once Specifier
    {
        uint32_t skippedMsgCount = 0;
        m_thisLogEnable = checkOnceTimeout(m_onceHash, skippedMsgCount);
        if (m_thisLogEnable)
        {
            // Put information about skipped messages count in the postfix:
            char oncePostfix[10]{0};
            int postfixSize = sprintf(oncePostfix, " [%u]", skippedMsgCount);
            putStringToBuffer(oncePostfix);
        }
    }
    if (m_sender && line && m_thisLogEnable)
        m_sender(line, m_buffEndIndex);    // Send string to stream
    m_buffEndIndex = 0;
    busy = false;   // Release log
}

Log& Log::operator<<(const char* str)
{
    if (!str || !busy || !m_thisLogEnable)
        return *this;

    putStringToBuffer(str);
    return *this;
}

Log& Log::operator<<(char ch)
{
    if (!busy || !m_thisLogEnable || (m_buffEndIndex == m_BufferMaxSize))
        return *this;

    memcpy(&line[m_buffEndIndex], &ch, 1);
    m_buffEndIndex++;
    return *this;
}

Log& Log::operator<<(float val)
{
    if (!m_thisLogEnable) return *this;
    putRealToBuffer<float>(val);
    return *this;
}

Log& Log::operator<<(double val)
{
    if (!m_thisLogEnable) return *this;
    putRealToBuffer<double>(val);
    return *this;
}

Log& Log::operator<<(int val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<int>(val, true);
    return *this;
}

Log& Log::operator<<(uint8_t val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<uint8_t>(val, false);
    return *this;
}

Log& Log::operator<<(uint16_t val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<uint16_t>(val, false);
    return *this;
}

Log& Log::operator<<(uint32_t val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<uint32_t>(val, false);
    return *this;
}

Log& Log::operator<<(uint64_t val)
{
    if (!m_thisLogEnable) return *this;
    putInt64ToBuffer<uint64_t>(val, false);
    return *this;
}

Log& Log::operator<<(int8_t val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<int8_t>(val, true);
    return *this;
}

Log& Log::operator<<(int16_t val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<int16_t>(val, true);
    return *this;
}

Log& Log::operator<<(int32_t val)
{
    if (!m_thisLogEnable) return *this;
    putIntToBuffer<int32_t>(val, true);
    return *this;
}

Log& Log::operator<<(int64_t val)
{
    if (!m_thisLogEnable) return *this;
    putInt64ToBuffer<int64_t>(val, true);
    return *this;
}

Log& Log::operator<<(Base basemod)
{
    if (!m_thisLogEnable) return *this;
    intBase = basemod;
    return *this;
}

Log& Log::operator<<(Precision prec)
{
    if (!m_thisLogEnable) return *this;
    floatPrecision = prec.value;
    return *this;
}

Log& Log::operator<<(const OnceSpec& once)
{
    m_onceHash = MakeHash(once);
    m_onceTimeSetMs = once.m_timeoutMs;
    return *this;
}

bool Log::isGroupAllowed(LogGroup module)
{
    return(maskModule [module/bitsInByte] & (1 << (module % bitsInByte))) != 0;
}

void Log::groupOn(LogGroup module)
{
    maskModule[module/bitsInByte] |= 1 << (module % bitsInByte);
}

void Log::groupOff(LogGroup module)
{
    maskModule[module/bitsInByte] &= ~(1 << (module % bitsInByte));
}

void Log::enable()
{
    logEnable = true;
}

void Log::disable()
{
    logEnable = false;
}

bool Log::checkOnceTimeout(const uint64_t& hash, uint32_t& skippedMsgCount)
{
    bool passed = true;
    if (!m_getSysTimeMs)
        return passed;   // No sysTimeGetter connected- Once doesn't work, pass everything

    uint64_t timestamp = m_getSysTimeMs();
    auto iter = onceSpecStorage.find(hash);
    if (iter != onceSpecStorage.end())
    {
        if ((timestamp - iter->second.lastTimeStamp) < m_onceTimeSetMs)
        {
            // The time has not elapsed yet. Save skipped messages counter and disable output:
            skippedMsgCount = ++iter->second.skippedMsgCounter;
            passed = false;
        }
        else
        {
            // The time has elapsed. Update the timestamp in storage and reset skipped messages counter:
            iter->second.lastTimeStamp = timestamp;
            skippedMsgCount = iter->second.skippedMsgCounter;
            iter->second.skippedMsgCounter = 0;
        }
    }
    else   // It's a new Log instance
    {
        OnceSpecCounter newOnce = {.lastTimeStamp = timestamp, .skippedMsgCounter = 0};
        onceSpecStorage.insert(etl::pair{hash, newOnce});
    }
    return passed;
}
