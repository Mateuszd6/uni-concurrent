// TODO: Not sure if this is TheGreatestIdeaOfAllTimes(TM)
#define VECTOR_PUSH(ARR__, SIZE__, CAPACITY__, VALUE__)                      \
    do {                                                                     \
        if(SIZE__ == CAPACITY__)                                             \
        {                                                                    \
            CAPACITY__ *= 2;                                                 \
            if(!(ARR__ = realloc(ARR__, CAPACITY__ * (sizeof(ARR__[0])))))   \
            {                                                                \
                PANIC("Could not reallocte with %lu bytes of memory!",       \
                      CAPACITY__ * (sizeof(ARR__[0])));                      \
            }                                                                \
        }                                                                    \
                                                                             \
        ARR__[SIZE__++] = VALUE__;                                           \
    } while(0)

#define VECTOR_INIT(ARR__, SIZE__, CAPACITY__, INITIAL_CAPACITY__)           \
    do {                                                                     \
        CAPACITY__ = INITIAL_CAPACITY__;                                     \
        SIZE__ = 0;                                                          \
        if(!(ARR__ = malloc(sizeof(ARR__[0]) * CAPACITY__)))                 \
        {                                                                    \
            PANIC("Could not allocate %lu bytes of memory!",                 \
                  sizeof(ARR__[0]) * CAPACITY__);                            \
        }                                                                    \
    } while(0)
