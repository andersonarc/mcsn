// I re-read your first,
// your second, your third,
//
// look for your small xx,
// feeling absurd.
//
// The codes we send
// arrive with a broken chord.
//   - Carol Ann Duffy, Text

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <endian.h>
#include "translation_utils.h"
#include "data_utils.h"
#include "malloc.h"

typedef enum nbt_tag_type {
  TAG_END,
  TAG_BYTE,
  TAG_SHORT,
  TAG_INT,
  TAG_LONG,
  TAG_FLOAT,
  TAG_DOUBLE,
  TAG_BYTE_ARRAY,
  TAG_STRING,
  TAG_LIST,
  TAG_COMPOUND,
  TAG_INT_ARRAY,
  TAG_LONG_ARRAY
} nbt_tag_type;

void enc_byte(FILE* dest, const uint8_t src) {
  fputc(src, dest);
}
uint8_t dec_byte(FILE* src) {
  return (uint8_t) fgetc(src);
}

void enc_be16(FILE* dest, uint16_t src) {
  src = htobe16(src);
  fwrite(&src, sizeof(uint16_t), 1, dest);
}
uint16_t dec_be16(FILE* src) {
  uint16_t dest;
  fread(&dest, sizeof(uint16_t), 1, src);
  return be16toh(dest);
}
void enc_le16(FILE* dest, uint16_t src) {
  src = htole16(src);
  fwrite(&src, sizeof(uint16_t), 1, dest);
}
uint16_t dec_le16(FILE* src) {
  uint16_t dest;
  fread(&dest, sizeof(uint16_t), 1, src);
  return le16toh(dest);
}

void enc_be32(FILE* dest, uint32_t src) {
  src = htobe32(src);
  fwrite(&src, sizeof(uint32_t), 1, dest);
}
uint32_t dec_be32(FILE* src) {
  uint32_t dest;
  fread(&dest, sizeof(uint32_t), 1, src);
  return be32toh(dest);
}
void enc_le32(FILE* dest, uint32_t src) {
  src = htole32(src);
  fwrite(&src, sizeof(uint32_t), 1, dest);
}
uint32_t dec_le32(FILE* src) {
  uint32_t dest;
  fread(&dest, sizeof(uint32_t), 1, src);
  return le32toh(dest);
}

void enc_be64(FILE* dest, uint64_t src) {
  src = htobe64(src);
  fwrite(&src, sizeof(uint64_t), 1, dest);
}
uint64_t dec_be64(FILE* src) {
  uint64_t dest;
  fread(&dest, sizeof(uint64_t), 1, src);
  return be64toh(dest);
}
void enc_le64(FILE* dest, uint64_t src) {
  src = htole64(src);
  fwrite(&src, sizeof(uint64_t), 1, dest);
}
uint64_t dec_le64(FILE* src) {
  uint64_t dest;
  fread(&dest, sizeof(uint64_t), 1, src);
  return le64toh(dest);
}

void enc_bef32(FILE* dest, float src) {
  uint32_t tmp = htobe32((uint32_t) src);
  fwrite(&tmp, sizeof(uint32_t), 1, dest);
}
float dec_bef32(FILE* src) {
  uint32_t dest;
  fread(&dest, sizeof(uint32_t), 1, src);
  return (float) be32toh(dest);
}
void enc_lef32(FILE* dest, float src) {
  uint32_t tmp = htole32((uint32_t) src);
  fwrite(&tmp, sizeof(uint32_t), 1, dest);
}
float dec_lef32(FILE* src) {
  uint32_t dest;
  fread(&dest, sizeof(uint32_t), 1, src);
  return (float) le32toh(dest);
}

void enc_bef64(FILE* dest, double src) {
  uint64_t tmp = htobe64((uint64_t) src);
  fwrite(&tmp, sizeof(uint64_t), 1, dest);
}
double dec_bef64(FILE* src) {
  uint64_t dest;
  fread(&dest, sizeof(uint64_t), 1, src);
  return (double) be32toh(dest);
}
void enc_lef64(FILE* dest, double src) {
  uint64_t tmp = htole64((uint64_t) src);
  fwrite(&tmp, sizeof(uint64_t), 1, dest);
}
double dec_lef64(FILE* src) {
  uint64_t dest;
  fread(&dest, sizeof(uint64_t), 1, src);
  return (double) le64toh(dest);
}

int verify_varnum(const char *buf, size_t max_len, int type_max) {
  if(!max_len)
    return VARNUM_OVERRUN;
  int len =  1;
  for(; *(const unsigned char*) buf & 0x80; buf++, len++) {
    if(len == type_max)
      return VARNUM_INVALID;
    if((size_t) len == max_len)
      return VARNUM_OVERRUN;
  }
  return len;
}

int verify_varint(const char *buf, size_t max_len) {
  return verify_varnum(buf, max_len, 5);
}
int verify_varlong(const char *buf, size_t max_len) {
  return verify_varnum(buf, max_len, 10);
}

size_t size_varint(uint32_t varint) {
  if(varint < (1 << 7))
    return 1;
  if(varint < (1 << 14))
    return 2;
  if(varint < (1 << 21))
    return 3;
  if(varint < (1 << 28))
    return 4;
  return 5;
}
size_t size_varlong(uint64_t varlong) {
  if(varlong < (1 << 7))
    return 1;
  if(varlong < (1 << 14))
    return 2;
  if(varlong < (1 << 21))
    return 3;
  if(varlong < (1 << 28))
    return 4;
  if(varlong < (1ULL << 35))
    return 5;
  if(varlong < (1ULL << 42))
    return 6;
  if(varlong < (1ULL << 49))
    return 7;
  if(varlong < (1ULL << 56))
    return 8;
  if(varlong < (1ULL << 63))
    return 9;
  return 10;
}

void enc_varint(FILE* dest, uint64_t src) {
  for(; src >= 0x80; src >>= 7)
    fputc(0x80 | (src & 0x7F), dest);
  fputc(src & 0x7F, dest);
}
int64_t dec_varint(FILE* src) {
  uint64_t dest = 0;
  int i = 0;
  uint64_t j = (uint64_t) fgetc(src);
  for(; j & 0x80; i+=7, j = (uint64_t) fgetc(src))
    dest |= (j & 0x7F) << i;
  dest |= j << i;
  return (int64_t) dest;
}

void enc_string(FILE* dest, const char* src) {
  enc_varint(dest, strlen(src));
  fputs(src, dest);
}
MALLOC char* dec_string(FILE* src) {
  size_t length = (size_t) dec_varint(src);
  char* string = malloc(length);
  fread(string, sizeof(char), length, src);
  return string;
}

void enc_uuid(FILE* dest, mc_uuid src) {
  enc_be64(dest, src.msb);
  enc_be64(dest, src.lsb);
}
mc_uuid dec_uuid(FILE* src) {
  mc_uuid dest;
  dest.msb = dec_be64(src);
  dest.lsb = dec_be64(src);
  return dest;
}

// From MSB to LSB x: 26-bits, z: 26-bits, y: 12-bits
// each is an independent signed 2-complement integer
void enc_position(FILE* dest, mc_position src) {
  uint64_t tmp = (
    ((uint64_t) src.x & 0x3FFFFFFUL) << 38 |
    ((uint64_t) src.z & 0x3FFFFFFUL) << 12 |
    ((uint64_t) src.y & 0xFFFUL)
  );
  enc_be64(dest, tmp);
}
mc_position dec_position(FILE* src) {
  mc_position dest;
  uint64_t tmp = dec_be64(src);
  if((dest.x = tmp >> 38) & (1UL << 25))
    dest.x -= 1UL << 26;
  if((dest.z = tmp >> 12 & 0x3FFFFFFUL) & (1UL << 25))
    dest.z -= 1UL << 26;
  if((dest.y = tmp & 0xFFFUL) & (1UL << 11))
    dest.y -= 1UL << 12;
  return dest;
}

void enc_buffer(FILE* dest, const char_vector_t src) {
  fwrite(src.data, sizeof(char), src.size, dest);
}

MALLOC char_vector_t dec_buffer(FILE* src, size_t len) {
  char_vector_t dest;
  dest.size = len;
  dest.data = malloc(sizeof(char) * len);
  fread(&dest.data, sizeof(char), len, src);
  return dest;
}

void mc_slot_encode(FILE* dest, mc_slot* this) {
  enc_byte(dest, this->present);
  if (this->present) {
    enc_varint(dest, (uint64_t) this->item_id);
    enc_byte(dest, (uint8_t) this->item_count);
    if(this->nbt_data.has_value) {
      /* nbt_data.value().encode_full(dest); */
    } else {
      enc_byte(dest, TAG_END);
    }
  }
}

void mc_slot_decode(FILE* src, mc_slot* this) {
  this->present = dec_byte(src);
  if (this->present) {
    this->item_id = (int32_t) dec_varint(src);
    this->item_count = (int8_t) dec_byte(src);
    if (dec_byte(src) == TAG_COMPOUND) {
      /* nbt_data.emplace(src, read_string(src)); */
    }
  }
}

void mc_particle_encode(FILE* dest, mc_particle* this) {
  switch(this->type) {
    case PARTICLE_BLOCK:
    case PARTICLE_FALLING_DUST:
      enc_varint(dest, (uint64_t) this->block_state);
      break;

    case PARTICLE_DUST:
      enc_be32(dest, (uint32_t) this->red);
      enc_be32(dest, (uint32_t) this->green);
      enc_be32(dest, (uint32_t) this->blue);
      enc_be32(dest, (uint32_t) this->scale);
      break;

    case PARTICLE_ITEM:
      mc_slot_encode(&this->item, dest);
      break;
  }
}

void mc_particle_decode(FILE* src, mc_particle* this, mc_particle_type p_type) {
  this->type = p_type;
  switch (this->type) {
    case PARTICLE_BLOCK:
    case PARTICLE_FALLING_DUST:
      this->block_state = (int32_t) dec_varint(src);
      break;

    case PARTICLE_DUST:
      this->red = (float) dec_be32(src);
      this->green = (float) dec_be32(src);
      this->blue = (float) dec_be32(src);
      this->scale = (float) dec_be32(src);
      break;

    case PARTICLE_ITEM:
      mc_slot_decode(&this->item, src);
      break;
  }
}

void mc_smelting_encode(FILE* dest, mc_smelting* this) {
  enc_string(dest, this->group);
  enc_varint(dest, this->ingredient.size);
  for (size_t i = 0; i < this->ingredient.size; i++) {
    mc_slot_encode(&this->ingredient.data[i], dest);
  }
  mc_slot_encode(&this->result, dest);
  enc_bef32(dest, this->experience);
  enc_varint(dest, (uint64_t) this->cook_time);
}

MALLOC void mc_smelting_decode(FILE* src, mc_smelting* this) {
  this->group = dec_string(src);
  this->ingredient.size = (size_t) dec_varint(src);
  this->ingredient.data = malloc(this->ingredient.size * sizeof(mc_slot));
  for (size_t i = 0; i < this->ingredient.size; i++) {
    mc_slot_decode(&this->ingredient.data[i], src);
  }
  mc_slot_decode(&this->result, src);
  this->experience = dec_bef32(src);
  this->cook_time = (int32_t) dec_varint(src);
}

void mc_tag_encode(FILE* dest, mc_tag* this) {
  enc_string(dest, this->tag_name);
  enc_varint(dest, this->entries.size);
  for (size_t i = 0; i < this->entries.size; i++) {
    enc_varint(dest, (uint64_t) this->entries.data[i]);
  }
}

void mc_tag_decode(FILE* src, mc_tag* this) {
  this->tag_name = dec_string(src);
  this->entries.size = (size_t) dec_varint(src);
  this->entries.data = malloc(this->entries.size * sizeof(int32_t));
  for (size_t i = 0; i < this->entries.size; i++) {
    this->entries.data[i] = (int32_t) dec_varint(src);
  }
}

void mc_entity_equipment_encode(FILE* dest, mc_entity_equipment* this) {
  for (size_t i = 0; i < (this->equipments.size - 1); i++) {
    enc_byte(dest, (uint8_t) (0x80 | this->equipments.data[i].slot));
    mc_slot_encode(&this->equipments.data[i].item, dest);
  }
  mc_item last = this->equipments.data[this->equipments.size - 1];
  enc_byte(dest, (uint8_t) last.slot);
  mc_slot_encode(&last.item, dest);
}

void mc_entity_equipment_decode(FILE* src, mc_entity_equipment* this) {
  //todo VLA

  /*this->equipments.size = 0;
  uint8_t slot;
  do {
    slot = dec_byte(src);
    this->equipments.emplace_back();
    this->equipments.back().slot = slot & 0x7F;
    this->equipments.back().item.decode(src);
  } while(slot & 0x80);*/
}

void mc_entity_metadata_encode(FILE* dest, mc_entity_metadata* this) {
  /*for(auto &el : data) {
    enc_byte(dest, el.index);
    enc_varint(dest, el.type);
    switch(el.type) {
      case METATAG_BYTE:
      case METATAG_BOOLEAN:
        enc_byte(dest, (int8_t) el.value);
        break;
      case METATAG_VARINT:
      case METATAG_DIRECTION:
      case METATAG_BLOCKID:
      case METATAG_POSE:
        enc_varint(dest, (int32_t) el.value);
        break;
      case METATAG_FLOAT:
        enc_bef32(dest, (float) el.value);
        break;
      case METATAG_STRING:
      case METATAG_CHAT:
        enc_string(dest, (const char*) el.value);
        break;
      case METATAG_OPTCHAT: {
        auto str = (const char*) el.value;
        enc_byte(dest, str.has_value());
        if(str.has_value())
          enc_string(dest, str.value());
      }
        break;
      case METATAG_SLOT:
        (mc_slot) el.value.encode(dest);
        break;
      case METATAG_ROTATION:
        for(auto& el : (std::array<float, 3>) el.value)
          enc_bef32(dest, el);
      case METATAG_POSITION:
        enc_position(dest, (mc_position) el.value);
        break;
      case METATAG_OPTPOSITION: {
        auto pos = (mc_position) el.value;
        enc_byte(dest, pos.has_value());
        if(pos.has_value())
          enc_position(dest, pos.value());
      }
        break;
      case METATAG_OPTUUID: {
        auto uuid = (mc_uuid) el.value;
        enc_byte(dest, uuid.has_value());
        if(uuid.has_value())
          enc_uuid(dest, uuid.value());
      }
        break;
      case METATAG_NBT:
        (nbt::TagCompound) el.value.encode_full(dest);
        break;
      case METATAG_PARTICLE:
        enc_varint(dest, (MCParticle) el.value.type);
        (MCParticle) el.value.encode(dest);
        break;
      case METATAG_VILLAGERDATA:
        for(auto& el : (std::array<int32_t, 3>) el.value)
          enc_varint(dest, el);
        break;
      case METATAG_OPTVARINT: {
        auto varint = (int32_t) el.value;
        enc_byte(dest, varint.has_value());
        if(varint.has_value())
          enc_varint(dest, varint.value());
      }
        break;
    }
  }
  enc_byte(dest, 0xFF);*/
}

void mc_entity_metadata_decode(FILE* src, mc_entity_metadata* this) {
  /*data.clear();
  uint8_t index = dec_byte(src);
  while(index != 0xFF) {
    auto& tag = data.emplace_back();
    tag.index = index;
    tag.type = dec_varint(src);
    switch(tag.type) {
      case METATAG_BYTE:
      case METATAG_BOOLEAN:
        tag.value = dec_byte(src);
        break;
      case METATAG_VARINT:
      case METATAG_DIRECTION:
      case METATAG_BLOCKID:
      case METATAG_POSE:
        // Not sure why this is considered ambiguous, maybe conflict with int8?
        tag.value.emplace<int32_t>(dec_varint(src));
        break;
      case METATAG_FLOAT:
        tag.value = dec_bef32(src);
        break;
      case METATAG_STRING:
      case METATAG_CHAT:
        tag.value = dec_string(src);
        break;
      case METATAG_OPTCHAT: {
        auto& str = tag.value.emplace<const char*>();
        if(dec_byte(src))
          str = dec_string(src);
      }
        break;
      case METATAG_SLOT: {
        auto& slot = tag.value.emplace<mc_slot>();
        slot.decode(src);
      }
        break;
      case METATAG_ROTATION: {
        auto& rot = tag.value.emplace<std::array<float, 3>>();
        for(auto &el : rot)
          el = dec_bef32(src);
      }
        break;
      case METATAG_POSITION:
        tag.value = dec_position(src);
        break;
      case METATAG_OPTPOSITION: {
        auto& pos = tag.value.emplace<mc_position>();
        if(dec_byte(src))
          pos = dec_position(src);
      }
        break;
      case METATAG_OPTUUID: {
        auto& uuid = tag.value.emplace<mc_uuid>();
        if(dec_byte(src))
          uuid = dec_uuid(src);
      }
        break;
      case METATAG_NBT: {
        auto& nbt_tag = tag.value.emplace<nbt::TagCompound>();
        nbt_tag.decode_full(src);
      }
        break;
      case METATAG_PARTICLE: {
        auto& particle = tag.value.emplace<MCParticle>();
        particle.decode(src, (particle_type) dec_varint(src));
      }
        break;
      case METATAG_VILLAGERDATA: {
        auto& data = tag.value.emplace<std::array<int32_t, 3>>();
        for(auto &el : data)
          el = dec_varint(src);
      }
        break;
      case METATAG_OPTVARINT: {
        auto& varint = tag.value.emplace<int32_t>();
        if(dec_byte(src))
          varint = dec_varint(src);
      }
        break;
    }
    index = dec_byte(src);
  }*/
}
