include_directories(include)
aux_source_directory(./src srcDir)

# Adding non-RTOS functional: MainLoop and and the Signals it processes (Time, Offline)
if (NOT WITH_RTOS)
    aux_source_directory(./nonRtos nonRtosSrc)
    list(APPEND srcDir ${nonRtosSrc})
    include_directories(nonRtos)
endif()

add_library(modules STATIC ${srcDir})
