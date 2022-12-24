#include <gba.h>
#include "text_handler.h"

const u8 text_gen3_to_gen12_int_bin[];
const u8 text_gen3_to_gen12_jp_bin[];
const u8 text_gen3_to_gen12_int_jp_bin[];
const u8 text_gen3_to_gen12_jp_int_bin[];
const u8 text_gen3_to_general_int_bin[];
const u8 text_gen3_to_general_jp_bin[];
const u8 text_general_to_gen3_int_bin[];
const u8 text_general_to_gen3_jp_bin[];
const u8 text_gen12_to_gen3_int_bin[];
const u8 text_gen12_to_gen3_jp_bin[];

u8 text_general_count_question(u8* src, u8 src_size, u8 terminator, u8 question) {
    int counter = 0;
    for(int i = 0; i < src_size; i++) {
        if(src[i] == terminator)
            break;
        if(src[i] == question)
            counter++;
    }
    return counter;
}

u8 text_general_size(u8* src, u8 src_size, u8 terminator) {
    int i = 0;
    for(; i < src_size; i++)
        if(src[i] == terminator)
            break;
    return i;
}

u8 text_generic_count_question(u8* src, u8 src_size) {
    return text_general_count_question(src, src_size, GENERIC_EOL, GENERIC_QUESTION);
}

u8 text_gen3_count_question(u8* src, u8 src_size) {
    return text_general_count_question(src, src_size, GEN3_EOL, GEN3_QUESTION);
}

u8 text_gen2_count_question(u8* src, u8 src_size) {
    return text_general_count_question(src, src_size, GEN2_EOL, GEN2_QUESTION);
}

u8 text_generic_size(u8* src, u8 src_size) {
    return text_general_size(src, src_size, GENERIC_EOL);
}

u8 text_gen3_size(u8* src, u8 src_size) {
    return text_general_size(src, src_size, GEN3_EOL);
}

u8 text_gen2_size(u8* src, u8 src_size) {
    return text_general_size(src, src_size, GEN2_EOL);
}

void text_general_conversion(u8* src, u8* dst, u8 src_size, u8 dst_size, u8 src_terminator, u8 dst_terminator, u8* conv_table) {
    for(int i = 0; (i < src_size) && (i < dst_size); i++) {
        if(src[i] == src_terminator) {
            dst[i] = dst_terminator;
            break;
        }
        dst[i] = conv_table[src[i]];
    }
}

void text_generic_to_gen3(u8* src, u8* dst, u8 src_size, u8 dst_size, u8 jp_src, u8 jp_dst) {
    if(jp_dst)
        text_general_conversion(src, dst, src_size, dst_size, GENERIC_EOL, GEN3_EOL, text_general_to_gen3_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GENERIC_EOL, GEN3_EOL, text_general_to_gen3_int_bin);
}

void text_gen3_to_generic(u8* src, u8* dst, u8 src_size, u8 dst_size, u8 jp_src, u8 jp_dst) {
    if(jp_src)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GENERIC_EOL, text_gen3_to_general_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GENERIC_EOL, text_gen3_to_general_int_bin);
}

void text_gen3_to_gen12(u8* src, u8* dst, u8 src_size, u8 dst_size, u8 jp_src, u8 jp_dst) {
    if(jp_src && jp_dst)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_jp_bin);
    else if(jp_src)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_jp_int_bin);
    else if(jp_dst)
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_int_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GEN3_EOL, GEN2_EOL, text_gen3_to_gen12_int_bin);
}

void text_gen12_to_gen3(u8* src, u8* dst, u8 src_size, u8 dst_size, u8 jp_src, u8 jp_dst) {
    if(jp_src)
        text_general_conversion(src, dst, src_size, dst_size, GEN2_EOL, GEN3_EOL, text_gen12_to_gen3_jp_bin);
    else
        text_general_conversion(src, dst, src_size, dst_size, GEN2_EOL, GEN3_EOL, text_gen12_to_gen3_int_bin);
}