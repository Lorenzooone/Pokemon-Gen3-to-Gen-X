#ifndef TEXT_HANDLER__
#define TEXT_HANDLER__

#include <stddef.h>

#define GENERIC_SPACE 0x20

#define GENERIC_M_GENDER 0x88
#define GENERIC_F_GENDER 0x89
#define GENERIC_U_GENDER GENERIC_SPACE

#define GEN3_FIRST_TICKS_START 0x37
#define GEN3_FIRST_TICKS_END 0x4A
#define GEN3_FIRST_CIRCLE_START 0x4B
#define GEN3_FIRST_CIRCLE_END 0x4F
#define GEN3_SECOND_TICKS_START 0x87
#define GEN3_SECOND_TICKS_END 0x9A
#define GEN3_SECOND_CIRCLE_START 0x9B
#define GEN3_SECOND_CIRCLE_END 0x9F
#define GENERIC_TICKS_CHAR 0xFD
#define GENERIC_CIRCLE_CHAR 0xFE

#define GEN3_EOL 0xFF
#define GEN2_EOL 0x50
#define GENERIC_EOL 0

#define GEN3_QUESTION 0xAC
#define GEN2_QUESTION 0xE6
#define GENERIC_QUESTION 0x3F

void text_generic_concat(const u8*, const u8*, u8*, size_t, size_t, size_t);
void text_gen3_concat(const u8*, const u8*, u8*, size_t, size_t, size_t);
void text_gen2_concat(const u8*, const u8*, u8*, size_t, size_t, size_t);

void text_generic_replace(u8*, size_t, u8, u8);
void text_gen3_replace(u8*, size_t, u8, u8);
void text_gen2_replace(u8*, size_t, u8, u8);

void text_generic_terminator_fill(u8*, size_t);
void text_gen3_terminator_fill(u8*, size_t);
void text_gen2_terminator_fill(u8*, size_t);

void text_generic_copy(const u8*, u8*, size_t, size_t);
void text_gen3_copy(const u8*, u8*, size_t, size_t);
void text_gen2_copy(const u8*, u8*, size_t, size_t);

u8 text_generic_is_same(const u8*, const u8*, size_t, size_t);
u8 text_gen3_is_same(const u8*, const u8*, size_t, size_t);
u8 text_gen2_is_same(const u8*, const u8*, size_t, size_t);

size_t text_generic_count_question(const u8*, size_t);
size_t text_gen3_count_question(const u8*, size_t);
size_t text_gen2_count_question(const u8*, size_t);

size_t text_generic_size(const u8*, size_t);
size_t text_gen3_size(const u8*, size_t);
size_t text_gen2_size(const u8*, size_t);

void text_generic_to_gen3(const u8*, u8*, size_t, size_t, u8, u8);
void text_gen3_to_generic(const u8*, u8*, size_t, size_t, u8, u8);
void text_gen3_to_gen12(const u8*, u8*, size_t, size_t, u8, u8);
void text_gen12_to_gen3(const u8*, u8*, size_t, size_t, u8, u8);

#endif
