#ifndef TIMING_BASIC__
#define TIMING_BASIC__

#define CLOCK_SPEED 16777216
#define NS_PER_CYCLE ((1000000000/CLOCK_SPEED)+((1000000000%CLOCK_SPEED) == 0 ? 0 : 1))

#define GBA_CLOCK_SPEED 16777216

#endif
