#ifndef CONFIG_SETTINGS__
#define CONFIG_SETTINGS

#include <stddef.h>

void set_default_settings(void);
void set_sys_language(u8);
void set_target_int_language(u8);
u8 get_sys_language(void);
u8 get_target_int_language(void);
u8 get_filtered_target_int_language(void);

#endif
