#ifndef TEXT_HANDLER__
#define TEXT_HANDLER__

#define GEN3_EOL 0xFF
#define GEN2_EOL 0x50
#define GENERIC_EOL 0

#define GEN3_QUESTION 0xAC
#define GEN2_QUESTION 0xE6
#define GENERIC_QUESTION 0x3F

void text_generic_to_upper(u8*, u8*, u8, u8);

void text_generic_replace(u8*, u8, u8, u8);
void text_gen3_replace(u8*, u8, u8, u8);
void text_gen2_replace(u8*, u8, u8, u8);

void text_generic_terminator_fill(u8*, u8);
void text_gen3_terminator_fill(u8*, u8);
void text_gen2_terminator_fill(u8*, u8);

void text_generic_copy(u8*, u8*, u8, u8);
void text_gen3_copy(u8*, u8*, u8, u8);
void text_gen2_copy(u8*, u8*, u8, u8);

u8 text_generic_is_same(u8*, u8*, u8, u8);
u8 text_gen3_is_same(u8*, u8*, u8, u8);
u8 text_gen2_is_same(u8*, u8*, u8, u8);

u8 text_generic_count_question(u8*, u8);
u8 text_gen3_count_question(u8*, u8);
u8 text_gen2_count_question(u8*, u8);

u8 text_generic_size(u8*, u8);
u8 text_gen3_size(u8*, u8);
u8 text_gen2_size(u8*, u8);

void text_generic_to_gen3(u8*, u8*, u8, u8, u8, u8);
void text_gen3_to_generic(u8*, u8*, u8, u8, u8, u8);
void text_gen3_to_gen12(u8*, u8*, u8, u8, u8, u8);
void text_gen12_to_gen3(u8*, u8*, u8, u8, u8, u8);

#endif