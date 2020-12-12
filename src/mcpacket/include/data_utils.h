#ifndef MCD_DATAUTILS_H
#define MCD_DATAUTILS_H

#include <stdint.h>
#include <stdio.h>
#include "particle_types.h"
#include "translation_utils.h"

vector_typedef(char)
vector_typedef(int32_t)
optional_typedef(nbt_tag_compound)
optional_typedef(string_t)
optional_typedef(int32_t)

typedef struct mc_uuid {
  uint64_t msb;
  uint64_t lsb;
} mc_uuid;
optional_typedef(mc_uuid)

typedef struct mc_position {
  int32_t x;
  int32_t y;
  int32_t z;
} mc_position;
optional_typedef(mc_position)

typedef struct mc_slot {
  uint8_t present;
  int32_t item_id;
  int8_t item_count;
  nbt_tag_compound_optional_t nbt_data;
} mc_slot;
vector_typedef(mc_slot)
void mc_slot_encode(FILE* dest, mc_slot* this);
void mc_slot_decode(FILE* src, mc_slot* this);

typedef struct mc_particle {
  mc_particle_type type;
  int32_t block_state;
  float red;
  float green;
  float blue;
  float scale;
  mc_slot item;
} mc_particle;
void mc_particle_encode(FILE* dest, mc_particle* this);
void mc_particle_decode(FILE* src, mc_particle* this, mc_particle_type p_type);

typedef struct mc_smelting {
  char* group;
  mc_slot_vector_t ingredient;
  mc_slot result;
  float experience;
  int32_t cook_time;
} mc_smelting;
void mc_smelting_encode(FILE* dest, mc_smelting* this);
void mc_smelting_decode(FILE* src, mc_smelting* this);


typedef struct mc_tag {
  char* tag_name;
  int32_t_vector_t entries;
} mc_tag;
vector_typedef(mc_tag)
void mc_tag_encode(FILE* dest, mc_tag* this);
void mc_tag_decode(FILE* src, mc_tag* this);

typedef struct mc_item {
  int8_t slot;
  mc_slot item;
} mc_item;
vector_typedef(mc_item)

typedef struct mc_entity_equipment {
   mc_item_vector_t equipments;
} mc_entity_equipment;
void mc_entity_equipment_encode(FILE* dest, mc_entity_equipment* this);
void mc_entity_equipment_decode(FILE* src, mc_entity_equipment* this);

typedef enum nbt_metadata_tag_type {
  NBT_METADATA_TAG_BYTE,
  NBT_METADATA_TAG_VARINT,
  NBT_METADATA_TAG_FLOAT,
  NBT_METADATA_TAG_STRING,
  NBT_METADATA_TAG_CHAT,
  NBT_METADATA_TAG_OPTCHAT,
  NBT_METADATA_TAG_SLOT,
  NBT_METADATA_TAG_BOOLEAN,
  NBT_METADATA_TAG_ROTATION,
  NBT_METADATA_TAG_POSITION,
  NBT_METADATA_TAG_OPTPOSITION,
  NBT_METADATA_TAG_DIRECTION,
  NBT_METADATA_TAG_OPTUUID,
  NBT_METADATA_TAG_BLOCKID,
  NBT_METADATA_TAG_NBT,
  NBT_METADATA_TAG_PARTICLE,
  NBT_METADATA_TAG_VILLAGERDATA,
  NBT_METADATA_TAG_OPTVARINT,
  NBT_METADATA_TAG_POSE
} nbt_metadata_tag_type;

typedef struct nbt_metadata_tag {
  uint8_t index;
  nbt_metadata_tag_type type;
  // WTF
  union {
    int8_t value_int8;
    int32_t value_int32;
    float value_float;
    string_t value_str;
    string_t_optional_t value_str_opt;
    mc_slot value_mc_slot;
    float value_float_3[3];
    mc_position value_mc_position;
    mc_position_optional_t value_mc_position_opt;
    mc_uuid_optional_t value_mc_uuid_opt;
    nbt_tag_compound value_tag_compound;
    mc_particle value_mc_particle;
    int32_t value_int32_3[3];
    int32_t_optional_t value_int32_opt;
  } value;
} nbt_metadata_tag;

typedef struct mc_entity_metadata {
  nbt_metadata_tag* data;
} mc_entity_metadata;
void mc_entity_metadata_encode(FILE* dest, mc_entity_metadata* this);
void mc_entity_metadata_decode(FILE* src, mc_entity_metadata* this);

void enc_byte(FILE* dest, const uint8_t src);
uint8_t dec_byte(FILE* src);

void enc_be16(FILE* dest, uint16_t src);
uint16_t dec_be16(FILE* src);
void enc_le16(FILE* dest, uint16_t src);
uint16_t dec_le16(FILE* src);

void enc_be32(FILE* dest, uint32_t src);
uint32_t dec_be32(FILE* src);
void enc_le32(FILE* dest, uint32_t src);
uint32_t dec_le32(FILE* src);

void enc_be64(FILE* dest, uint64_t src);
uint64_t dec_be64(FILE* src);
void enc_le64(FILE* dest, uint64_t src);
uint64_t dec_le64(FILE* src);

void enc_bef32(FILE* dest, float src);
float dec_bef32(FILE* src);
void enc_lef32(FILE* dest, float src);
float dec_lef32(FILE* src);

void enc_bef64(FILE* dest, double src);
double dec_bef64(FILE* src);
void enc_lef64(FILE* dest, double src);
double dec_lef64(FILE* src);

typedef enum varnum_fail {
  VARNUM_INVALID = -1, // Impossible varnum, give up
  VARNUM_OVERRUN = -2  // Your buffer is too small, read more
} varnum_fail;

int verify_varnum(const char *buf, size_t max_len, int type_max);
int verify_varint(const char *buf, size_t max_len);
int verify_varlong(const char *buf, size_t max_len);

size_t size_varint(uint32_t varint);
size_t size_varlong(uint64_t varlong);

void enc_varint(FILE* dest, uint64_t src);
int64_t dec_varint(FILE* src);

void enc_string(FILE* dest, const char* src);
char* dec_string(FILE* src);

void enc_uuid(FILE* dest, mc_uuid src);
mc_uuid dec_uuid(FILE* src);

void enc_position(FILE* dest, mc_position src);
mc_position dec_position(FILE* src);

void enc_buffer(FILE* dest, char_vector_t src);
char_vector_t dec_buffer(FILE* src, size_t len);

#endif
