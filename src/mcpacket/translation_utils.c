/**
 * @file translation_utils.c
 * @author andersonarc (andersonarc@github.com)
 * @brief utilities required for transition from C++ to C
 * @version 0.1
 * @date 2020-12-12
 * 
 * @copyright andersonarc
 */
#include "translation_utils.h" /* this */

/**
 * @brief exit with error
 */
void runtime_error(const char* message) {
    fputs(message, stdout);
    fputs(message, stderr);
    exit(EXIT_FAILURE);
}

void encode_full() {}
void decode_full() {}
void nbt_read_string() {}