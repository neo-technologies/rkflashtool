#define PUT32LE(x, y) \
    do { \
        (x)[0] = ((y)>> 0) & 0xff; \
        (x)[1] = ((y)>> 8) & 0xff; \
        (x)[2] = ((y)>>16) & 0xff; \
        (x)[3] = ((y)>>24) & 0xff; \
    } while (0)
