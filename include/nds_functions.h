#ifndef NDS_FUNCTIONS__
#define NDS_FUNCTIONS__

#ifdef __NDS__
typedef struct {
	u16 attr0;
	u16 attr1;
	u16 attr2;
	u16 dummy;
} ALIGN(4) OBJATTR;

void Halt_ARM9(void);
#endif

#endif
