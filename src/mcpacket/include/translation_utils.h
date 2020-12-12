/**
 * @file translationutils.h
 * @author andersonarc (andersonarc@github.com)
 * @brief utilities required for transition from C++ to C
 * @version 0.1
 * @date 2020-12-07
 * 
 * @copyright andersonarc
 */

#ifndef TRANSLATIONUTILS_H
#define TRANSLATIONUTILS_H

#include <stdlib.h>  /* size_t,  exit */
#include <stdio.h>   /* stderr, stdin */
#include <stdbool.h> /* bool */

/**
 * @brief nbt enum stub
 */
enum {
  NBT_TAG_END,
  NBT_TAG_BYTE,
  NBT_TAG_SHORT,
  NBT_TAG_INT,
  NBT_TAG_LONG,
  NBT_TAG_FLOAT,
  NBT_TAG_DOUBLE,
  NBT_TAG_BYTE_ARRAY,
  NBT_TAG_STRING,
  NBT_TAG_LIST,
  NBT_TAG_COMPOUND,
  NBT_TAG_INT_ARRAY,
  NBT_TAG_LONG_ARRAY
};

/**
 * @brief typedef for string
 */
#define string_t char*

/**
 * @brief exit with error
 */
void runtime_error(const char* message);

/**
 * @brief std::optional replacement
 */
#define optional_typedef(type)      \
typedef struct type##_optional_t {  \
    type value;                     \
    bool has_value;                 \
} type##_optional_t;

/**
 * @brief std::vector replacement
 */
#define vector_typedef(type)     \
typedef struct type##_vector_t {  \
    type* data;  \
    size_t size; \
} type##_vector_t;

/**
 * @brief indicates a runtime-allocated memory value
 */
#define MALLOC

/**
 * @brief currently not implemented NBT tag
 */
#define nbt_tag_compound void*

#endif /* TRANSLATIONUTILS_H */
