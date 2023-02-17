#ifndef PID_IV_TID__
#define PID_IV_TID__

void worst_case_conversion_tester(vu32*);
void init_unown_tsv(void);

u8 get_roamer_ivs(u32, u8, u8, u32*);
void generate_unown_shiny_info(u8, u16, u16, u32*, u32*);
void generate_unown_shiny_info_letter_preloaded(u8, u8, u16, u32*, u32*);
void generate_unown_info(u8, u16, u16, u32*, u32*);
void generate_unown_info_letter_preloaded(u8, u16, u8, u16, u32*, u32*);
void generate_static_shiny_info(u8, u16, u32*, u32*);
void generate_generic_genderless_shadow_shiny_info_colo(u8, u16, u32*, u32*, u8*);
void generate_static_info(u8, u16, u16, u32*, u32*);
void generate_generic_genderless_shadow_info_colo(u8, u16, u16, u32*, u32*, u8*);
void generate_generic_genderless_shadow_info_xd(u8, u8, u16, u16, u32*, u32*, u8*);
void generate_egg_shiny_info(u8, u8, u16, u16, u8, u32*, u32*);
void generate_egg_info(u8, u8, u16, u16, u8, u32*, u32*);
u32 generate_ot(u16, u8*);
u8 are_colo_valid_tid_sid(u16, u16);
void convert_roamer_to_colo_info(u8, u16, u8, u8, u16, u32*, u32*, u8*);
void convert_shiny_roamer_to_colo_info(u8, u8, u8, u16, u32*, u32*, u8*);

#endif
