/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// independent from idl_parser, since this code is not needed for most clients

#include <functional>
#include "flatbuffers/code_generators.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"
#if defined(FLATBUFFERS_CPP98_STL)
#include <cctype>
#endif  // defined(FLATBUFFERS_CPP98_STL)

namespace flatbuffers {

using CodeblockFunction = std::function<void(int, CodeWriter &)>;
// These arrays need to correspond to the IDLOptions::k enum.

struct LanguageParameters {
  IDLOptions::Language language;
  // Whether function names in the language typically start with uppercase.
  bool first_camel_upper;
  std::string file_extension;
  std::string string_type;
  std::string bool_type;
  std::string open_curly;
  std::string accessor_type;
  std::string const_decl;
  std::string unsubclassable_decl;
  std::string enum_decl;
  std::string enum_separator;
  std::string getter_prefix;
  std::string getter_suffix;
  std::string inheritance_marker;
  std::string namespace_ident;
  std::string namespace_begin;
  std::string namespace_end;
  std::string set_bb_byteorder;
  std::string get_bb_position;
  std::string get_fbb_offset;
  std::string accessor_prefix;
  std::string accessor_prefix_static;
  std::string optional_suffix;
  std::string includes;
  std::string class_annotation;
  std::string generated_type_annotation;
  CommentConfig comment_config;
  const FloatConstantGenerator *float_gen;
};

static TypedFloatConstantGenerator JavaFloatGen("Double.", "Float.", "NaN",
                                                "POSITIVE_INFINITY",
                                                "NEGATIVE_INFINITY");

static const LanguageParameters lang_ = {
    IDLOptions::kJava,
    false,       // first_camel_upper
    ".kt",       // file_extension
    "String",    // string_type
    "boolean ",  // bool_type
    " {\n",      // open_curly
    "class ",    // accessor_type
    " final ",   // const_decl
    "final ",    // unsubclassable_decl
    "class ",    // enum_decl
    "\n",        // enum_separator
    "()",        // getter_prefix
    "",          // getter_suffix
    " : ",       // inheritance_marker
    "package ",
    "",
    "",
    "_bb.order(ByteOrder.LITTLE_ENDIAN); ",  // set_bb_byteorder
    "position()",                            // get_bb_position
    "offset()",
    "",  // accessor_prefix
    "",  // accessor_prefix_static
    "",  // optional_suffix
    "import java.nio.*\nimport java.lang.*\nimport "
    "java.util.*\nimport com.google.flatbuffers.*\n",
    "",
    "",
    {
        "/**",
        " *",
        " */",
    },
    &JavaFloatGen};

namespace kotlin {
class KotlinGenerator : public BaseGenerator {
 public:
  KotlinGenerator(const Parser &parser, const std::string &path,
                  const std::string &file_name)
      : BaseGenerator(parser, path, file_name, "", "."),
        cur_name_space_(nullptr) {}

  CodeWriter code_;
  
  KotlinGenerator &operator=(const KotlinGenerator &);
  bool generate() {
    std::string one_file_code;
    cur_name_space_ = parser_.current_namespace_;

    for (auto it = parser_.enums_.vec.begin(); it != parser_.enums_.vec.end();
         ++it) {
      std::string enumcode;
      auto &enum_def = **it;
      if (!parser_.opts.one_file) cur_name_space_ = enum_def.defined_namespace;
      GenEnum(enum_def, &enumcode);
      if (parser_.opts.one_file) {
        one_file_code += enumcode;
      } else {
        if (!SaveType(enum_def.name, *enum_def.defined_namespace, enumcode,
                      false))
          return false;
      }
    }

    for (auto it = parser_.structs_.vec.begin();
         it != parser_.structs_.vec.end(); ++it) {
      std::string declcode;
      auto &struct_def = **it;
      if (!parser_.opts.one_file)
        cur_name_space_ = struct_def.defined_namespace;
      GenStruct(struct_def, &declcode);
      if (parser_.opts.one_file) {
        one_file_code += declcode;
      } else {
        if (!SaveType(struct_def.name, *struct_def.defined_namespace, declcode,
                      true))
          return false;
      }
    }

    if (parser_.opts.one_file) {
      return SaveType(file_name_, *parser_.current_namespace_, one_file_code,
                      true);
    }
    return true;
  }

  // Save out the generated code for a single class while adding
  // declaration boilerplate.
  bool SaveType(const std::string &defname, const Namespace &ns,
                const std::string &classcode, bool needs_includes) const {
    if (!classcode.length()) return true;

    std::string code;
    if (lang_.language == IDLOptions::kCSharp) {
      code =
          "// <auto-generated>\n"
          "//  " +
          std::string(FlatBuffersGeneratedWarning()) +
          "\n"
          "// </auto-generated>\n\n";
    } else {
      code = "// " + std::string(FlatBuffersGeneratedWarning()) + "\n\n";
    }

    std::string namespace_name = FullNamespace(".", ns);
    if (!namespace_name.empty()) {
      code += lang_.namespace_ident + namespace_name + lang_.namespace_begin;
      code += "\n\n";
    }
    if (needs_includes) {
      code += lang_.includes;
      if (parser_.opts.gen_nullable) {
        code += "\nimport javax.annotation.Nullable\n";
      }
      code += lang_.class_annotation;
    }
    if (parser_.opts.gen_generated) {
      code += lang_.generated_type_annotation;
    }
    code += classcode;
    if (!namespace_name.empty()) code += lang_.namespace_end;
    auto filename = NamespaceDir(ns) + defname + lang_.file_extension;
    return SaveFile(filename.c_str(), code, false);
  }

  const Namespace *CurrentNameSpace() const { return cur_name_space_; }

  std::string GenNullableAnnotation(const Type &t) const {
    return parser_.opts.gen_nullable &&
                   !IsScalar(DestinationType(t, true).base_type)
               ? " @Nullable "
               : "";
  }

  static bool IsEnum(const Type &type) {
    return type.enum_def != nullptr && IsInteger(type.base_type);
  }

  static std::string GenTypeBasic(const Type &type) {
    // clang-format off
  static const char * const java_typename[] = {
    #define FLATBUFFERS_TD(ENUM, IDLTYPE, \
        CTYPE, JTYPE, GTYPE, NTYPE, PTYPE, RTYPE) \
        #JTYPE,
      FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
    #undef FLATBUFFERS_TD
  };
    return java_typename[type.base_type];
    
  }

  std::string GenTypePointer(const Type &type) const {
    switch (type.base_type) {
      case BASE_TYPE_STRING:
        return lang_.string_type;
      case BASE_TYPE_VECTOR:
        return GenTypeGet(type.VectorType());
      case BASE_TYPE_STRUCT:
        return WrapInNameSpace(*type.struct_def);
      case BASE_TYPE_UNION:
        // Unions in C# use a generic Table-derived type for better type safety
        if (lang_.language == IDLOptions::kCSharp) return "TTable";
        FLATBUFFERS_FALLTHROUGH();  // else fall thru
      default:
        return "Table";
    }
  }

  std::string GenTypeGet(const Type &type) const {
    return IsScalar(type.base_type) ? GenTypeBasic(type) : GenTypePointer(type);
  }

  // Find the destination type the user wants to receive the value in (e.g.
  // one size higher signed types for unsigned serialized values in Java).
  static Type DestinationType(const Type &type, bool vectorelem) {
    if (lang_.language != IDLOptions::kJava) return type;
    switch (type.base_type) {
      // We use int for both uchar/ushort, since that generally means less
      // casting than using short for uchar.
      case BASE_TYPE_UCHAR:
        return Type(BASE_TYPE_INT);
      case BASE_TYPE_USHORT:
        return Type(BASE_TYPE_INT);
      case BASE_TYPE_UINT:
        return Type(BASE_TYPE_LONG);
      case BASE_TYPE_VECTOR:
        if (vectorelem) return DestinationType(type.VectorType(), vectorelem);
        FLATBUFFERS_FALLTHROUGH();  // else fall thru
      default:
        return type;
    }
  }

  static std::string GenOffsetConstruct(const StructDef &struct_def,
                                 const std::string &variable_name) {
    return variable_name;
  }

  // Generate destination type name
  std::string GenTypeNameDest(const Type &type) const {
    return GenTypeGet(DestinationType(type, true));
  }

  // Mask to turn serialized value into destination type value.
  std::string DestinationMask(const Type &type, bool vectorelem) const {
    switch (type.base_type) {
      case BASE_TYPE_UCHAR:
        return " & 0xFF";
      case BASE_TYPE_USHORT:
        return " & 0xFFFF";
      case BASE_TYPE_UINT:
        return " & 0xFFFFFFFFL";
      case BASE_TYPE_VECTOR:
        if (vectorelem) return DestinationMask(type.VectorType(), vectorelem);
        FLATBUFFERS_FALLTHROUGH();  // else fall thru
      default:
        return "";
    }
  }

  // Casts necessary to correctly read serialized data
  std::string DestinationCast(const Type &type) const {
    if (type.base_type == BASE_TYPE_VECTOR) {
      return DestinationCast(type.VectorType());
    } else if (type.base_type == BASE_TYPE_UINT){
        return "(long)";
    }
    return "";
  }

  // Cast statements for mutator method parameters.
  // In Java, parameters representing unsigned numbers need to be cast down to
  // their respective type. For example, a long holding an unsigned int value
  // would be cast down to int before being put onto the buffer. In C#, one cast
  // directly cast an Enum to its underlying type, which is essential before
  // putting it onto the buffer.
  static std::string SourceCast(const Type &type, bool castFromDest) {
    if (type.base_type == BASE_TYPE_VECTOR) {
      return SourceCast(type.VectorType(), castFromDest);
    } else {
      switch (lang_.language) {
        case IDLOptions::kJava:
          if (castFromDest) {
            if (type.base_type == BASE_TYPE_UINT)
              return "(int)";
            else if (type.base_type == BASE_TYPE_USHORT)
              return "(short)";
            else if (type.base_type == BASE_TYPE_UCHAR)
              return "(byte)";
          }
          break;
        case IDLOptions::kCSharp:
          if (IsEnum(type)) return "(" + GenTypeBasic(type) + ")";
          break;
        default:
          break;
      }
    }
    return "";
  }

  static std::string SourceCast(const Type &type) {
    return SourceCast(type, true);
  }

  std::string SourceCastBasic(const Type &type, bool castFromDest) const {
    return IsScalar(type.base_type) ? SourceCast(type, castFromDest) : "";
  }

  std::string SourceCastBasic(const Type &type) const {
    return SourceCastBasic(type, true);
  }

  std::string GenEnumDefaultValue(const FieldDef &field) const {
    auto &value = field.value;
    FLATBUFFERS_ASSERT(value.type.enum_def);
    auto &enum_def = *value.type.enum_def;
    auto enum_val = enum_def.FindByValue(value.constant);
    return enum_val ? (WrapInNameSpace(enum_def) + "." + enum_val->name)
                    : value.constant;
  }

  std::string GenDefaultValue(const FieldDef &field,
                              bool enableLangOverrides) const {
    auto &value = field.value;
    if (enableLangOverrides) {
      // handles both enum case and vector of enum case
      if (lang_.language == IDLOptions::kCSharp &&
          value.type.enum_def != nullptr &&
          value.type.base_type != BASE_TYPE_UNION) {
        return GenEnumDefaultValue(field);
      }
    }

    auto longSuffix = lang_.language == IDLOptions::kJava ? "L" : "";
    switch (value.type.base_type) {
      case BASE_TYPE_BOOL:
        return value.constant == "0" ? "false" : "true";
      case BASE_TYPE_ULONG: {
        if (lang_.language != IDLOptions::kJava) return value.constant;
        // Converts the ulong into its bits signed equivalent
        uint64_t defaultValue = StringToUInt(value.constant.c_str());
        return NumToString(static_cast<int64_t>(defaultValue)) + longSuffix;
      }
      case BASE_TYPE_UINT:
      case BASE_TYPE_LONG:
        return value.constant + longSuffix;
      default:
        if (IsFloat(value.type.base_type))
          return lang_.float_gen->GenFloatConstant(field);
        else
          return value.constant;
    }
  }

  std::string GenDefaultValue(const FieldDef &field) const {
    return GenDefaultValue(field, true);
  }

  std::string GenDefaultValueBasic(const FieldDef &field,
                                   bool enableLangOverrides) const {
    auto &value = field.value;
    if (!IsScalar(value.type.base_type)) {
      if (enableLangOverrides) {
        if (lang_.language == IDLOptions::kCSharp) {
          switch (value.type.base_type) {
            case BASE_TYPE_STRING:
              return "default(StringOffset)";
            case BASE_TYPE_STRUCT:
              return "default(Offset<" +
                     WrapInNameSpace(*value.type.struct_def) + ">)";
            case BASE_TYPE_VECTOR:
              return "default(VectorOffset)";
            default:
              break;
          }
        }
      }
      return "0";
    }
    return GenDefaultValue(field, enableLangOverrides);
  }

  std::string GenDefaultValueBasic(const FieldDef &field) const {
    return GenDefaultValueBasic(field, true);
  }

  void GenEnum(EnumDef &enum_def, std::string *code_ptr) const {
    std::string &code = *code_ptr;
    if (enum_def.generated) return;

    // Generate enum definitions of the form:
    // public static (final) int name = value;
    // In Java, we use ints rather than the Enum feature, because we want them
    // to map directly to how they're used in C/C++ and file formats.
    // That, and Java Enums are expensive, and not universally liked.
    GenComment(enum_def.doc_comment, code_ptr, &lang_.comment_config);
    code += lang_.enum_decl + enum_def.name;
    if (lang_.language == IDLOptions::kCSharp) {
      code += lang_.inheritance_marker +
              GenTypeBasic(enum_def.underlying_type);
    }
    code += lang_.open_curly;
    if (lang_.language == IDLOptions::kJava) {
      code += "  private " + enum_def.name + "() { }\n";
    }
    for (auto it = enum_def.Vals().begin(); it != enum_def.Vals().end(); ++it) {
      auto &ev = **it;
      GenComment(ev.doc_comment, code_ptr, &lang_.comment_config, "  ");
      if (lang_.language != IDLOptions::kCSharp) {
        code += "  static";
        code += lang_.const_decl;
        code += GenTypeBasic(enum_def.underlying_type);
      }
      code += " " + ev.name + " = ";
      code += enum_def.ToString(ev);
      code += lang_.enum_separator;
    }

    // Generate a generate string table for enum values.
    // We do not do that for C# where this functionality is native.
      // Problem is, if values are very sparse that could generate really big
      // tables. Ideally in that case we generate a map lookup instead, but for
      // the moment we simply don't output a table at all.
      auto range = enum_def.Distance();
      // Average distance between values above which we consider a table
      // "too sparse". Change at will.
      static const uint64_t kMaxSparseness = 5;
      if (range / static_cast<uint64_t>(enum_def.size()) < kMaxSparseness) {
        code += "\n  static";
        code += lang_.const_decl;
        code += lang_.string_type;
        code += "[] names = { ";
        auto val = enum_def.Vals().front();
        for (auto it = enum_def.Vals().begin(); it != enum_def.Vals().end();
             ++it) {
          auto ev = *it;
          for (auto k = enum_def.Distance(val, ev); k > 1; --k)
            code += "\"\", ";
          val = ev;
          code += "\"" + (*it)->name + "\", ";
        }
        code += "};\n\n";
        code += "  static ";
        code += lang_.string_type;
        code += " " + MakeCamel("name", lang_.first_camel_upper);
        code += "(int e) { return names[e";
        if (enum_def.MinValue()->IsNonZero())
          code += " - " + enum_def.MinValue()->name;
        code += "]; }\n";
      }

    // Close the class
    code += "}";
    // Java does not need the closing semi-colon on class definitions.
    code += "\n\n";
  }

  // Returns the function name that is able to read a value of the given type.
  std::string GenGetter(const Type &type) const {
    switch (type.base_type) {
      case BASE_TYPE_STRING:
        return lang_.accessor_prefix + "__string";
      case BASE_TYPE_STRUCT:
        return lang_.accessor_prefix + "__struct";
      case BASE_TYPE_UNION:
        return lang_.accessor_prefix + "__union";
      case BASE_TYPE_VECTOR:
        return GenGetter(type.VectorType());
      default: {
        std::string getter = lang_.accessor_prefix + "bb.get";
        if (type.base_type == BASE_TYPE_BOOL) {
          getter = "0!=" + getter;
        } else if (GenTypeBasic(type) != "byte") {
          getter += MakeCamel(GenTypeBasic(type));
        }
        return getter;
      }
    }
  }

  // Returns the function name that is able to read a value of the given type.
  std::string GenGetterForLookupByKey(flatbuffers::FieldDef *key_field,
                                      const std::string &data_buffer,
                                      const char *num = nullptr) const {
    auto type = key_field->value.type;
    auto dest_mask = DestinationMask(type, true);
    auto dest_cast = DestinationCast(type);
    auto getter = data_buffer + ".get";
    if (GenTypeBasic(type) != "byte") {
      getter += MakeCamel(GenTypeBasic(type));
    }
    getter = dest_cast + getter + "(" + GenOffsetGetter(key_field, num) + ")" +
             dest_mask;
    return getter;
  }

  // Direct mutation is only allowed for scalar fields.
  // Hence a setter method will only be generated for such fields.
  std::string GenSetter(const Type &type) const {
    if (IsScalar(type.base_type)) {
      std::string setter = lang_.accessor_prefix + "bb.put";
      if (GenTypeBasic(type) != "byte" &&
          type.base_type != BASE_TYPE_BOOL) {
        setter += MakeCamel(GenTypeBasic(type));
      }
      return setter;
    } else {
      return "";
    }
  }

  // Returns the method name for use with add/put calls.
  static std::string GenMethod(const Type &type) {
    return IsScalar(type.base_type) ? MakeCamel(GenTypeBasic(type))
                                    : (IsStruct(type) ? "Struct" : "Offset");
  }

  // Recursively generate arguments for a constructor, to deal with nested
  // structs.
  static void GenStructArgs(const StructDef &struct_def, CodeWriter &code,
                     const char *nameprefix) {
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      auto &field = **it;
      if (IsStruct(field.value.type)) {
        // Generate arguments for a struct inside a struct. To ensure names
        // don't clash, and to make it obvious these arguments are constructing
        // a nested struct, prefix the name with the field name.
        GenStructArgs(*field.value.type.struct_def, code,
                      (nameprefix + (field.name + "_")).c_str());
      } else {
        code += std::string(", ") + nameprefix + "\\";
        code += MakeCamel(field.name, lang_.first_camel_upper) + ": \\";
        code += GenTypeBasic(DestinationType(field.value.type, false)) + "\\";
      }
    }
  }

  // Recusively generate struct construction statements of the form:
  // builder.putType(name);
  // and insert manual padding.
  static void GenStructBody(const StructDef &struct_def, CodeWriter &code,
                     const char *nameprefix) {
    code += "    builder.prep(\\";
    code += NumToString(struct_def.minalign) + ", \\";
    code += NumToString(struct_def.bytesize) + ")";
    for (auto it = struct_def.fields.vec.rbegin();
         it != struct_def.fields.vec.rend(); ++it) {
      auto &field = **it;
      if (field.padding) {
        code += "    builder.pad(\\";
        code += NumToString(field.padding) + ")";
      }
      if (IsStruct(field.value.type)) {
        GenStructBody(*field.value.type.struct_def, code,
                      (nameprefix + (field.name + "_")).c_str());
      } else {
        code += "    builder.put\\";
        code += GenMethod(field.value.type) + "(\\";
        code += SourceCast(field.value.type) + "\\";
        auto argname =
            nameprefix + MakeCamel(field.name, lang_.first_camel_upper);
        code += argname + "\\";
        code += ")";
      }
    }
  }

  std::string GenByteBufferLength(const char *bb_name) const {
    std::string bb_len = bb_name;
    if (lang_.language == IDLOptions::kCSharp)
      bb_len += ".Length";
    else
      bb_len += ".capacity()";
    return bb_len;
  }

  std::string GenOffsetGetter(flatbuffers::FieldDef *key_field,
                              const char *num = nullptr) const {
    std::string key_offset = "";
    key_offset += lang_.accessor_prefix_static + "__offset(" +
                  NumToString(key_field->value.offset) + ", ";
    if (num) {
      key_offset += num;
      key_offset +=
          (lang_.language == IDLOptions::kCSharp ? ".Value, builder.DataBuffer)"
                                                 : ", _bb)");
    } else {
      key_offset += GenByteBufferLength("bb");
      key_offset += " - tableOffset, bb)";
    }
    return key_offset;
  }

  std::string GenLookupKeyGetter(flatbuffers::FieldDef *key_field) const {
    std::string key_getter = "      ";
    key_getter += "int tableOffset = " + lang_.accessor_prefix_static;
    key_getter += "__indirect(vectorLocation + 4 * (start + middle)";
    key_getter += ", bb);\n      ";
    if (key_field->value.type.base_type == BASE_TYPE_STRING) {
      key_getter += "int comp = " + lang_.accessor_prefix_static;
      key_getter += "compareStrings(";
      key_getter += GenOffsetGetter(key_field);
      key_getter += ", byteKey, bb)\n";
    } else {
      auto get_val = GenGetterForLookupByKey(key_field, "bb");
      if (lang_.language == IDLOptions::kCSharp) {
        key_getter += "int comp = " + get_val + ".CompareTo(key)\n";
      } else {
        key_getter += GenTypeNameDest(key_field->value.type) + " val = ";
        key_getter += get_val + "\n";
        key_getter += "      int comp = val > key ? 1 : val < key ? -1 : 0\n";
      }
    }
    return key_getter;
  }

  std::string GenKeyGetter(flatbuffers::FieldDef *key_field) const {
    std::string key_getter = "";
    auto data_buffer =
        (lang_.language == IDLOptions::kCSharp) ? "builder.DataBuffer" : "_bb";
    if (key_field->value.type.base_type == BASE_TYPE_STRING) {
      if (lang_.language == IDLOptions::kJava) key_getter += " return ";
      key_getter += lang_.accessor_prefix_static;
      key_getter += "compareStrings(";
      key_getter += GenOffsetGetter(key_field, "o1") + ", ";
      key_getter += GenOffsetGetter(key_field, "o2") + ", " + data_buffer + ")";
    } else {
      auto field_getter = GenGetterForLookupByKey(key_field, data_buffer, "o1");
      if (lang_.language == IDLOptions::kCSharp) {
        key_getter += field_getter;
        field_getter = GenGetterForLookupByKey(key_field, data_buffer, "o2");
        key_getter += ".CompareTo(" + field_getter + ")";
      } else {
        key_getter +=
            "\n    " + GenTypeNameDest(key_field->value.type) + " val_1 = ";
        key_getter +=
            field_getter + "\n    " + GenTypeNameDest(key_field->value.type);
        key_getter += " val_2 = ";
        field_getter = GenGetterForLookupByKey(key_field, data_buffer, "o2");
        key_getter += field_getter + "\n";
        key_getter +=
            "    return val_1 > val_2 ? 1 : val_1 < val_2 ? -1 : 0;\n ";
      }
    }
    return key_getter;
  }

  void GenStruct(StructDef &struct_def, std::string *code_ptr) const {
    if (struct_def.generated) return;
    std::string &code = *code_ptr;

    // Generate a struct accessor class, with methods of the form:
    // public type name() { return bb.getType(i + offset); }
    // or for tables of the form:
    // public type name() {
    //   int o = __offset(offset); return o != 0 ? bb.getType(o + i) : default;
    // }
    GenComment(struct_def.doc_comment, code_ptr, &lang_.comment_config);
    code += lang_.unsubclassable_decl;
    code += lang_.accessor_type + struct_def.name;
    code += lang_.inheritance_marker;
    code += struct_def.fixed ? "Struct" : "Table";
    code += lang_.open_curly;

    if (!struct_def.fixed) {
      if (parser_.root_struct_def_ == &struct_def) {
        if (parser_.file_identifier_.length()) {
          // Check if a buffer has the identifier.
          code += "  static ";
          code += lang_.bool_type + struct_def.name;
          code += "BufferHasIdentifier(ByteBuffer _bb) { return ";
          code += lang_.accessor_prefix_static + "__has_identifier(_bb, \"";
          code += parser_.file_identifier_;
          code += "\") }\n";
        }
      }
    }
    // Generate the __init method that sets the field in a pre-existing
    // accessor object. This is to allow object reuse.
    code += "  void __init(int _i, ByteBuffer _bb) ";
    code += "{ " + lang_.accessor_prefix + "bb_pos = _i; ";
    code += lang_.accessor_prefix + "bb = _bb; ";
    if (!struct_def.fixed && lang_.language == IDLOptions::kJava) {
      code += lang_.accessor_prefix +
              "vtable_start = " + lang_.accessor_prefix + "bb_pos - ";
      code += lang_.accessor_prefix + "bb.getInt(";
      code += lang_.accessor_prefix + "bb_pos); " + lang_.accessor_prefix +
              "vtable_size = ";
      code += lang_.accessor_prefix + "bb.getShort(";
      code += lang_.accessor_prefix + "vtable_start); ";
    }
    code += "}\n";
    code += "  " + struct_def.name + " __assign(int _i, ByteBuffer _bb) ";
    code += "{ __init(_i, _bb); return this; }\n\n";
    
    
    // GENERATE GETTERS
    auto fields_vec = struct_def.fields.vec;
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      auto &field = **it;
      if (field.deprecated) continue;
      GenComment(field.doc_comment, code_ptr, &lang_.comment_config, "  ");
      std::string field_name_camel = MakeCamel(field.name);
      std::string type_name = GenTypeGet(field.value.type);
      std::string type_name_dest = GenTypeNameDest(field.value.type);
      std::string conditional_cast = "";
      std::string optional = "";
      std::string dest_mask = DestinationMask(field.value.type, true);
      std::string dest_cast = DestinationCast(field.value.type);
      std::string src_cast = SourceCast(field.value.type);
      std::string method_start =
          "  " +
          (field.required ? "" : GenNullableAnnotation(field.value.type)) +
          type_name_dest + optional + " " +
          MakeCamel(field.name);
      std::string obj = "obj";

      // Most field accessors need to retrieve and test the field offset first,
      // this is the prefix code for that:
      auto offset_prefix = "val o = __offset(" +
                           NumToString(field.value.offset) +
                           "); return o != 0 ? ";
      // Generate the accessors that don't do object reuse.
      if (field.value.type.base_type == BASE_TYPE_STRUCT) {
          // Calls the accessor that takes an accessor object with a new object.
          // e.g. public Vec3 pos() { return pos(new Vec3()); }
          // val pos
          //     get() {
          //         return pos(Vec3())
          //     }
          code += GenerateGetter(1, field_name_camel, type_name, 
                                 [&](int ilvl, CodeWriter &writer){
              writer += t(ilvl) + "return {{name}}({{type}}())";
          });  
      } else if (field.value.type.base_type == BASE_TYPE_VECTOR &&
                 field.value.type.element == BASE_TYPE_STRUCT) {
        // Accessors for vectors of structs also take accessor objects, this
        // generates a variant without that argument.
        // e.g. fun weapons(j: Int) : Weapon { 
        //          return weapons(Weapon(), j) }
        //      }
        code += GenerateFun(1,field_name_camel, "j: Int", type_name_dest, 
                            [&](int ilvl, CodeWriter &writer){
            writer += t(ilvl) + "return {{return_type}}(), j)";
         });
      }
      std::string getter = dest_cast + GenGetter(field.value.type);
      //code += method_start;
      std::string default_cast = "";
      std::string member_suffix = "; ";
      if (IsScalar(field.value.type.base_type)) {
        code += lang_.getter_prefix;
        member_suffix += lang_.getter_suffix;
        if (struct_def.fixed) {
            code += GenerateGetter(1, field_name_camel, type_name, 
                                   [&](int ilvl, CodeWriter &writer){
                writer += t(ilvl) + "return " + getter + "(bb_pos + " + NumToString(field.value.offset) + ")" + dest_mask;
            }); 
        } else {
            code += GenerateGetter(1, field_name_camel, type_name, 
                                   [&](int ilvl, CodeWriter &writer){
                writer += t(ilvl) + "val o = __offset(" + NumToString(field.value.offset) + ")";
                writer += t(ilvl) + "return if(o != 0) " + getter + "(o + bb_pos)" + dest_mask + " else " + default_cast + GenDefaultValue(field);
            });
        }
      } else {
        switch (field.value.type.base_type) {
          case BASE_TYPE_STRUCT:
            
            if (struct_def.fixed) {
                // create getter with object reuse
                // e.g.
                //  fun pos(obj: Vec3) : Vec3? {
                //      return obj.__assign(bb_pos + 4, bb)
                //  }
                // ? adds nullability annotation
                code += GenerateFun(1, field_name_camel, "obj: " + type_name , 
                                    type_name + "?", 
                                    [&](int ilvl, CodeWriter &writer){
                    writer.SetValue("offset", NumToString(field.value.offset));
                    code += t(ilvl) + "return obj."
                                      "__assign(bb_pos + {{offset}}, bb)";
                });
            } else {
              // create getter with object reuse
              // e.g.
              //  fun pos(obj: Vec3) : Vec3? {
              //      val o = __offset(4)
              //      return if(o != 0) {
              //          obj.__assign(o + bb_pos, bb)
              //      else {
              //          null
              //      }
              //  }
              // ? adds nullability annotation
              code += GenerateFun(1, field_name_camel, "obj: " + type_name , 
                                  type_name + "?", 
                                  [&](int ilvl, CodeWriter &writer){
                  auto fixed = field.value.type.struct_def->fixed;
                  auto direct = "o + bb_pos";
                  auto indirect = "__indirect(o + bb_pos)";
                  
                  writer.SetValue("seek", fixed ? direct : indirect);
                  writer.SetValue("offset", NumToString(field.value.offset));
                  
                  writer += t(ilvl) + "val o = __offset({{offset}})";
                  writer += t(ilvl) + "return if (o != 0) {";
                  writer += t(ilvl+1) + "obj.__assign({{seek}}, bb)";
                  writer += t(ilvl) + "else {";
                  writer += t(ilvl+1) + "null";
                  writer += t(ilvl) + "}";
              });
            }
            break;
          case BASE_TYPE_STRING:
            // create getter with object reuse
            // e.g.
            //  fun pos(obj: Vec3) : Vec3? {
            //      val o = __offset(4)
            //      return if(o != 0) {
            //          obj.__assign(o + bb_pos, bb)
            //      else {
            //          null
            //      }
            //  }
            // ? adds nullability annotation
            code += GenerateGetter(1, field_name_camel, type_name + "?", 
                                [&](int ilvl, CodeWriter &writer){
                writer.SetValue("offset", NumToString(field.value.offset));
                
                writer += t(ilvl) + "val o = __offset({{offset}})";
                writer += t(ilvl) + "return if (o != 0) __string(o + bb_pos)"
                                    " else null";
            });
            break;
          case BASE_TYPE_VECTOR: {
            code += "(" + type_name + " obj" + ")";
            auto vectortype = field.value.type.VectorType();
            code += "(";
            if (vectortype.base_type == BASE_TYPE_STRUCT) {
              code += type_name + " obj, ";
              getter = obj + ".__assign";
            } else if (vectortype.base_type == BASE_TYPE_UNION) {
              code += type_name + " obj, ";
            }
            code += "int j)";
            const auto body = offset_prefix + conditional_cast + getter + "(";
            if (vectortype.base_type == BASE_TYPE_UNION) {
              code += body + "obj, ";
            } else {
              code += body;
            }
            auto index = lang_.accessor_prefix + "__vector(o) + j * " +
                         NumToString(InlineSize(vectortype));
            if (vectortype.base_type == BASE_TYPE_STRUCT) {
              code += vectortype.struct_def->fixed
                          ? index
                          : lang_.accessor_prefix + "__indirect(" + index + ")";
              code += ", " + lang_.accessor_prefix + "bb";
            } else if (vectortype.base_type == BASE_TYPE_UNION) {
              code += index + " - " + lang_.accessor_prefix + "bb_pos";
            } else {
              code += index;
            }
            code += ")" + dest_mask + " : ";

            code +=
                field.value.type.element == BASE_TYPE_BOOL
                    ? "false"
                    : (IsScalar(field.value.type.element) ? default_cast + "0"
                                                          : "null");
            break;
          }
          case BASE_TYPE_UNION:
            code += "(" + type_name + " obj" + ")";
            code += "(" + type_name + " obj)" + offset_prefix + getter;
            code += "(obj, o) : null";
            break;
          default:
            FLATBUFFERS_ASSERT(0);
        }
      }
      code += member_suffix;
      code += "}\n";
      if (field.value.type.base_type == BASE_TYPE_VECTOR) {
        code += "  int " + MakeCamel(field.name, lang_.first_camel_upper);
        code += "Length";
        code += lang_.getter_prefix;
        code += offset_prefix;
        code += lang_.accessor_prefix + "__vector_len(o) : 0; ";
        code += lang_.getter_suffix;
        code += "}\n";
        // See if we should generate a by-key accessor.
        if (field.value.type.element == BASE_TYPE_STRUCT &&
            !field.value.type.struct_def->fixed) {
          auto &sd = *field.value.type.struct_def;
          auto &fields = sd.fields.vec;
          for (auto kit = fields.begin(); kit != fields.end(); ++kit) {
            auto &key_field = **kit;
            if (key_field.key) {
              auto qualified_name = WrapInNameSpace(sd);
              code += "  " + qualified_name + lang_.optional_suffix + " ";
              code += MakeCamel(field.name, lang_.first_camel_upper) + "ByKey(";
              code += GenTypeNameDest(key_field.value.type) + " key)";
              code += offset_prefix;
              code += qualified_name + ".__lookup_by_key(";
              if (lang_.language == IDLOptions::kJava) code += "null, ";
              code += lang_.accessor_prefix + "__vector(o), key, ";
              code += lang_.accessor_prefix + "bb) : null; ";
              code += "}\n";

              code += "  " + qualified_name + lang_.optional_suffix + " ";
              code += MakeCamel(field.name, lang_.first_camel_upper) + "ByKey(";
              code += qualified_name + lang_.optional_suffix + " obj, ";
              code += GenTypeNameDest(key_field.value.type) + " key)";
              code += offset_prefix;
              code += qualified_name + ".__lookup_by_key(obj, ";
              code += lang_.accessor_prefix + "__vector(o), key, ";
              code += lang_.accessor_prefix + "bb) : null; ";
              code += "}\n";
              break;
            }
          }
        }
      }
      // Generate a ByteBuffer accessor for strings & vectors of scalars.
      if ((field.value.type.base_type == BASE_TYPE_VECTOR &&
           IsScalar(field.value.type.VectorType().base_type)) ||
          field.value.type.base_type == BASE_TYPE_STRING) {
        code += "  ByteBuffer ";
        code += MakeCamel(field.name, lang_.first_camel_upper);
        code += "AsByteBuffer() { return ";
        code += lang_.accessor_prefix + "__vector_as_bytebuffer(";
        code += NumToString(field.value.offset) + ", ";
        code += NumToString(field.value.type.base_type == BASE_TYPE_STRING
                                ? 1
                                : InlineSize(field.value.type.VectorType()));
        code += ") }\n";
        code += "  ByteBuffer ";
        code += MakeCamel(field.name, lang_.first_camel_upper);
        code += "InByteBuffer(ByteBuffer _bb) { return ";
        code += lang_.accessor_prefix + "__vector_in_bytebuffer(_bb, ";
        code += NumToString(field.value.offset) + ", ";
        code += NumToString(field.value.type.base_type == BASE_TYPE_STRING
                                ? 1
                                : InlineSize(field.value.type.VectorType()));
        code += ") }\n";
      }
      // generate object accessors if is nested_flatbuffer
      if (field.nested_flatbuffer) {
        auto nested_type_name = WrapInNameSpace(*field.nested_flatbuffer);
        auto nested_method_name =
            MakeCamel(field.name, lang_.first_camel_upper) + "As" +
            nested_type_name;
        auto get_nested_method_name = nested_method_name;

        code += "  " + nested_type_name + lang_.optional_suffix + " ";
        code += nested_method_name + "() { return ";
        code +=
            get_nested_method_name + "(new " + nested_type_name + "()) }\n";
        code += "  " + nested_type_name + lang_.optional_suffix + " ";
        code += get_nested_method_name + "(";
        code += nested_type_name + " obj";
        code += ") { int o = " + lang_.accessor_prefix + "__offset(";
        code += NumToString(field.value.offset) + "); ";
        code += "return o != 0 ? " + conditional_cast + obj + ".__assign(";
        code += lang_.accessor_prefix;
        code += "__indirect(" + lang_.accessor_prefix + "__vector(o)), ";
        code += lang_.accessor_prefix + "bb) : null; }\n";
      }
      // Generate mutators for scalar fields or vectors of scalars.
      if (parser_.opts.mutable_buffer) {
        auto underlying_type = field.value.type.base_type == BASE_TYPE_VECTOR
                                   ? field.value.type.VectorType()
                                   : field.value.type;
        // Boolean parameters have to be explicitly converted to byte
        // representation.
        auto setter_parameter = underlying_type.base_type == BASE_TYPE_BOOL
                                    ? "(byte)(" + field.name + " ? 1 : 0)"
                                    : field.name;
        auto mutator_prefix = MakeCamel("mutate", lang_.first_camel_upper);
        // A vector mutator also needs the index of the vector element it should
        // mutate.
        auto mutator_params =
            (field.value.type.base_type == BASE_TYPE_VECTOR ? "(int j, "
                                                            : "(") +
            GenTypeNameDest(underlying_type) + " " + field.name + ") { ";
        auto setter_index =
            field.value.type.base_type == BASE_TYPE_VECTOR
                ? lang_.accessor_prefix + "__vector(o) + j * " +
                      NumToString(InlineSize(underlying_type))
                : (struct_def.fixed
                       ? lang_.accessor_prefix + "bb_pos + " +
                             NumToString(field.value.offset)
                       : "o + " + lang_.accessor_prefix + "bb_pos");
        if (IsScalar(field.value.type.base_type) ||
            (field.value.type.base_type == BASE_TYPE_VECTOR &&
             IsScalar(field.value.type.VectorType().base_type))) {
          code += "  ";
          code += struct_def.fixed ? "void " : lang_.bool_type;
          code += mutator_prefix + MakeCamel(field.name, true);
          code += mutator_params;
          if (struct_def.fixed) {
            code += GenSetter(underlying_type) + "(" + setter_index + ", ";
            code += src_cast + setter_parameter + ") }\n";
          } else {
            code += "int o = " + lang_.accessor_prefix + "__offset(";
            code += NumToString(field.value.offset) + ")";
            code += " if (o != 0) { " + GenSetter(underlying_type);
            code += "(" + setter_index + ", " + src_cast + setter_parameter +
                    "); return true; } else { return false; } }\n";
          }
        }
      }
    }
    code += "\n";
    flatbuffers::FieldDef *key_field = nullptr;
    if (!struct_def.fixed) {
      // Generate a method that creates a table in one go. This is only possible
      // when the table has no struct fields, since those have to be created
      // inline, and there's no way to do so in Java.
      bool has_no_struct_fields = true;
      int num_fields = 0;
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        auto &field = **it;
        if (field.deprecated) continue;
        if (IsStruct(field.value.type)) {
          has_no_struct_fields = false;
        } else {
          num_fields++;
        }
      }
      // JVM specifications restrict default constructor params to be < 255.
      // Longs and doubles take up 2 units, so we set the limit to be < 127.
      if (has_no_struct_fields && num_fields && num_fields < 127) {
        // Generate a table constructor of the form:
        // public static int createName(FlatBufferBuilder builder, args...)
        code += "  static Int ";
        code += "create" + struct_def.name;
        code += "(FlatBufferBuilder builder";
        for (auto it = struct_def.fields.vec.begin();
             it != struct_def.fields.vec.end(); ++it) {
          auto &field = **it;
          if (field.deprecated) continue;
          code += ",\n      ";
          code += GenTypeBasic(DestinationType(field.value.type, false));
          code += " ";
          code += field.name;
          if (!IsScalar(field.value.type.base_type)) code += "Offset";

          // Java doesn't have defaults, which means this method must always
          // supply all arguments, and thus won't compile when fields are added.
          if (lang_.language != IDLOptions::kJava) {
            code += " = ";
            code += GenDefaultValueBasic(field);
          }
        }
        code += ") {\n    builder.";
        code += "startObject(";
        code += NumToString(struct_def.fields.vec.size()) + ")\n";
        for (size_t size = struct_def.sortbysize ? sizeof(largest_scalar_t) : 1;
             size; size /= 2) {
          for (auto it = struct_def.fields.vec.rbegin();
               it != struct_def.fields.vec.rend(); ++it) {
            auto &field = **it;
            if (!field.deprecated &&
                (!struct_def.sortbysize ||
                 size == SizeOf(field.value.type.base_type))) {
              code += "    " + struct_def.name + ".";
              code += "add";
              code += MakeCamel(field.name) + "(builder, " + field.name;
              if (!IsScalar(field.value.type.base_type)) code += "Offset";
              code += ")\n";
            }
          }
        }
        code += "    return " + struct_def.name + ".";
        code += "end" + struct_def.name;
        code += "(builder)\n  }\n\n";
      }
      // Generate a set of static methods that allow table construction,
      // of the form:
      // public static void addName(FlatBufferBuilder builder, short name)
      // { builder.addShort(id, name, default); }
      // Unlike the Create function, these always work.
      code += "  static void start";
      code += struct_def.name;
      code += "(FlatBufferBuilder builder) { builder.";
      code += "startObject(";
      code += NumToString(struct_def.fields.vec.size()) + ") }\n";
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        auto &field = **it;
        if (field.deprecated) continue;
        if (field.key) key_field = &field;
        code += "  static void add";
        code += MakeCamel(field.name);
        code += "(FlatBufferBuilder builder, ";
        code += GenTypeBasic(DestinationType(field.value.type, false));
        auto argname = MakeCamel(field.name, false);
        if (!IsScalar(field.value.type.base_type)) argname += "Offset";
        code += " " + argname + ") { builder.add";
        code += GenMethod(field.value.type) + "(";
        code += NumToString(it - struct_def.fields.vec.begin()) + ", ";
        code += SourceCastBasic(field.value.type);
        code += argname;
        code += ", ";
        if (lang_.language == IDLOptions::kJava)
          code += SourceCastBasic(field.value.type);
        code += GenDefaultValue(field, false);
        code += ") }\n";
        if (field.value.type.base_type == BASE_TYPE_VECTOR) {
          auto vector_type = field.value.type.VectorType();
          auto alignment = InlineAlignment(vector_type);
          auto elem_size = InlineSize(vector_type);
          if (!IsStruct(vector_type)) {
            // Generate a method to create a vector from a Java array.
            code += "  static Int ";
            code += "create";
            code += MakeCamel(field.name);
            code += "Vector(FlatBufferBuilder builder, ";
            code += GenTypeBasic(vector_type) + "[] data) ";
            code += "{ builder.startVector(";
            code += NumToString(elem_size);
            code += ", data.length, ";
            code += NumToString(alignment);
            code += "); for (int i = data.";
            code += "length - 1; i >= 0; i--) builder.";
            code += "add";
            code += GenMethod(vector_type);
            code += "(";
            code += SourceCastBasic(vector_type, false);
            code += "data[i]";
            code += "); return builder.endVector() }\n";
          }
          // Generate a method to start a vector, data to be added manually
          // after.
          code += "  static void start";
          code += MakeCamel(field.name);
          code += "Vector(FlatBufferBuilder builder, int numElems) ";
          code += "{ builder.startVector(";
          code += NumToString(elem_size);
          code += ", numElems, " + NumToString(alignment);
          code += ") }\n";
        }
      }
      code += "  static Int ";
      code += "end" + struct_def.name;
      code += "(FlatBufferBuilder builder) {\n    int o = builder.";
      code += "endObject()\n";
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        auto &field = **it;
        if (!field.deprecated && field.required) {
          code += "    builder.required(o, ";
          code += NumToString(field.value.offset);
          code += ");  // " + field.name + "\n";
        }
      }
      code += "    return " + GenOffsetConstruct(struct_def, "o") + "\n  }\n";
      if (parser_.root_struct_def_ == &struct_def) {
        std::string size_prefix[] = {"", "SizePrefixed"};
        for (int i = 0; i < 2; ++i) {
          code += "  static void ";
          code += "finish" + size_prefix[i] + struct_def.name;
          code +=
              "Buffer(FlatBufferBuilder builder, Int";
          code += " offset) {";
          code += " builder.finish" + size_prefix[i] + "(offset";
          if (parser_.file_identifier_.length())
            code += ", \"" + parser_.file_identifier_ + "\"";
          code += ") }\n";
        }
      }
    }
    // Only generate key compare function for table,
    // because `key_field` is not set for struct
    if (struct_def.has_key && !struct_def.fixed) {
      code += "\n  @Override\n  protected int keysCompare(";
      code += "Integer o1, Integer o2, ByteBuffer _bb) {";
      code += GenKeyGetter(key_field);
      code += " }\n";

      code += "\n  static " + struct_def.name + lang_.optional_suffix;
      code += " __lookup_by_key(";
      code += struct_def.name + " obj, ";
      code += "int vectorLocation, ";
      code += GenTypeNameDest(key_field->value.type);
      code += " key, ByteBuffer bb) {\n";
      if (key_field->value.type.base_type == BASE_TYPE_STRING) {
        code += "    byte[] byteKey = ";
        code += "key.getBytes(Table.UTF8_CHARSET.get())\n";
      }
      code += "    int span = ";
      code += "bb.getInt(vectorLocation - 4)\n";
      code += "    int start = 0\n";
      code += "    while (span != 0) {\n";
      code += "      int middle = span / 2\n";
      code += GenLookupKeyGetter(key_field);
      code += "      if (comp > 0) {\n";
      code += "        span = middle\n";
      code += "      } else if (comp < 0) {\n";
      code += "        middle++\n";
      code += "        start += middle\n";
      code += "        span -= middle\n";
      code += "      } else {\n";
      code += "        return ";
      code += "(obj == null ? new " + struct_def.name + "() : obj)";
      code += ".__assign(tableOffset, bb)\n";
      code += "      }\n    }\n";
      code += "    return null\n";
      code += "  }\n";
    }
    
    code += GenerateCompanionObject(1, struct_def, [&](int ilevel, CodeWriter &writer){
        if (!struct_def.fixed) {
            GenerateGetRootAsAccessors(ilevel, struct_def.name, writer);
        } else {
            GenerateStaticConstructor(ilevel, struct_def, writer);
        }
    });
    // class closing
    code += "}\n";
  }
  
  std::string GenerateCompanionObject(int ilevel, StructDef &struct_def, 
                                      CodeblockFunction callback) const {
      if (struct_def.generated) return "";
      CodeWriter code;
      code += t(ilevel) + "companion object {\n";
      // 1 means identation level
      callback(ilevel + 1, code);
      code += t(ilevel) + "}";
      return code.ToString();
  }
  
  static void GenerateGetRootAsAccessors(int ilvl, std::string struct_name, 
                                         CodeWriter &writer) {
    // Generate a special accessor for the table that when used as the root
    // e.g. `fun getRootAsMonster(_bb: ByteBuffer): Monster {...}`
    writer.SetValue("gr_name", struct_name);
    writer.SetValue("gr_method", "getRootAs" + struct_name);
    writer.SetValue("gr_bb_pos", lang_.get_bb_position);
    
    // create convenience method that doesn't require an existing object
    writer += t(ilvl) + "fun {{gr_method}}(_bb: ByteBuffer): {{gr_name}} {";
    writer += t(ilvl+1) + "return {{gr_method}}(_bb, {{gr_name}}())";
    writer += t(ilvl) + "}\n";
 
    // create method that allows object reuse
    // e.g. fun Monster getRootAsMonster(_bb: ByteBuffer, obj: Monster) {...}
    writer += t(ilvl) + 
            "fun {{gr_method}}"
            "(_bb: ByteBuffer, obj: {{gr_name}}): {{gr_name}} {";
    writer += t(ilvl+1) + lang_.set_bb_byteorder;
    writer += t(ilvl+1) + "return (obj.__assign(_bb.getInt(_bb.{{gr_bb_pos}})"
                                " + _bb.{{gr_bb_pos}}, _bb))";
    writer += t(ilvl) + "}\n";
  }
  
  static void GenerateStaticConstructor(int ilvl, const StructDef &struct_def, CodeWriter &code) {
      // create a struct constructor function
      code.SetValue("gsc_name", struct_def.name);
      code += t(ilvl) + "fun create{{gsc_name}}(builder: FlatBufferBuilder\\";
      GenStructArgs(struct_def, code, "");
      code += "): Int {\n";
      GenStructBody(struct_def, code, "");
      code += "    return ";
      code += GenOffsetConstruct(
          struct_def, "builder." + std::string(lang_.get_fbb_offset));
      code += "\n  }\n";
  }
  
  static std::string GenerateGetter(int ilevel, 
                                    std::string name, 
                                    std::string type,  
                                    CodeblockFunction body) {
      // Generates Kotlin getter for properties
      // e.g.: 
      // val prop: Mytype
      //     get() = {
      //       return x
      //     }
      CodeWriter code;   
      code.SetValue("name", name);
      code.SetValue("type", type);
      code += t(ilevel) + "val {{name}} : {{type}}";
      code += t(ilevel+1) + "get() = {";
      body(ilevel+2, code);
      code += t(ilevel+1) + "}";
      return code.ToString();
  }
  
  static std::string GenerateFun(int ilevel, 
                                    std::string name,
                                    std::string params,
                                    std::string returnType,  
                                    CodeblockFunction body) {
      // Generates Kotlin function
      // e.g.: 
      // fun path(j: Int): Vec3 { 
      //     return path(Vec3(), j)
      // }
      CodeWriter code;   
      code.SetValue("name", name);
      code.SetValue("params", params);
      code.SetValue("return_type", returnType
                    );
      code += t(ilevel) + "fun {{name}}({{params}}) : {{return_type}} {";
      body(ilevel+1, code);
      code += t(ilevel) + "}";
      return code.ToString();
  }
  
  // Ident code by adding spaces according to an ident level
  static std::string t(int level) {
      // Four spaces ident aligns well in kotlin
      std::string out;
      while(level--){
          out += "    ";
      }
      return out;
  }
  
  // This tracks the current namespace used to determine if a type need to be
  // prefixed by its namespace
  const Namespace *cur_name_space_;
};
}  // namespace kotlin

bool GenerateKotlin(const Parser &parser, const std::string &path,
                    const std::string &file_name) {
  kotlin::KotlinGenerator generator(parser, path, file_name);
  return generator.generate();
}
}  // namespace flatbuffers
