#include <gba.h>
#include "text_handler.h"
#include "bin_table_handler.h"
#include "useful_qualifiers.h"
#include <stddef.h>

#include "text_gen3_to_gen12_int_bin.h"
#include "text_gen3_to_gen12_jp_bin.h"
#include "text_gen3_to_gen12_int_jp_bin.h"
#include "text_gen3_to_gen12_jp_int_bin.h"
#include "text_gen3_to_general_int_bin.h"
#include "text_gen3_to_general_jp_bin.h"
#include "text_general_to_gen3_int_bin.h"
#include "text_general_to_gen3_jp_bin.h"
#include "text_gen12_to_gen3_int_bin.h"
#include "text_gen12_to_gen3_jp_bin.h"

#define GENERIC_A 0x61
#define GENERIC_Z 0x7A
#define GENERIC_TO_UPPER 0x20
#define GEN12_TRAINER 0x5D

size_t text_general_count_question(const u8*, size_t, u8, u8);
size_t text_general_size(const u8*, size_t, u8);
void text_general_conversion(const u8*, u8*, size_t, size_t, u8, u8, const u8*);
u8 text_general_is_same(const u8*, const u8*, size_t, size_t, u8);
void text_general_copy(const u8*, u8*, size_t, size_t, u8);
void text_general_concat(const u8*, const u8*, u8*, size_t, size_t, size_t, u8);
void text_general_replace(u8*, size_t, u8, u8, u8);
void text_general_terminator_fill(u8*, size_t, u8);

size_t text_general_count_question(const u8* src, size_t src_size, u8 terminator, u8 question) {
    size_t counter = 0;
    for(size_t i = 0; i < src_size; i++) {
        if(src[i] == terminator)
            break;
        if(src[i] == question)
            counter++;
    }
    return counter;
}

size_t text_general_size(const u8* src, size_t src_size, u8 terminator) {
    size_t i = 0;
    for(; i < src_size; i++)
        if(src[i] == terminator)
            break;
    return i;
}

void text_general_conversion(const u8* src, u8* dst, size_t src_size, size_t dst_size, u8 src_terminator, u8 dst_terminator, const u8* conv_table) {
    for(size_t i = 0; (i < src_size) && (i < dst_size); i++) {
        if(src[i] == src_terminator) {
            dst[i] = dst_terminator;
            break;
        }
        dst[i] = conv_table[src[i]];
    }
    if(src_size < dst_size)
        dst[src_size] = dst_terminator;
}

u8 text_general_is_same(const u8* src, const u8* dst, size_t src_size, size_t dst_size, u8 terminator) {
    if(text_general_size(src, src_size, terminator) != text_general_size(dst, dst_size, terminator))
        return 0;

    size_t i = 0;
    for(; (i < src_size) && (i < dst_size); i++) {
        if(src[i] == terminator) {
            if(dst[i] == terminator)
                return 1;
            else
                return 0;
        }
        if(dst[i] == terminator)
            return 0;
        if(src[i] != dst[i])
            return 0;
    }
    return 1;
    
}

void text_general_copy(const u8* src, u8* dst, size_t src_size, size_t dst_size, u8 terminator) {
    for(size_t i = 0; (i < src_size) && (i < dst_size); i++) {
        if(src[i] == terminator) {
            dst[i] = terminator;
            break;
        }
        dst[i] = src[i];
    }
    if(src_size < dst_size)
        dst[src_size] = terminator;
}

void text_general_concat(const u8* src, const u8* src2, u8* dst, size_t src_size, size_t src2_size, size_t dst_size, u8 terminator) {
    size_t i = 0;
    for(; (i < src_size) && (i < dst_size); i++) {
        if(src[i] == terminator) {
            break;
        }
        dst[i] = src[i];
    }
    for(size_t j = 0; (j < src2_size) && ((i+j) < dst_size); j++) {
        if(src2[j] == terminator) {
            dst[i+j] = terminator;
            break;
        }
        dst[i+j] = src2[j];
    }
    if((i+src2_size) < dst_size)
        dst[i+src2_size] = terminator;
}

void text_general_replace(u8* src, size_t src_size, u8 base_char, u8 new_char, u8 terminator) {
    u8 replaced_once = 0;
    for(size_t i = 0; i < src_size; i++) {
        if((src[i] == terminator) && (base_char != terminator))
            break;
        if(src[i] == base_char) {
            src[i] = new_char;
            replaced_once = 1;
        }
        if(replaced_once && (new_char == terminator))
            src[i] = terminator;
    }
}

void text_general_terminator_fill(u8* src, size_t src_size, u8 terminator) {
    for(size_t i = 0; i < src_size; i++)
        src[i] = terminator;    
}

void text_generic_concat(const u8* src, const u8* src2, u8* dst, size_t src_size, size_t src2_size, size_t dst_size) {
    text_general_concat(src, src2, dst, src_size, src2_size, dst_size, GENERIC_EOL);
}

void text_gen3_concat(const u8* src, const u8* src2, u8* dst, size_t src_size, size_t src2_size, size_t dst_size) {
    text_general_concat(src, src2, dst, src_size, src2_size, dst_size, GEN3_EOL);
}

void text_gen2_concat(const u8* src, const u8* src2, u8* dst, size_t src_size, size_t src2_size, size_t dst_size) {
    text_general_concat(src, src2, dst, src_size, src2_size, dst_size, GEN2_EOL);
}

void text_generic_terminator_fill(u8* src, size_t src_size) {
    text_general_terminator_fill(src, src_size, GENERIC_EOL);
}

void text_gen3_terminator_fill(u8* src, size_t src_size) {
    text_general_terminator_fill(src, src_size, GEN3_EOL);
}

void text_gen2_terminator_fill(u8* src, size_t src_size) {
    text_general_terminator_fill(src, src_size, GEN2_EOL);
}

void text_generic_replace(u8* src, size_t src_size, u8 base_char, u8 new_char) {
    text_general_replace(src, src_size, base_char, new_char, GENERIC_EOL);
}

void text_gen3_replace(u8* src, size_t src_size, u8 base_char, u8 new_char) {
    text_general_replace(src, src_size, base_char, new_char, GEN3_EOL);
}

void text_gen2_replace(u8* src, size_t src_size, u8 base_char, u8 new_char) {
    text_general_replace(src, src_size, base_char, new_char, GEN2_EOL);
}

void text_generic_copy(const u8* src, u8* dst, size_t src_size, size_t dst_size) {
    text_general_copy(src, dst, src_size, dst_size, GENERIC_EOL);
}

void text_gen3_copy(const u8* src, u8* dst, size_t src_size, size_t dst_size) {
    text_general_copy(src, dst, src_size, dst_size, GEN3_EOL);
}

void text_gen2_copy(const u8* src, u8* dst, size_t src_size, size_t dst_size) {
    text_general_copy(src, dst, src_size, dst_size, GEN2_EOL);
}

u8 text_generic_is_same(const u8* src, const u8* dst, size_t src_size, size_t dst_size) {
    return text_general_is_same(src, dst, src_size, dst_size, GENERIC_EOL);
}

u8 text_gen3_is_same(const u8* src, const u8* dst, size_t src_size, size_t dst_size) {
    return text_general_is_same(src, dst, src_size, dst_size, GEN3_EOL);
}

u8 text_gen2_is_same(const u8* src, const u8* dst, size_t src_size, size_t dst_size) {
    return text_general_is_same(src, dst, src_size, dst_size, GEN2_EOL);
}

size_t text_generic_count_question(const u8* src, size_t src_size) {
    return text_general_count_question(src, src_size, GENERIC_EOL, GENERIC_QUESTION);
}

size_t text_gen3_count_question(const u8* src, size_t src_size) {
    return text_general_count_question(src, src_size, GEN3_EOL, GEN3_QUESTION);
}

size_t text_gen2_count_question(const u8* src, size_t src_size) {
    return text_general_count_question(src, src_size, GEN2_EOL, GEN2_QUESTION);
}

size_t text_generic_size(const u8* src, size_t src_size) {
    return text_general_size(src, src_size, GENERIC_EOL);
}

size_t text_gen3_size(const u8* src, size_t src_size) {
    return text_general_size(src, src_size, GEN3_EOL);
}

size_t text_gen2_size(const u8* src, size_t src_size) {
    return text_general_size(src, src_size, GEN2_EOL);
}

void text_generic_to_gen3(const u8* src, u8* dst, size_t src_size, size_t dst_size, u8 UNUSED(jp_src), u8 jp_dst) {
    if(jp_dst)
        text_general_conversion(src, dst, src_size, dst_size, GENERIC_EOL, GEN3_EOL, text_general_to_gen3_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GENERIC_EOL, GEN3_EOL, text_general_to_gen3_int_bin);
}

void text_gen3_to_generic(const u8* src, u8* dst, size_t src_size, size_t dst_size, u8 jp_src, u8 UNUSED(jp_dst)) {
    if(jp_src)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GENERIC_EOL, text_gen3_to_general_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GENERIC_EOL, text_gen3_to_general_int_bin);
}

void text_gen3_to_gen12(const u8* src, u8* dst, size_t src_size, size_t dst_size, u8 jp_src, u8 jp_dst) {
    if(jp_src && jp_dst)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_jp_bin);
    else if(jp_src)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_jp_int_bin);
    else if(jp_dst)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_int_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_int_bin);
}

u8 is_gen12_trainer(const u8* src) {
    if(src[0] == GEN12_TRAINER)
        return 1;
    return 0;
}

void text_gen12_to_gen3(const u8* src, u8* dst, size_t src_size, size_t dst_size, u8 jp_src, u8 UNUSED(jp_dst)) {
    if(jp_src)
        text_general_conversion(src, dst, src_size, dst_size, GEN2_EOL, GEN3_EOL, text_gen12_to_gen3_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GEN2_EOL, GEN3_EOL, text_gen12_to_gen3_int_bin);
}
