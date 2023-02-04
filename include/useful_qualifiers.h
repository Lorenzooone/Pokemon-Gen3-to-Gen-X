#ifndef USEFUL_QUALIFIERS__
#define USEFUL_QUALIFIERS__

#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#define ALWAYS_INLINE __attribute__((always_inline)) static inline
#define MAX_OPTIMIZE __attribute__((optimize(3)))
#define PACKED __attribute__((packed))
#define ARM_TARGET __attribute__((target("arm"),noinline))
#define ALIGNED(x) __attribute__((aligned(x)))

#endif
