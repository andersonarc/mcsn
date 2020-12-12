# What are the roots that clutch, what branches grow
# Out of this stony rubbish? Son of man,
# You cannot say, or guess, for you know only
# A heap of broken images, where the sun beats,
# And the dead tree gives no shelter, the cricket no relief
#   - T.S. Eliot, The Waste Land


import minecraft_data
import re
import os

indent = "  "

mcd_type_map = {}
type_pre_definitions = dict(
    char_vector_t="",
    int32_t_vector_t="",
    mc_slot_vector_t="",
    mc_tag_vector_t="",
    mc_item_vector_t="",
    string_t_optional_t="",
    nbt_tag_compound_optional_t="",
    int32_t_optional_t="",
    mc_uuid_optional_t="",
    mc_position_optional_t=""
)
type_definitions = type_pre_definitions.copy()


def mc_data_name(typename):
    def inner(cls):
        mcd_type_map[typename] = cls
        return cls

    return inner


# MCD/Protodef has two elements of note, "fields" and "types"
# "Fields" are JSON objects with the following members:
#   * "name" (optional): Name of the field
#   * "anonymous" (optional): Only present if Name is absent, always "True"
#   * "type": A MCD/Protodef "type"
#
# "Types" are either strings or 2-element JSON arrays
#   * String "types" are a string of the type name
#   * JSON Array "types" have the following elements:
#     * 0: String of the type name, same as String "types"
#     * 1: Type data, a JSON object who's members are type-dependent
#
# Of Note: The only MCD "type" that can contain other MCD "fields" is the
# "container" type. Even though MCD "switches" have a type data member called
# "fields" these are not MCD "fields", they are actually MCD "types".
#
# The following extract functions normalize this format

# Param: MCD "type"
# Returns: Field type, Field data
def extract_type(type_field):
    if isinstance(type_field, str):
        return type_field, []
    return type_field[0], type_field[1]


# Param: MCD "field"
# Returns: Field name, Field type, Field data
def extract_field(packet_field):
    name = packet_field.get('name', '')
    field_type, field_data = extract_type(packet_field['type'])
    return name, field_type, field_data


class generic_type:
    typename = ''
    postfix = ''

    def __init__(self, name, parent, type_data=None, use_compare=False):
        self.name = name
        self.old_name = None
        self.parent = parent
        self.switched = False

        # This is needed for switches because switches suck ass
        # More descriptively, switches need to be able to change the names of
        # fields when they're constructed in order to avoid name collisions. But
        # when merging later sister fields they need to be able to identify
        # duplicate fields that didn't undergo that transformation.
        #
        # To accomplish this, we repurpose the "name" field strictly as a display
        # name, and do comparisons with the "compare_name".
        self.compare_name = name
        self.use_compare = use_compare

    def declaration(self):
        return f"{self.typename} {self.name};",

    def initialization(self, val):
        return f"{self.typename} {self.name} = {val};",

    def encoder(self):
        return f"enc_{self.postfix}(dest, {self.name});",

    def decoder(self):
        if self.name:
            return f"{self.name} = dec_{self.postfix}(src);",
        # Special case for when we need to decode a variable as a parameter
        else:
            return f"dec_{self.postfix}(src)"

    def dec_initialized(self):
        return f"{self.typename} {self.name} = dec_{self.postfix}(src);",

    # This comes up enough to write some dedicated functions for it
    # Conglomerate types take one of two approaches to fundamental types:
    # * Set the field name _every_ time prior to decl/enc/dec
    # * Only change it when needed, and then reset it at the end of the op
    # These support the second workflow
    def temp_name(self, temp):
        if self.old_name is None:
            self.old_name = self.name
        self.name = temp

    def reset_name(self):
        if self.old_name is not None:
            self.name = self.old_name

    def __eq__(self, val):
        if not isinstance(val, type(self)):
            return False
        if hasattr(self, 'typename') and hasattr(val, 'typename'):
            if self.typename != val.typename:
                return False
        if self.use_compare:
            return self.compare_name == val.compare_name
        return self.name == val.name

    def __str__(self):
        return f"{self.typename} {self.name}"


# A "simple" type is one that doesn't need to typedef an additional type in
# order to declare its variable. The only "complex" types are bitfields and
# containers, everything else is considered "simple"
class simple_type(generic_type):
    pass


class numeric_type(simple_type):
    size = 0


# These exist because MCD switches use them. I hate MCD switches
@mc_data_name('void')
class void_type(numeric_type):
    typename = 'void'

    def declaration(self):
        return f"/* '{self.name}' is a void type */",

    def encoder(self):
        return f"/* '{self.name}' is a void type */",

    def decoder(self):
        return f"/* '{self.name}' is a void type */",


@mc_data_name('u8')
class num_u8(numeric_type):
    size = 1
    typename = 'uint8_t'
    postfix = 'byte'


@mc_data_name('i8')
class num_i8(num_u8):
    typename = 'int8_t'


@mc_data_name('bool')
class num_bool(num_u8):
    pass


@mc_data_name('u16')
class num_u16(numeric_type):
    size = 2
    typename = 'uint16_t'
    postfix = 'be16'


@mc_data_name('i16')
class num_i16(num_u16):
    typename = 'int16_t'


@mc_data_name('u32')
class num_u32(numeric_type):
    size = 4
    typename = 'uint32_t'
    postfix = 'be32'


@mc_data_name('i32')
class num_i32(num_u32):
    typename = 'int32_t'


@mc_data_name('u64')
class num_u64(numeric_type):
    size = 8
    typename = 'uint64_t'
    postfix = 'be64'


@mc_data_name('i64')
class num_i64(num_u64):
    typename = 'int64_t'


@mc_data_name('f32')
class num_float(num_u32):
    typename = 'float'
    postfix = 'bef32'


@mc_data_name('f64')
class num_double(num_u64):
    typename = 'double'
    postfix = 'bef64'


# Positions and UUIDs are broadly similar to numeric types
# A position is technically a bitfield but we hide that behind a utility func
@mc_data_name('position')
class num_position(num_u64):
    typename = 'mc_position'
    postfix = 'position'


@mc_data_name('UUID')
class num_uuid(numeric_type):
    size = 16
    typename = 'mc_uuid'
    postfix = 'uuid'


@mc_data_name('varint')
class mc_varint(numeric_type):
    # typename = 'std::int32_t'
    # All varints are varlongs until this gets fixed
    # https://github.com/PrismarineJS/minecraft-data/issues/119
    typename = 'int64_t'
    postfix = 'varint'


@mc_data_name('varlong')
class mc_varlong(numeric_type):
    typename = 'int64_t'
    # Decoding varlongs is the same as decoding varints
    postfix = 'varint'


@mc_data_name('string')
class mc_string(simple_type):
    typename = 'string_t'
    postfix = 'string'


@mc_data_name('buffer')
class mc_buffer(simple_type):
    typename = 'char_vector_t'
    postfix = 'buffer'

    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)
        self.count = mcd_type_map[type_data['countType']].postfix

    def encoder(self):
        return (
            f"enc_{self.count}(dest, {self.name}.size);",
            f"enc_buffer(dest, {self.name});",
        )

    def decoder(self):
        return f"{self.name} = dec_buffer(src, dec_{self.count}(src));",


@mc_data_name('restBuffer')
class mc_rest_buffer(simple_type):
    typename = 'char_vector_t'
    postfix = 'buffer'

    def encoder(self):
        return f"fwrite({self.name}.data, sizeof(char), {self.name}.size, dest);",

    def decoder(self):
        return f"{self.name}.size = 0; /* todo */",
        # return (
        #  f"{self.name} = std::vector<char>(std::istreambuf_iterator<char>(src),",
        #    indent * 2 + f"std::istreambuf_iterator<char>());"
        # )


@mc_data_name('nbt')
class mc_nbt(simple_type):
    typename = 'nbt_tag_compound'

    def encoder(self):
        return f"encode_full(dest, {self.name});",

    def decoder(self):
        return f"decode_full(src, {self.name});",


@mc_data_name('optionalNbt')
class mc_optional_nbt(simple_type):
    # DECLARE(TagCompound) optional
    typename = 'nbt_tag_compound_optional_t'

    def encoder(self):
        return (
            f"if ({self.name}.has_value) {{",
            f"{indent}encode_full(dest, {self.name}.value);",
            f"}} else {{",
            f"{indent}enc_byte(dest, NBT_TAG_END);",
            "}"
        )

    def decoder(self):
        return (
            "if (dec_byte(src) == NBT_TAG_COMPOUND) {",
            f"{indent}{self.name}.has_value = true;",
            f"{indent}{self.name}.value = nbt_read_string(src);",
            "}"
        )


class self_serializing_type(simple_type):
    def encoder(self):
        return f"{self.typename}_encode(dest, &{self.name});",

    def decoder(self):
        return f"{self.typename}_decode(src, &{self.name});",


@mc_data_name('slot')
class mc_slot(self_serializing_type):
    typename = 'mc_slot'


@mc_data_name('minecraft_smelting_format')
class mc_smelting(self_serializing_type):
    typename = 'mc_smelting'


@mc_data_name('entityMetadata')
class mc_metadata(self_serializing_type):
    typename = 'mc_entity_metadata'


# This is not how topBitSetTerminatedArray works, but the real solution is hard
# and this solution is easy. As long as this type is only found in the Entity
# Equipment packet we're going to stick with this solution
@mc_data_name('topBitSetTerminatedArray')
class mc_entity_equipment(self_serializing_type):
    typename = 'mc_entity_equipment'


@mc_data_name('particleData')
class mc_particle(self_serializing_type):
    typename = 'mc_particle'

    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)
        self.id_field = type_data['compareTo']

    def decoder(self):
        return f"{self.typename}_decode(src, &{self.name}, (mc_particle_type) this->{self.id_field});",


class vector_type(simple_type):
    element = ''
    count = mc_varint

    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)
        self.depth = 0
        p = parent
        while not isinstance(p, packet):
            self.depth += 1
            p = p.parent

    def encoder(self):
        iterator = f"i{self.depth}"
        return (
            f"enc_{self.count.postfix}(dest, {self.name}.size);",
            f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{",
            f"{indent}{self.element}_encode(dest, &{self.name}.data[{iterator}]);",
            "}"
        )

    def decoder(self):
        iterator = f"i{self.depth}"
        return (
            f"{self.name}.size = dec_{self.count.postfix}(src);",
            f"{self.name}.data = ({self.element}*) malloc({self.name}.size * sizeof({self.element}));",
            f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{",
            f"{indent}{self.element}_decode(src, &{self.name}.data[{iterator}]);",
            "}"
        )


@mc_data_name('ingredient')
class mc_ingredient(vector_type):
    element = 'mc_slot'
    typename = 'mc_slot_vector_t'


@mc_data_name('tags')
class mc_tags(vector_type):
    element = 'mc_tag'
    typename = 'mc_tag_vector_t'


@mc_data_name('option')
class mc_option(simple_type):
    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)
        f_type, f_data = extract_type(type_data)
        self.field = mcd_type_map[f_type](f"{name}", self, f_data)

        if isinstance(self.field, simple_type):
            self.typename = f"{self.field.typename}_optional_t"
        else:
            self.typename = f"{self.name}_type_optional_t"

    def declaration(self):
        if isinstance(self.field, simple_type):
            type_definitions[self.typename] = [f"optional_typedef({self.field.typename})"]
            return super().declaration()
        self.field.name = f"{self.name}_type"
        type_definitions[self.typename] = self.field.typedef()
        type_definitions[self.typename].append(f"optional_typedef({self.field.name})")
        return [f"{self.typename} {self.name};"]

    def encoder(self):
        self.field.name = f"{self.name}.value"
        return [
            f"enc_byte(dest, {self.name}.has_value);",
            f"if ({self.name}.has_value) {{",
            *(indent + line for line in self.field.encoder()),
            "}"
        ]

    def decoder(self):
        ret = [f"if (dec_byte(src)) {{"]
        if isinstance(self.field, numeric_type) or type(self.field) in (mc_string,
                                                                        mc_buffer, mc_rest_buffer):
            self.field.name = self.name
        # todo check origin
        ret.append(f"{indent}{self.name}.has_value = true;")
        self.field.name = f"{self.name}.value"
        ret.extend(indent + line for line in self.field.decoder())
        ret.append('}')
        return ret


class complex_type(generic_type):
    def declaration(self):
        if self.name:
            return [
                "struct {",
                *(indent + l for f in self.fields for l in f.declaration()),
                f"}} {self.name};"
            ]
        return [l for f in self.fields for l in f.declaration()]

    def typedef(self):
        ret = self.declaration()
        ret[0] = f"typedef {ret[0]}"
        return ret


def get_storage(numbits):
    if numbits <= 8:
        return 8
    elif numbits <= 16:
        return 16
    elif numbits <= 32:
        return 32
    else:
        return 64


@mc_data_name('bitfield')
class mc_bitfield(complex_type):
    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)
        lookup_unsigned = {
            8: num_u8,
            16: num_u16,
            32: num_u32,
            64: num_u64
        }
        lookup_signed = {
            8: num_i8,
            16: num_i16,
            32: num_i32,
            64: num_i64
        }
        self.fields = []
        self.extra_data = []
        self.field_sizes = {}

        total = 0
        for idx, field in enumerate(type_data):
            total += field['size']
            if field['name'] in ('_unused', 'unused'):
                continue
            self.field_sizes[field['name']] = field['size']
            shift = 0
            for temp in type_data[idx + 1:]:
                shift += temp['size']
            self.extra_data.append(((1 << field['size']) - 1, shift, field['size'],
                                    field['signed']))
            numbits = get_storage(field['size'])
            if field['signed']:
                self.fields.append(lookup_signed[numbits](field['name'], self))
            else:
                self.fields.append(lookup_unsigned[numbits](field['name'], self))

        self.storage = lookup_unsigned[total](f"{name}_", self)
        self.size = total // 8

    def encoder(self):
        ret = [*self.storage.initialization("0")]
        name = f"{self.name}." if self.name else ""
        for idx, field in enumerate(self.fields):
            mask, shift, size, signed = self.extra_data[idx]
            shift_str = f" << {shift}" if shift else ""
            ret.append(f"{self.storage.name} |= ((({self.storage.typename}) {name}{field.name}) & {mask}){shift_str};")
        ret.extend(self.storage.encoder())
        return ret

    def decoder(self):
        ret = [*self.storage.dec_initialized()]
        name = f"{self.name}." if self.name else ""
        for idx, field in enumerate(self.fields):
            mask, shift, size, signed = self.extra_data[idx]
            shift_str = f" >> {shift}" if shift else ""
            ret.append(f"{name}{field.name} = ({self.storage.name}{shift_str}) & {mask};")
            if signed:
                ret.extend((
                    f"if ({name}{field.name} & (1UL << {size - 1})) {{",
                    f"{indent}{name}{field.name} -= 1UL << {size};",
                    "}"
                ))
        return ret


# Whatever you think an MCD "switch" is you're probably wrong
@mc_data_name('switch')
class mc_switch(simple_type):
    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)

        if not (hasattr(self.parent, 'name') and self.parent.name):
            self.parent.name = 'this->'

        self.compareTo = type_data['compareTo']
        # Unions are a sum type that differs based on conditions. They are a true
        # "switch". All others are here because MCData is bad and I hate it.
        self.is_union = False

        # A non-union switch consists of one or more consistent types, some or all
        # of which are decoded under different conditions. They act like unions
        # except they leave behind Null switches if they're multi-element.
        #
        # Null switches are fields that got merged into a sister switch, they're
        # carcasses that don't do anything. They're found when MCD encodes a
        # multi-element switch. All the elements except the "lead" element become
        # null switches.
        self.null_switch = False
        # Inverses are default present _except_ for certain conditions, and they're
        # bugged on tons of corner cases that thankfully aren't present in MCD
        # right now.
        self.is_inverse = False
        self.is_str_switch = False

        self.lead_sister = None
        self.fields = []
        self.field_dict = {}

        # Find out if all possible values are the same type, in which case we're
        # not a union. And if all the types are void we're an inverse.
        values = type_data['fields'].values()
        if all(map(lambda x: x == 'void', values)):
            self.is_inverse = True
            f_type, f_data = extract_type(type_data['default'])
            self.fields.append(mcd_type_map[f_type](name, self, f_data))
        else:
            values = list(filter(lambda x: x != 'void', values))
            self.is_union = not all(x == values[0] for x in values)

        # In the case we're not a union, we need to check for sister switches
        if not self.is_union:
            for field in parent.fields:
                if isinstance(field, mc_switch) and field.compareTo == self.compareTo:
                    self.null_switch = True
                    self.lead_sister = field.lead_sister if field.null_switch else field
                    self.lead_sister.merge(name, type_data['fields'], self.is_inverse,
                                           type_data.get('default', None))
                    return

        self.process_fields(self.name, type_data['fields'])

    def declaration(self):
        if self.null_switch:
            return []
        return [l for f in self.fields for l in f.declaration()]

    def code_fields(self, ret, fields, encode=True):
        if hasattr(self.parent, 'name') and self.parent.name.endswith("->"):
            suffix = ""
        else:
            suffix = "."
        for field in fields:
            if hasattr(self.parent, 'name') and self.parent.name:
                field.temp_name(f"{self.parent.name}{suffix}{field.name}")
            if encode:
                ret.extend(f"{indent}{l}" for l in field.encoder())
            else:
                ret.extend(f"{indent}{l}" for l in field.decoder())
            field.reset_name()
        return ret

    def inverse(self, comp, encode=True):
        if len(self.field_dict.items()) == 1:
            case, _ = next(iter(self.field_dict.items()))
            ret = [f"if ({comp} != {case}) {{"]
        else:
            return '// Multi-Condition Inverse Not Yet Implemented',
        self.code_fields(ret, self.fields, encode)
        ret.append("}")
        return ret

    # Special case for single condition switches, which are just optionals
    # mascarading as switches
    def optional(self, comp, encode=True):
        case, fields = next(iter(self.field_dict.items()))
        ret = []
        if case == "true":
            ret.append(f"if ({comp}) {{")
        elif case == "false":
            ret.append(f"if (!{comp}) {{")
        elif case.isdigit():
            ret.append(f"if ({comp} == {case}) {{")
        else:
            ret.append(f"if (!strcmp({comp}, {case})) {{")
        self.code_fields(ret, fields, encode)
        ret.append("}")
        return ret

    def str_switch(self, comp, encode=True):
        items = list(self.field_dict.items())
        case, fields = items[0]
        ret = [f"if (!strcmp({comp}, {case})) {{"]
        self.code_fields(ret, fields, encode)
        for case, fields in items[1:]:
            ret.append(f"}} else if (!strcmp({comp}, {case})) {{")
            self.code_fields(ret, fields, encode)
        ret.append("}")
        return ret

    def union_multi(self, comp, encode=True):
        if len(self.field_dict.items()) == 1:
            return self.optional(comp, encode)
        ret = [f"switch ({comp}) {{"]
        for case, fields in self.field_dict.items():
            ret.append(f"{indent}case {case}:")
            tmp = []
            self.code_fields(tmp, fields, encode)
            ret.extend(indent + l for l in tmp)
            ret.append(f"{indent * 2}break;")
        ret.append("}")
        return ret

    # compareTo is a mystical format, its powers unmatched. We don't try to parse
    # it. Instead, we just count the number of hierarchy levels it ascends with
    # ".." and move up the container hierarchy using that. If we hit the packet,
    # we abandon ship and assume the path is absolute.
    def get_compare(self):
        comp = self.compareTo.replace('../', '').replace('..', '').replace('/', '.')
        p = self.parent
        for i in range(self.compareTo.count('..')):
            while not (isinstance(p, complex_type) or isinstance(p, packet)):
                p = p.parent
            if isinstance(p, packet):
                break
            else:
                p = p.parent
        while not (isinstance(p, complex_type) or isinstance(p, packet)):
            p = p.parent
        if not isinstance(p, packet):
            comp = f"{p.name}.{comp}"
        if not comp.startswith("this->"):
            comp = "this->" + comp
        return comp

    def encoder(self):
        comp = self.get_compare()
        if self.null_switch:
            return []
        if self.is_inverse:
            return self.inverse(comp)
        if self.is_str_switch:
            return self.str_switch(comp)
        return self.union_multi(comp)

    def decoder(self):
        comp = self.get_compare()
        if self.null_switch:
            return []
        if self.is_inverse:
            return self.inverse(comp, False)
        if self.is_str_switch:
            return self.str_switch(comp, False)
        return self.union_multi(comp, False)

    def process_fields(self, name, fields):
        for key, field_info in fields.items():
            if not key.isdigit() and key not in ("true", "false"):
                key = f'"{key}"'
                self.is_str_switch = True
            f_type, f_data = extract_type(field_info)
            field = mcd_type_map[f_type](name, self, f_data, True)
            if field not in self.fields:
                if not isinstance(field, void_type):
                    self.fields.append(field)
            if not self.is_inverse and isinstance(field, void_type):
                continue
            if key in self.field_dict:
                self.field_dict[key].append(field)
            else:
                self.field_dict[key] = [field]

            # Look for name collisions and fix them. This is unnecessarily convoluted
            # and ripe for refactoring.
            has_dupes = []
            for field in self.fields:
                if len(list(filter(lambda x: x.compare_name == field.compare_name,
                                   self.fields))) > 1:
                    has_dupes.append(field)
            # If we're an anonymous switch don't bother, we'll break container fields
            if has_dupes and name:
                for key, fields in self.field_dict.items():
                    # Is this fine? Is it guaranteed there will be (at least) one and
                    # only one dupe for each key? I think so?
                    same = next(x for x in fields if x in has_dupes)
                    if self.is_str_switch:
                        tmp = key.replace('"', '').replace(':', '_')
                        same.name = tmp
                        # As a weird consequence of the merging strategy, we can end up in
                        # a situation with two tags that compare the same but have
                        # different names. These tags have the same type, but because
                        # string switch fields have "convenient" names they have different
                        # names. Here we restore these "lost" tags to the field list
                        if all(map(lambda x: x.name != same.name, self.fields)):
                            self.fields.append(same)
                    else:
                        same.name = f"{name}_{key}"

            if not self.is_str_switch:
                self.field_dict = {key: self.field_dict[key] for key in
                                   sorted(self.field_dict)}

    def merge(self, sis_name, sis_fields, is_inverse, default):
        if is_inverse:
            f_type, f_data = extract_type(default)
            self.fields.append(mcd_type_map[f_type](sis_name, self, f_data))
        self.process_fields(sis_name, sis_fields)

    def __eq__(self, value):
        if not super().__eq__(value) or len(self.fields) != len(value.fields):
            return False
        return all([i == j for i, j in zip(self.fields, value.fields)])


# Arrays come in three flavors. Fixed length, length prefixed, and foreign
# field
@mc_data_name('array')
class mc_array(simple_type):
    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)
        f_type, f_data = extract_type(type_data['type'])
        self.field = mcd_type_map[f_type]('', self, f_data)
        self.depth = 0
        p = parent
        while not isinstance(p, packet):
            self.depth += 1
            p = p.parent
        self.packet = p

        self.is_fixed = False
        self.is_prefixed = False
        self.is_foreign = False
        self.count = None
        if "countType" in type_data:
            self.is_prefixed = True
            self.count = mcd_type_map[type_data['countType']]('', self, [])
        elif isinstance(type_data['count'], int):
            self.is_fixed = True
            self.count = type_data['count']
        else:
            self.is_foreign = True
            self.count = type_data['count']

        if isinstance(self.field, simple_type):
            self.f_type = self.field.typename
        else:
            self.f_type = self.field.name
        self.typename = f"{self.f_type}_vector_t"

    def declaration(self):
        ret = []
        if isinstance(self.field, simple_type):
            self.f_type = self.field.typename
            typename = f"{self.f_type}_vector_t"
            type_definitions[typename] = [f"vector_typedef({self.f_type})"]
        else:
            self.field.name = f"{self.packet.packet_name}_{self.name}_type"
            typename = f"{self.field.name}_vector_t"
            type_definitions[typename] = self.field.typedef()
            self.f_type = self.field.name
            type_definitions[f"{self.field.name}_vector_t"].append(f"vector_typedef({self.f_type})")
        if self.is_fixed:
            ret.append(f"{typename} {self.name}; /* {self.count}> */")
        else:
            ret.append(f"{typename} {self.name};")
        return ret

    def fixed(self, encode=True):
        iterator = f"i{self.depth}"
        self.field.name = f"{self.name}.data[{iterator}]"
        ret = [f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{"]
        if encode:
            ret.extend(indent + l for l in self.field.encoder())
        else:
            ret.extend(indent + l for l in self.field.decoder())
        ret.append("}")
        return ret

    def prefixed_encode(self):
        iterator = f"i{self.depth}"
        self.count.name = f"{self.name}.size"
        self.field.name = f"{self.name}.data[{iterator}]"
        return [
            *self.count.encoder(),
            f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{",
            *(indent + l for l in self.field.encoder()),
            "}"
        ]

    def prefixed_decode(self):
        iterator = f"i{self.depth}"
        self.count.name = ''
        self.field.name = f"{self.name}.data[{iterator}]"
        return [
            f"{self.name}.size = {self.count.decoder()};",
            f"{self.name}.data = ({self.f_type}*) malloc({self.name}.size * sizeof({self.f_type}));",
            f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{",
            *(indent + l for l in self.field.decoder()),
            "}"
        ]

    # Identical to switches' compareTo
    def get_foreign(self):
        comp = self.count.replace('../', '').replace('..', '').replace('/', '.')
        p = self.parent
        for i in range(self.count.count('..')):
            while not (isinstance(p, complex_type) or isinstance(p, packet)):
                p = p.parent
            if isinstance(p, packet):
                break
            else:
                p = p.parent
        while not (isinstance(p, complex_type) or isinstance(p, packet)):
            p = p.parent
        if not isinstance(p, packet):
            comp = f"{p.name}.{comp}"
        return comp

    def foreign_encode(self):
        iterator = f"i{self.depth}"
        self.field.name = f"{self.name}.data[{iterator}]"
        return [
            f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{",
            *(indent + l for l in self.field.encoder()),
            "}"
        ]

    def foreign_decode(self):
        iterator = f"i{self.depth}"
        self.field.name = f"{self.name}.data[{iterator}]"
        return [
            f"{self.name}.size = {self.get_foreign()};",
            f"{self.name}.data = ({self.f_type}*) malloc({self.name}.size * sizeof({self.f_type}));",
            f"for (int {iterator} = 0; {iterator} < {self.name}.size; {iterator}++) {{",
            *(indent + l for l in self.field.decoder()),
            "}"
        ]

    def encoder(self):
        if self.is_fixed:
            return self.fixed()
        if self.is_prefixed:
            return self.prefixed_encode()
        return self.foreign_encode()

    def decoder(self):
        if self.is_fixed:
            return self.fixed(False)
        if self.is_prefixed:
            return self.prefixed_decode()
        return self.foreign_decode()


# Not a container_type because containers_types sometimes have trivial storage
# requirements. Actual containers always have non-trivial storage, which makes
# them a pure complex type
@mc_data_name('container')
class mc_container(complex_type):
    def __init__(self, name, parent, type_data, use_compare=False):
        super().__init__(name, parent, type_data, use_compare)

        self.fields = []
        for field_info in type_data:
            f_name, f_type, f_data = extract_field(field_info)
            self.fields.append(mcd_type_map[f_type](f_name, self, f_data))

    def encoder(self):
        ret = []
        if self.name:
            if self.name.endswith("->"):
                suffix = ""
            else:
                suffix = "."
            for field in self.fields:
                field.temp_name(f"{self.name}{suffix}{field.name}")
                ret.extend(field.encoder())
                field.reset_name()
        else:
            for field in self.fields:
                ret.extend(field.encoder())
        return ret

    def decoder(self):
        ret = []
        if self.name:
            if self.name.endswith("->"):
                suffix = ""
            else:
                suffix = "."
            for field in self.fields:
                field.temp_name(f"{self.name}{suffix}{field.name}")
                ret.extend(field.decoder())
                field.reset_name()
        else:
            for field in self.fields:
                ret.extend(field.decoder())
        return ret

    def __eq__(self, value):
        if not super().__eq__(value) or len(self.fields) != len(value.fields):
            return False
        return all([i == j for i, j in zip(self.fields, value.fields)])


def get_decoder(field):
    field.temp_name("this->" + field.name)
    string = field.decoder()
    field.reset_name()
    return string


def get_encoder(field):
    field.temp_name("this->" + field.name)
    string = field.encoder()
    field.reset_name()
    return string


class packet:
    def __init__(self, state, direction, packet_id, packet_name, data):
        self.state = state
        self.direction = "Clientbound" if direction == "toClient" else "Serverbound"
        self.packet_id = packet_id
        self.packet_name = packet_name
        self.data = data
        self.class_name = f"{self.direction}{self.packet_name}"

        self.fields = []
        for data_field in data:
            f_name, f_type, f_data = extract_field(data_field)
            self.fields.append(mcd_type_map[f_type](f_name, self, f_data))

    def declaration(self):
        return [
            f"typedef struct {self.class_name} {{",
            f"{indent}PACKET",
            *(indent + l for f in self.fields for l in f.declaration()),
            f"}} {self.class_name};",
            f"void encode(FILE* dest);",
            f"void decode(FILE* src);",
        ]

    def encoder(self):
        return [
            f"void {self.class_name}_encode(FILE* dest, {self.class_name}* this) {{",
            *(indent + l for f in self.fields for l in get_encoder(f)),
            "}"
        ]

    def decoder(self):
        return [
            f"void {self.class_name}_decode(FILE* src, {self.class_name}* this) {{",
            *(indent + l for f in self.fields for l in get_decoder(f)),
            "}"
        ]


mc_states = "handshaking", "status", "login", "play"
mc_directions = "toClient", "toServer"

warning = (
    "/*",
    "  This file was generated by mcd2c.py",
    "  It should not be edited by hand.",
    "*/",
    "",
)

first_cap_re = re.compile('(.)([A-Z][a-z]+)')
all_cap_re = re.compile('([a-z0-9])([A-Z])')


def to_enum(name, direction, state):
    s1 = first_cap_re.sub(r'\1_\2', name)
    name = all_cap_re.sub(r'\1_\2', s1).upper()
    d = "SB" if direction == "toServer" else "CB"
    st = {"handshaking": "HS", "status": "ST", "login": "LG",
          "play": "PL"}[state]
    return f"{d}_{st}_{name}"


def to_camel_case(string):
    return string.title().replace('_', '')


def extract_infos_from_listing(listing):
    ret = []
    # Why this seemingly random position inside the full listing? Because mcdata
    # hates you.
    for p_id, p_name in listing['types']['packet'][1][0]['type'][1]['mappings'].items():
        ret.append((int(p_id, 0), to_camel_case(p_name), f"packet_{p_name}"))
    return ret


def run(version):
    mcd = minecraft_data(version)
    version = version.replace('.', '_')
    proto = mcd.protocol
    header_upper = [
        *warning,
        "#ifndef PROTOCOL_H",
        "#define PROTOCOL_H",
        "",
        "#include <stdint.h>",
        "#include <malloc.h>",
        "#include <string.h>",
        "#include <stdio.h>",
        "#include \"particle_types.h\"",
        "#include \"translation_utils.h\"",
        "#include \"data_utils.h\"",
        "//#include \"nbt.h\"",
        "",
        f"#define MC_PROTOCOL_VERSION {mcd.version['version']}",
        "",
        "typedef enum packet_direction {",
        "  SERVERBOUND,",
        "  CLIENTBOUND,",
        "  DIRECTION_MAX",
        "} packet_direction;",
        "",
        "typedef enum packet_state {",
        "  HANDSHAKING,",
        "  STATUS,",
        "  LOGIN,",
        "  PLAY,",
        "  STATE_MAX",
        "} packet_state;",
        "",
        "#define PACKET                       \\",
        "  packet_state packet_state;         \\",
        "  packet_direction packet_direction; \\",
        "  int32_t packet_id;                 \\",
        "const string_t packet_name; ",
        "",
        "typedef struct Packet {",
        "  PACKET",
        "} Packet;"
            ]
    header_lower = [
        "extern const char* serverbound_handshaking_cstrings["
        "SERVERBOUND_HANDSHAKING_MAX];",
        "extern const char* clientbound_status_cstrings[CLIENTBOUND_STATUS_MAX];",
        "extern const char* serverbound_status_cstrings[SERVERBOUND_STATUS_MAX];",
        "extern const char* clientbound_login_cstrings[CLIENTBOUND_LOGIN_MAX];",
        "extern const char* serverbound_login_cstrings[SERVERBOUND_LOGIN_MAX];",
        "extern const char* clientbound_play_cstrings[CLIENTBOUND_PLAY_MAX];",
        "extern const char* serverbound_play_cstrings[SERVERBOUND_PLAY_MAX];",
        "",
        "extern const char **protocol_cstrings[STATE_MAX][DIRECTION_MAX];",
        "extern const int protocol_max_ids[STATE_MAX][DIRECTION_MAX];",
        "",
        "Packet* make_packet(packet_state state, packet_direction dir, int packet_id);",
        ""
    ]
    impl_upper = [
        *warning,
        f"#include \"protocol.h\"",
        ""
    ]
    impl_lower = []
    particle_header = [
        *warning,
        f"// For MCD version {version.replace('_', '.')}",
        "#ifndef PARTICLE_TYPES_H",
        "#define PARTICLE_TYPES_H",
        "",
        "typedef enum mc_particle_type {"
    ]
    particle_header.extend(f"{indent}PARTICLE_{x.upper()}," for x in mcd.particles_name)
    particle_header[-1] = particle_header[-1][:-1]
    particle_header += ["} mc_particle_type;", "#endif /* PARTICLE_TYPES_H */", ""]
    packet_enum = {}
    packets = {}

    for state in mc_states:
        packet_enum[state] = {}
        packets[state] = {}
        for direction in mc_directions:
            packet_enum[state][direction] = []
            packets[state][direction] = []
            packet_info_list = extract_infos_from_listing(proto[state][direction])
            for info in packet_info_list:
                packet_data = proto[state][direction]['types'][info[2]][1]
                if info[1] != "LegacyServerListPing":
                    packet_id = to_enum(info[1], direction, state)
                    packet_enum[state][direction].append(packet_id)
                else:
                    packet_id = info[0]
                pak = packet(state, direction, packet_id, info[1], packet_data)
                if info[1] != "LegacyServerListPing":
                    packets[state][direction].append(pak)
                header_lower += pak.declaration()
                header_lower.append('')

                impl_lower += pak.encoder()
                impl_lower += pak.decoder()
                impl_lower.append('')

    for state in mc_states:
        for direction in mc_directions:
            dr = "clientbound" if direction == "toClient" else "serverbound"
            packet_enum[state][direction].append(f"{dr.upper()}_{state.upper()}_MAX")
            header_upper.append(f"enum {dr}_{state}_ids {{")
            header_upper.extend(f"{indent}{l}," for l in packet_enum[state][direction])
            header_upper[-1] = header_upper[-1][:-1]
            header_upper.extend(("};", ""))

    make_packet = [
        "Packet* make_packet(packet_state state, packet_direction dir, int packet_id) {",
        "  /*switch(state) {"
    ]

    for state in mc_states:
        make_packet.append(f"{indent * 2}case {state.upper()}:")
        make_packet.append(f"{indent * 3}switch(dir) {{")
        for direction in mc_directions:
            dr = "CLIENTBOUND" if direction == "toClient" else "SERVERBOUND"
            make_packet.append(f"{indent * 4}case {dr}:")
            make_packet.append(f"{indent * 5}switch(packet_id) {{")
            for pak in packets[state][direction]:
                make_packet.append(f"{indent * 6} case {pak.packet_id}:")
                make_packet.append(f"{indent * 7} return "
                                   f"{pak.class_name};")
            make_packet.append(f"{indent * 6}default:")
            make_packet.append(f"{indent * 7}runtime_error(\"Invalid "
                               "Packet Id\");")
            make_packet.append(f"{indent * 5}}}")
        make_packet.append(f"{indent * 4}default:")
        make_packet.append(f"{indent * 5}runtime_error(\"Invalid "
                           "Packet Direction\");")
        make_packet.append(f"{indent * 3}}}")
    make_packet.append(f"{indent * 2}default:")
    make_packet.append(f"{indent * 3}runtime_error(\"Invalid "
                       "Packet State\");")
    make_packet.extend((f"{indent}}}*/ return NULL;", "}", ""))

    for state in mc_states:
        for direction in mc_directions:
            if packets[state][direction]:
                dr = "clientbound" if direction == "toClient" else "serverbound"
                impl_upper.append(f"const char* {dr}_{state}_cstrings[] = {{")
                for pak in packets[state][direction]:
                    impl_upper.append(f"{indent}\"{pak.class_name}\",")
                impl_upper[-1] = impl_upper[-1][:-1]
                impl_upper.extend(("};", ""))

    impl_upper += [
        "const char **protocol_cstrings[STATE_MAX][DIRECTION_MAX] = {",
        f"{indent}{{serverbound_handshaking_cstrings}},",
        f"{indent}{{serverbound_status_cstrings, clientbound_status_cstrings}},",
        f"{indent}{{serverbound_login_cstrings, clientbound_login_cstrings}},",
        f"{indent}{{serverbound_play_cstrings, clientbound_play_cstrings}}",
        "};",
        "",
        "const int protocol_max_ids[STATE_MAX][DIRECTION_MAX] = {",
        f"{indent}{{SERVERBOUND_HANDSHAKING_MAX, CLIENTBOUND_HANDSHAKING_MAX}},",
        f"{indent}{{SERVERBOUND_STATUS_MAX, CLIENTBOUND_STATUS_MAX}},",
        f"{indent}{{SERVERBOUND_LOGIN_MAX, CLIENTBOUND_LOGIN_MAX}},",
        f"{indent}{{SERVERBOUND_PLAY_MAX, CLIENTBOUND_PLAY_MAX}}",
        "};",
        "",
    ]

    for type_pre_definition in type_pre_definitions:
        type_definitions[type_pre_definition] = ""

    for type_definition in type_definitions:
        header_upper.extend(type_definitions[type_definition])
        header_upper.append(' ')

    header = header_upper + header_lower + ["#endif", ""]
    impl = impl_upper + impl_lower + make_packet + [""]

    with open("particle_types.h", "w") as f:
        f.write('\n'.join(particle_header))
    with open("src/mcpacket/protocol.c", "w") as f:
        f.write('\n'.join(impl))
    with open("protocol.h", "w") as f:
        f.write('\n'.join(header))


if __name__ == "__main__":
    run(os.environ['CPACKET_MINECRAFT_VERSION'])
