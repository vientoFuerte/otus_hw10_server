#pragma once

// для быстрого включения/выключения отладочной информации.
#define DEBUG 1  // 1 - включить, 0 - выключить

#if DEBUG
    #define DLOG(...) std::cerr << "DEBUG: " << __VA_ARGS__

#else
    #define DLOG(...) 

#endif
