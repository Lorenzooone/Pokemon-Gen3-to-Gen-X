#ifndef PID_IV_TID__
#define PID_IV_TID__

void worst_case_conversion_tester(u32*);
void init_unown_tsv(void);

u8 get_roamer_ivs(u32, u8, u8, u32*);
void generate_unown_shiny_info(u8, u16, u16, u32*, u32*);
void generate_unown_info(u8, u16, u16, u32*, u32*);
void generate_static_shiny_info(u8, u16, u32*, u32*);
void generate_static_info(u8, u16, u16, u32*, u32*);
void generate_egg_shiny_info(u8, u8, u16, u16, u8, u32*, u32*);
void generate_egg_info(u8, u8, u16, u16, u8, u32*, u32*);
u32 generate_ot(u16, u8*);

#endif
