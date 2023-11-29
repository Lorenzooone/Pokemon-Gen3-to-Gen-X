#ifndef GRAPHICS_HANDLER__
#define GRAPHICS_HANDLER__

#include <stddef.h>

void convert_xbpp(u8*, u32*, size_t, u8*, u8, u8);
void convert_1bpp(u8*, u32*, size_t, u8*, u8);
void convert_2bpp(u8*, u32*, size_t, u8*, u8);
void convert_3bpp(u8*, u32*, size_t, u8*, u8);

#endif
