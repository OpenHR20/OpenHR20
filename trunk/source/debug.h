

#define DEBUG_MODE 1

#if DEBUG_MODE == 1
    #define DEBUG_BEFORE_SLEEP() (PORTE &= ~(1<<PE2))
    #define DEBUG_AFTER_SLEEP() (PORTE |= (1<<PE2))
#else
    #define DEBUG_BEFORE_SLEEP()
    #define DEBUG_AFTER_SLEEP()
#endif
