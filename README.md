# Stm32_Modules

# Slot-Signal module

Version from 27.01.2026
Realises a slot-signal data exchange mechanism
There are three types of signal templates: 
- [ ] Signal: invokes its handler immediately, as it activated
- [ ] SignalTime: periodically invokes by timer preset
- [ ] SignalMainLoop: invokes its handler in MainLoop, outside of function where it was activated

## Dependencies and macros:

- Macro "WITH_ETL" to determine which List to use (etl:: or std::)
- System time (in milliseconds) getter function

# McuLoger module
Version from 31.03.2026
Realises a logs output to the Uart terminal (for use with MCUs)

## Functionality:
- Overloaded << operator for char* strings, signed/unsigned integers (8/16/32/64-bytes), floats/doubles;
- Output integers in Hex/Dec format;
- Specializer for float variables precision;
- Customizable Log Groups (in LogGroups.h);

## Dependencies and macros:
- Function for waiting Uart (its DMA) to be available: uses DWT timeout checking method

## Example:
```
Log::setTransmitter(std::bind(&uart::Port::transmit, &console, std::placeholders::_1, std::placeholders::_2));
Log::setPortChecking([&](uint32_t timeout)
{
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (console.dmaTxIsBusy())
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }
    return true;
});
Log::groupOn(lmSystem);     // enable some logs levels..
Log(lmSystem, Info) << "Helloworld!";
```

# Ring Buffer module

Receiver ring buffers for reading data from periphery (uart, can, etc..).
It also contains the template for user-defined buffers (for example, can messages).

