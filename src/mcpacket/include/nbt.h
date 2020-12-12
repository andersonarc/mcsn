#ifndef NBT_HPP
#define NBT_HPP

#include <iostream>
#include <string>
#include <cstdint>
#include <concepts>
#include <vector>
#include <optional>
#include <string.h>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include "byteswap.h"

#define template_FixedString(N) \
typedef struct _FixedString {   \
  char el[N];                   \
} _FixedString;                 \
_FixedString _FixedString_constructor(const char s[N]) { \
  _FixedString this = malloc(sizeof(char) * N);          \
  strcpy(s, this->el);                                   \
  return this;                                           \
}

// My tiny, crap version of pprint
// But we barely need pprint at all, so we use this instead
class NbtPrinter {
  FILE* stream_;
  size_t indent_;
  string_t line_terminator_;

public:
  NbtPrinter(FILE* stream = std::cout) : stream_(stream),
  indent_(2), line_terminator_("\n") {}

  NbtPrinter& line_terminator(const string_t& value) {
    line_terminator_ = value;
    return *this;
  }
  NbtPrinter& indent(size_t indent) {
    indent_ = indent;
    return *this;
  }
  NbtPrinter& inc_indent(size_t inc) {
    indent_ += inc;
    return *this;
  }
  NbtPrinter& dec_indent(size_t dec) {
    indent_ -= dec;
    return *this;
  }

  void print(auto value) {
    print_internal(value, 0, line_terminator_);
  }
  void print_inline(auto value) {
    print_internal(value, indent_, "");
  }

private:
  void print_internal(auto value, size_t indent = 0,
  const string_t* line_terminator = "\n") {
    stream_ << string_t(indent, ' ') << value << line_terminator;
  }

  void print_internal(float value, size_t indent = 0,
  const string_t* line_terminator = "\n") {
    stream_ << string_t(indent, ' ') << value << 'f' << line_terminator;
  }
};

enum {
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
};


class BaseTag {
public:
  const uint8_t tag_id;
  const string_t type_name;
  std::optional<string_t> name;

  BaseTag(const uint8_t id, const string_t* type_name) :
      tag_id(id), type_name(type_name) {}
  BaseTag(const string_t* name, const uint8_t id,
      const string_t type_name) : tag_id(id), type_name(type_name),
      name(name)  {}
  virtual ~BaseTag() = default;

  virtual void encode(FILE* buf) const = 0;
  virtual void decode(FILE* buf) = 0;
  virtual void print(FILE* os) const {
    NbtPrinter pr(os);
    print(pr);
  };
  virtual void print(NbtPrinter* pr, int pr_inline = 0) const = 0;
};

inline FILE* operator<<(FILE* os, const BaseTag* b) {
  b.print(os);
  return os;
}


template <typename T, int id>
class Tag : public BaseTag {
public:
  T val;
  
};
Tag Tag_constructor(const string_t& type_name) : BaseTag(id, type_name) {}
  Tag(const string_t name, const::string_t& type_name) :
      BaseTag(name, id, type_name) {}
  Tag(const T val, const string_t name, const::string_t& type_name) :
      BaseTag(name, id, type_name), val(val) {}


typedef struct TagIntegral {
  void decode(FILE* buf) {
    fread((char*) (&this->val), sizeof(this->val));
    this->val = nbeswap(this->val);
  }

  void encode(FILE* buf) const {
    Integral tmp = nbeswap(this->val);
    buf.write((char *) (&tmp), sizeof(tmp));
  }

  void print(NbtPrinter* pr, int pr_inline = 0) const {
    string_t str;
    if(this->name.has_value())
      str = this->type_name + "('" + this->name.value() + "'): " +
      std::to_string(this->val);
    else
      str = this->type_name + "(None): " + std::to_string(this->val);
    if(pr_inline)
      pr.print_inline(str);
    else
      pr.print(str);
  }
} TagIntegral;
TagIntegral TagIntegral_constructor() {

}
TagIntegral TagIntegral_constructor(const string_t* name) {

}
TagIntegral TagIntegral_constructor(Integral val, string_t name) {

}
TagIntegral TagIntegral_constructor(Integral val) {
  
}
TagIntegral TagIntegral_constructor() {
  
}
TagIntegral(FILE* buf, const string_t name) :
    TagIntegral::Tag(name, type_name.el) {
  this->decode(buf);
}

typedef TagIntegral<int8_t, TAG_BYTE, "TagByte"> TagByte;
typedef TagIntegral<int16_t, TAG_SHORT, "TagShort"> TagShort;
typedef TagIntegral<int32_t, TAG_INT, "TagInt"> TagInt;
typedef TagIntegral<int64_t, TAG_LONG, "TagLong"> TagLong;


template <std::floating_point Decimal, uint8_t id, _FixedString type_name>
class TagDecimal : public Tag<Decimal, id> {
public:

  TagDecimal() : TagDecimal::Tag(type_name.el) {}
  TagDecimal(string_t name) : TagDecimal::Tag(name, type_name.el) {}
  TagDecimal(Decimal val, string_t name) :
  TagDecimal::Tag(val, name, type_name.el) {}
  TagDecimal(Decimal val) : TagDecimal::Tag(type_name.el) {
    this->val = val;
  }
  TagDecimal(FILE* buf) : TagDecimal::Tag(type_name.el) {
    this->decode(buf);
  }
  TagDecimal(FILE* buf, string_t name) :
  TagDecimal::Tag(name, type_name.el) {
    this->decode(buf);
  }

  void decode(FILE* buf) {
    std::conditional_t<
      std::is_same_v<Decimal, float>, uint32_t , uint64_t
    > tmp;
    buf.read((char *) (&tmp), sizeof(tmp));
    tmp = nbeswap(tmp);
    memcpy(&this->val, &tmp, sizeof(tmp));
  }

  void encode(FILE* buf) const {
    std::conditional_t<
      std::is_same_v<Decimal, float>, uint32_t , uint64_t
    > tmp;
    memcpy(&tmp, &this->val, sizeof(tmp));
    tmp = nbeswap(tmp);
    buf.write((char *) (&tmp), sizeof(tmp));
  }

  void print(NbtPrinter* pr, int pr_inline = 0) const {
    string_t str;
    if(this->name.has_value())
      str = this->type_name + "('" + this->name.value() + "'): " +
      std::to_string(this->val);
    else
      str = this->type_name + "(None): " + std::to_string(this->val);
    if(pr_inline)
      pr.print_inline(str);
    else
      pr.print(str);
  }
};

typedef TagDecimal<float, TAG_FLOAT, "TagFloat"> TagFloat;
typedef TagDecimal<double, TAG_DOUBLE, "TagDouble"> TagDouble;


template <integral T, uint8_t id, _FixedString type_name>
class TagArray : public Tag<std::vector<T>, id> {
public:

  TagArray() : TagArray::Tag(type_name.el) {}
  TagArray(const string_t& name) : TagArray::Tag(name, type_name.el) {}
  TagArray(std::vector<T> val, const string_t& name) :
      TagArray::Tag(val, name, type_name.el) {}
  TagArray(std::initializer_list<T> l) : TagArray::Tag(type_name.el) {
    this->val = l;
  }
  TagArray(std::vector<T> val) : TagArray::Tag(type_name.el) {
    this->val = val;
  }
  TagArray(FILE* buf) : TagArray::Tag(type_name.el) {
    this->decode(buf);
  }
  TagArray(FILE* buf, const string_t& name) :
  TagArray::Tag(name, type_name.el) {
    this->decode(buf);
  }

  void decode(FILE* buf) {
    int32_t len;
    buf.read((char *) (&len), sizeof(len));
    len = nbeswap(len);
    if(len <= 0) return;
    this->val.resize(len);
    for(auto &&el : this->val) {
      buf.read((char *) (&el), sizeof(el));
      el = nbeswap(el);
    }
  }

  void encode(FILE* buf) const {
    int32_t len = nbeswap((int32_t) (this->val.size()));
    buf.write((char *) (&len), sizeof(len));
    for(auto &&el : this->val) {
      auto tmp = nbeswap(el);
      buf.write((char *) (&tmp), sizeof(tmp));
    }
  }

  void print(NbtPrinter* pr, int pr_inline = 0) const {
    string_t str;
    if(this->name.has_value())
      str = this->type_name + "('" + this->name.value() + "'): [";
    else
      str = this->type_name + "(None) : [";
    for(auto &el : this->val)
      str += std::to_string(el) + ", ";
    if(this->val.size())
      str.resize(str.size() - 2);
    str += "]";
    if(pr_inline)
      pr.print_inline(str);
    else
      pr.print(str);
  }

  void print(FILE* os) const {
    NbtPrinter pr(os);
    print(pr);
  }
};

typedef TagArray<int8_t, TAG_BYTE_ARRAY, "TagByteAray"> TagByteArray;
typedef TagArray<int32_t, TAG_INT_ARRAY, "TagIntArray"> TagIntArray;
typedef TagArray<int64_t, TAG_LONG_ARRAY, "TagLongArray"> TagLongArray;

inline void write_string(FILE* buf, const string_t* str) {
  uint16_t len = nbeswap((uint16_t) (str.size()));
  buf.write((char *) (&len), sizeof(len));
  buf.write(str.data(), str.size());
}

inline string_t read_string(FILE* buf) {
    uint16_t len;
    buf.read((char *) (&len), sizeof(len));
    len = nbeswap(len);
    auto tmp = std::make_unique<char[]>(len);
    buf.read(tmp.get(), len);
    return string_t(tmp.get(), len);
}

class TagString : public Tag<string_t, TAG_STRING> {
public:
  TagString() : Tag("TagString") {}
  TagString(const string_t& name) : Tag(name, "TagString") {}
  TagString(const string_t& val, string_t name) :
      Tag(val, name, "TagString") {}
  TagString(FILE* buf) : Tag("TagString") {
    this->decode(buf);
  }
  TagString(FILE* buf, const string_t& name) :
      Tag(name, "TagString") {
    this->decode(buf);
  }

  void decode(FILE* buf) {
    this->val = read_string(buf);
  }

  void encode(FILE* buf) const {
    write_string(buf, this->val);
  }

  void print(NbtPrinter* pr, int pr_inline = 0) const {
    string_t str;
    if(this->name.has_value())
      str = this->type_name + "('" + this->name.value() + "'): '" +
      this->val + "'";
    else
      str = this->type_name + "(None): '" + this->val + "'";
    if(pr_inline)
      pr.print_inline(str);
    else
      pr.print(str);
  }
};

class TagList : public Tag<std::vector<std::unique_ptr<BaseTag>>, TAG_LIST> {
public:
  uint8_t list_id;
  TagList() : Tag("TagList") {}
  TagList(string_t name) : Tag(name, "TagList") {}
  TagList(FILE* buf) : Tag("TagList") {
    this->decode(buf);
  }
  TagList(FILE* buf, const string_t& name) : Tag(name, "TagList") {
    this->decode(buf);
  }
  TagList(const TagList &other) : Tag("TagList") {
    *this = other;
  }
  TagList& operator=(const TagList& other);

  void decode(FILE* buf);
  void encode(FILE* buf) const;

  void print(NbtPrinter* pr, int pr_inline = 0) const {
    auto str = string_t(this->type_name + "('" + this->name.value_or("") +
        "'): " + std::to_string(this->val.size()) +
        ((this->val.size() == 1) ? " entry " : " entries ") + "{");
    if(pr_inline) {
      pr.print_inline(str);
      pr.print("");
      pr.inc_indent(2);
    }
    else
      pr.print(str);
    for(auto &el : this->val) {
      el->print(pr, 1);
      pr.print("");
    }
    if(pr_inline) {
      pr.dec_indent(2);
      pr.print_inline("}");
    }
    else
      pr.print("}");
  }
};

class TagCompound : public Tag
<std::unordered_map<string_t, std::unique_ptr<BaseTag>>, TAG_COMPOUND> {
public:
  TagCompound() : Tag("TagCompound") {}
  TagCompound(const string_t& name) : Tag(name, "TagCompound") {}
  TagCompound(FILE* buf) : Tag("TagCompound") {
    this->decode(buf);
  }
  TagCompound(FILE* buf, const string_t& name) :
      Tag(name, "TagCompound") {
    this->decode(buf);
  }
  TagCompound(const TagCompound &other) : Tag("TagCompound") {
    *this = other;
  }

  TagCompound& operator=(const TagCompound& other) {
    if(this != &other) {
      if(other.name)
        this->name = other.name.value();
      else
        this->name.reset();
      for(auto& el : other.val) {
        switch(el.second->tag_id) {
          case TAG_BYTE:
            this->val.insert({el.first, std::make_unique<TagByte>(
                *(TagByte *) (el.second.get()))});
            break;
          case TAG_SHORT:
            this->val.insert({el.first, std::make_unique<TagShort>(
                *(TagShort *) (el.second.get()))});
            break;
          case TAG_INT:
            this->val.insert({el.first, std::make_unique<TagInt>(
                *(TagInt *) (el.second.get()))});
            break;
          case TAG_LONG:
            this->val.insert({el.first, std::make_unique<TagLong>(
                *(TagLong *) (el.second.get()))});
            break;
          case TAG_FLOAT:
            this->val.insert({el.first, std::make_unique<TagFloat>(
                *(TagFloat *) (el.second.get()))});
            break;
          case TAG_DOUBLE:
            this->val.insert({el.first, std::make_unique<TagDouble>(
                *(TagDouble *) (el.second.get()))});
            break;
          case TAG_BYTE_ARRAY:
            this->val.insert({el.first, std::make_unique<TagByteArray>(
                *(TagByteArray *) (el.second.get()))});
            break;
          case TAG_STRING:
            this->val.insert({el.first, std::make_unique<TagString>(
                *(TagString *) (el.second.get()))});
            break;
          case TAG_LIST:
            this->val.insert({el.first, std::make_unique<TagList>(
                *(TagList *) (el.second.get()))});
            break;
          case TAG_COMPOUND:
            this->val.insert({el.first, std::make_unique<TagCompound>(
                *(TagCompound *) (el.second.get()))});
            break;
          case TAG_INT_ARRAY:
            this->val.insert({el.first, std::make_unique<TagIntArray>(
                *(TagIntArray *) (el.second.get()))});
            break;
          case TAG_LONG_ARRAY:
            this->val.insert({el.first, std::make_unique<TagLongArray>(
                *(TagLongArray *) (el.second.get()))});
            break;
        }
      }
    }
    return *this;
  }

  void decode(FILE* buf) {
    this->val.clear();
    uint8_t tag_type;
    buf.read((char *) (&tag_type), sizeof(tag_type));
    while(tag_type != TAG_END) {
      string_t name = read_string(buf);
      switch (tag_type) {
        case TAG_BYTE:
          this->val.insert({name, std::make_unique<TagByte>(buf, name)});
          break;
        case TAG_SHORT:
          this->val.insert({name, std::make_unique<TagShort>(buf, name)});
          break;
        case TAG_INT:
          this->val.insert({name, std::make_unique<TagInt>(buf, name)});
          break;
        case TAG_LONG:
          this->val.insert({name, std::make_unique<TagLong>(buf, name)});
          break;
        case TAG_FLOAT:
          this->val.insert({name, std::make_unique<TagFloat>(buf, name)});
          break;
        case TAG_DOUBLE:
          this->val.insert({name, std::make_unique<TagDouble>(buf, name)});
          break;
        case TAG_BYTE_ARRAY:
          this->val.insert({name, std::make_unique<TagByteArray>(buf, name)});
          break;
        case TAG_STRING:
          this->val.insert({name, std::make_unique<TagString>(buf, name)});
          break;
        case TAG_LIST:
          this->val.insert({name, std::make_unique<TagList>(buf, name)});
          break;
        case TAG_COMPOUND:
          this->val.insert({name, std::make_unique<TagCompound>(buf, name)});
          break;
        case TAG_INT_ARRAY:
          this->val.insert({name, std::make_unique<TagIntArray>(buf, name)});
          break;
        case TAG_LONG_ARRAY:
          this->val.insert({name, std::make_unique<TagLongArray>(buf, name)});
          break;
      }
      buf.read((char *) (&tag_type), sizeof(tag_type));
    }
  }

  void decode_full(FILE* buf) {
    assert(buf.get() == TAG_COMPOUND);
    this->name = read_string(buf);
    decode(buf);
  }

  void encode(FILE* buf) const {
    for(auto &el : this->val) {
      buf.put(el.second->tag_id);
      write_string(buf, el.second->name.value());
      el.second->encode(buf);
    }
    buf.put(TAG_END);
  }

  void encode_full(FILE* buf) const {
    buf.put(this->tag_id);
    write_string(buf, this->name.value());
    encode(buf);
  }

  void print(NbtPrinter* pr, int pr_inline = 0) const {
    auto str = string_t(this->type_name + "('" + this->name.value_or("") +
        "'): " + std::to_string(this->val.size()) +
        ((this->val.size() == 1) ? " entry " : " entries ") + "{");
    if(pr_inline) {
      pr.print_inline(str);
      pr.print("");
      pr.inc_indent(2);
    }
    else
      pr.print(str);
    for(auto &el : this->val) {
      el.second->print(pr, 1);
      pr.print("");
    }
    if(pr_inline) {
      pr.dec_indent(2);
      pr.print_inline("}");
    }
    else
      pr.print("}");
  }
};

inline TagCompound read_compound(FILE* buf) {
  uint8_t id;
  buf.read((char *) (&id), sizeof(id));
  assert(id == TAG_COMPOUND);
  return TagCompound(buf, read_string(buf));
}

inline void write_compound(FILE* buf, TagCompound &tc) {
  tc.encode_full(buf);
}

inline TagList& TagList::operator=(const TagList& other) {
  if(this != &other) {
    if(other.name)
      this->name = other.name.value();
    else
      this->name.reset();
    switch(other.list_id) {
      case TAG_BYTE:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagByte>(
              *(TagByte *) (el.get())));
        break;
      case TAG_SHORT:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagShort>(
              *(TagShort *) (el.get())));
        break;
      case TAG_INT:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagInt>(
              *(TagInt *) (el.get())));
        break;
      case TAG_LONG:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagLong>(
              *(TagLong *) (el.get())));
        break;
      case TAG_FLOAT:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagFloat>(
              *(TagFloat *) (el.get())));
        break;
      case TAG_DOUBLE:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagDouble>(
              *(TagDouble *) (el.get())));
        break;
      case TAG_BYTE_ARRAY:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagByteArray>(
              *(TagByteArray *) (el.get())));
        break;
      case TAG_STRING:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagString>(
              *(TagString *) (el.get())));
        break;
      case TAG_LIST:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagList>(
              *(TagList *) (el.get())));
        break;
      case TAG_COMPOUND:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagCompound>(
              *(TagCompound *) (el.get())));
        break;
      case TAG_INT_ARRAY:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagIntArray>(
              *(TagIntArray *) (el.get())));
        break;
      case TAG_LONG_ARRAY:
        for(auto &el : other.val)
          this->val.push_back(std::make_unique<TagLongArray>(
              *(TagLongArray *) (el.get())));
        break;
    }
  }
  return *this;
}

inline void TagList::decode(FILE* buf) {
  buf.read((char *) (&list_id), sizeof(list_id));
  int32_t len;
  buf.read((char *) (&len), sizeof(len));
  len = nbeswap(len);
  if(len <= 0)
    return;
  this->val.reserve(len);
  switch (list_id) {
    case TAG_BYTE:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagByte>(buf));
      break;
    case TAG_SHORT:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagShort>(buf));
      break;
    case TAG_INT:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagInt>(buf));
      break;
    case TAG_LONG:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagLong>(buf));
      break;
    case TAG_FLOAT:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagFloat>(buf));
      break;
    case TAG_DOUBLE:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagDouble>(buf));
      break;
    case TAG_BYTE_ARRAY:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagByteArray>(buf));
      break;
    case TAG_STRING:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagString>(buf));
      break;
    case TAG_LIST:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagList>(buf));
      break;
    case TAG_COMPOUND:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagCompound>(buf));
      break;
    case TAG_INT_ARRAY:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagIntArray>(buf));
      break;
    case TAG_LONG_ARRAY:
      for(int i = 0; i < len; i++)
        this->val.push_back(std::make_unique<TagLongArray>(buf));
      break;
  }
}

inline void TagList::encode(FILE* buf) const {
  buf.put(list_id);
  int32_t len = nbeswap((int32_t) (this->val.size()));
  buf.write((char *) (&len), sizeof(len));
  for(auto &el : this->val)
    el->encode(buf);
}

} // namespace NBT
#endif // NBT_HPP
