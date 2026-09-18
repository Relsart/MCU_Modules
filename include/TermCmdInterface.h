#pragma once
#include <stdint.h>

namespace console {

class CmdInterface
{
private:
    const char* name;
    const char* description;

public:
    /**
     * @brief Command name (id) getter
     */
    const char* getName ()
    {
        return name;
    }

    /**
     * @brief Help getter
     */
    const char* getDescription ()
    {
        return description;
    }

    virtual void exec (uint32_t argc, char** arg) = 0;    

    CmdInterface (const char* name_, const char* description_): name (name_), description (description_)
    {}
};

}   // namespace console