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

using CodeblockFunction = std::function<void(CodeWriter &)>;

static TypedFloatConstantGenerator KotlinFloatGen("Double.", "Float.", "NaN",
                                                "POSITIVE_INFINITY",
                                                "NEGATIVE_INFINITY");

static const CommentConfig comment_config= { "/**", " *", " */" };

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
        
        std::string code = "// " + std::string(FlatBuffersGeneratedWarning()) + "\n\n";
        
        std::string namespace_name = FullNamespace(".", ns);
        if (!namespace_name.empty()) {
            code += "package " + namespace_name;
            code += "\n\n";
        }
        if (needs_includes) {
            code += "import java.nio.*\n";
            code += "import com.google.flatbuffers.*\n\n";
        }
        code += classcode;
        auto filename = NamespaceDir(ns) + defname + ".kt";
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
        static const char * const kotlin_typename[] = {
    #define FLATBUFFERS_TD(ENUM, IDLTYPE, \
        CTYPE, JTYPE, GTYPE, NTYPE, PTYPE, RTYPE, KTYPE) \
    #KTYPE,
        FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
    #undef FLATBUFFERS_TD
        };
        return kotlin_typename[type.base_type];
        
    }
    
    std::string GenTypePointer(const Type &type) const {
        switch (type.base_type) {
        case BASE_TYPE_STRING:
            return "String";
        case BASE_TYPE_VECTOR:
            return GenTypeGet(type.VectorType());
        case BASE_TYPE_STRUCT:
            return WrapInNameSpace(*type.struct_def);
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
    
    // Generate destination type name
    std::string GenTypeNameDest(const Type &type) const {
        return GenTypeGet(DestinationType(type, true));
    }
    
    // Mask to turn serialized value into destination type value.
    std::string DestinationMask(const Type &type, bool vectorelem) const {
        switch (type.base_type) {
        case BASE_TYPE_UCHAR:
            return ".and(0xFF)";
        case BASE_TYPE_USHORT:
            return ".and(0xFFFF)";
        case BASE_TYPE_UINT:
            return ".and(0xFFFFFFFFL)";
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
            return ".toLong()";
        }
        return "";
    }
    
    // Cast statements for mutator method parameters.
    // In Java, parameters representing unsigned numbers need to be cast down to
    // their respective type. For example, a long holding an unsigned int value
    // would be cast down to int before being put onto the buffer.
    static std::string SourceCast(const Type &type, bool castFromDest) {
        if (type.base_type == BASE_TYPE_VECTOR) {
            return SourceCast(type.VectorType(), castFromDest);
        } else {
            if (castFromDest) {
                if (type.base_type == BASE_TYPE_UINT)
                    return ".toInt()";
                else if (type.base_type == BASE_TYPE_USHORT)
                    return ".toShort()";
                else if (type.base_type == BASE_TYPE_UCHAR)
                    return ".toByte()";
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
        
        auto longSuffix = "L";
        switch (value.type.base_type) {
        case BASE_TYPE_BOOL:
            return value.constant == "0" ? "false" : "true";
        case BASE_TYPE_ULONG: {
            // Converts the ulong into its bits signed equivalent
            uint64_t defaultValue = StringToUInt(value.constant.c_str());
            return NumToString(static_cast<int64_t>(defaultValue)) + longSuffix;
        }
        case BASE_TYPE_UINT:
        case BASE_TYPE_LONG:
            return value.constant + longSuffix;
        default:
            if (IsFloat(value.type.base_type)) {
                auto val = KotlinFloatGen.GenFloatConstant(field);
                if (value.type.base_type == BASE_TYPE_DOUBLE &&
                        val.back() == 'f') {
                    val.pop_back();
                }
                return val;
            } else {
                return value.constant;
            }
        }
    }
    
    std::string GenDefaultValue(const FieldDef &field) const {
        return GenDefaultValue(field, true);
    }
    
    std::string GenDefaultValueBasic(const FieldDef &field,
                                     bool enableLangOverrides) const {
        auto &value = field.value;
        if (!IsScalar(value.type.base_type)) {
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
        GenComment(enum_def.doc_comment, code_ptr, &comment_config);
        CodeWriter writer;
        writer += "@Suppress(\"unused\")";
        writer += "class " + enum_def.name + " private constructor() {";
        writer.IncrementIdentLevel();
        
        GenerateCompanionObject(writer, [&](CodeWriter &code){
            // Write all properties
            auto vals = enum_def.Vals();
            for (auto it = vals.begin(); it != vals.end(); ++it) {
                auto &ev = **it;
                code.SetValue("name", ev.name);
                code.SetValue("type", GenTypeBasic(enum_def.underlying_type));
                code.SetValue("val", enum_def.ToString(ev));
                GenComment(ev.doc_comment, code_ptr, &comment_config, "  ");
                code += "const val {{name}}: {{type}} = {{val}}";
            }
            
            // Generate a generate string table for enum values.
            // Problem is, if values are very sparse that could generate really big
            // tables. Ideally in that case we generate a map lookup instead, but for
            // the moment we simply don't output a table at all.
            auto range = enum_def.Distance();
            // Average distance between values above which we consider a table
            // "too sparse". Change at will.
            static const uint64_t kMaxSparseness = 5;
            if (range / static_cast<uint64_t>(enum_def.size()) < kMaxSparseness) {
                GeneratePropertyOneLine(code, "names", "Array<String>", 
                               [&](CodeWriter &code){
                    code += "arrayOf(\\";
                    auto val = enum_def.Vals().front();
                    auto vals = enum_def.Vals();
                    for (auto it = vals.begin(); it != vals.end(); ++it) {
                        auto ev = *it;
                        for (auto k = enum_def.Distance(val, ev); k > 1; --k)
                            code += "\"\", \\";
                        val = ev;
                        code += "\"" + (*it)->name + "\"\\";
                        if (it+1 != vals.end()) {
                            code += ", \\";
                        }
                    }
                    code += ")";
                });
                GenerateFunOneLine(code, "name", "e: Int", "String", 
                                      [&](CodeWriter &code){
                    code += "names[e\\";
                    if (enum_def.MinValue()->IsNonZero())
                        code += " - " + enum_def.MinValue()->name + "\\";
                    code += "]";
                });
            }
        });
        writer.DecrementIdentLevel();
        writer += "}";
        code += writer.ToString();
    }
    
    // Returns the function name that is able to read a value of the given type.
    std::string GenGetter(const Type &type) const {
        switch (type.base_type) {
        case BASE_TYPE_STRING:
            return "__string";
        case BASE_TYPE_STRUCT:
            return "__struct";
        case BASE_TYPE_UNION:
            return "__union";
        case BASE_TYPE_VECTOR:
            return GenGetter(type.VectorType());
        default: {
            std::string getter = "bb.get";
            if (GenTypeBasic(type) != "Byte" && 
                    GenTypeBasic(type) != "Boolean") {
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
        if (GenTypeBasic(type) != "Byte") {
            getter += MakeCamel(GenTypeBasic(type));
        }
        getter = getter + "(" + GenOffsetGetter(key_field, num) + ")" + dest_cast + 
                dest_mask;
        return getter;
    }
    
    // Direct mutation is only allowed for scalar fields.
    // Hence a setter method will only be generated for such fields.
    std::string GenSetter(const Type &type) const {
        if (IsScalar(type.base_type)) {
            std::string setter = "bb.put";
            if (GenTypeBasic(type) != "Byte" &&
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
                code += MakeCamel(field.name) + ": \\";
                code += GenTypeBasic(DestinationType(field.value.type, false)) + "\\";
            }
        }
    }
    
    // Recusively generate struct construction statements of the form:
    // builder.putType(name);
    // and insert manual padding.
    static void GenStructBody(const StructDef &struct_def, CodeWriter &code,
                              const char *nameprefix) {
        code.SetValue("align", NumToString(struct_def.minalign));
        code.SetValue("size", NumToString(struct_def.bytesize));
        code += "builder.prep({{align}}, {{size}})";
        auto fields_vec = struct_def.fields.vec;
        for (auto it = fields_vec.rbegin(); it != fields_vec.rend(); ++it) {
            auto &field = **it;
            
            if (field.padding) {
                code.SetValue("pad", NumToString(field.padding));
                code += "builder.pad({{pad}})";
            }
            if (IsStruct(field.value.type)) {
                GenStructBody(*field.value.type.struct_def, code,
                              (nameprefix + (field.name + "_")).c_str());
            } else {
                code.SetValue("type", GenMethod(field.value.type));
                code.SetValue("cast", SourceCast(field.value.type));
                code.SetValue("argname", nameprefix + 
                              MakeCamel(field.name, false));
                code += "builder.put{{type}}({{argname}}{{cast}})";
            }
        }
    }
    
    std::string GenByteBufferLength(const char *bb_name) const {
        std::string bb_len = bb_name;
        bb_len += ".capacity()";
        return bb_len;
    }
    
    std::string GenOffsetGetter(flatbuffers::FieldDef *key_field,
                                const char *num = nullptr) const {
        std::string key_offset = "";
        key_offset += std::string("") + "__offset(" +
                NumToString(key_field->value.offset) + ", ";
        if (num) {
            key_offset += num;
            key_offset += ", _bb)";
        } else {
            key_offset += GenByteBufferLength("bb");
            key_offset += " - tableOffset, bb)";
        }
        return key_offset;
    }
    
    std::string GenKeyGetter(flatbuffers::FieldDef *key_field) const {
        std::string key_getter = "";
        auto data_buffer = "_bb";
        if (key_field->value.type.base_type == BASE_TYPE_STRING) {
            key_getter += " return ";
            key_getter += std::string("");
            key_getter += "compareStrings(";
            key_getter += GenOffsetGetter(key_field, "o1") + ", ";
            key_getter += GenOffsetGetter(key_field, "o2") + ", " + data_buffer + ")";
        } else {
            auto field_getter = GenGetterForLookupByKey(key_field, data_buffer, "o1");
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
        GenComment(struct_def.doc_comment, code_ptr, &comment_config);
        auto fixed = struct_def.fixed;
        
        CodeWriter writer;
        writer.SetValue("struct_name", struct_def.name);
        writer.SetValue("superclass", fixed ? "Struct" : "Table");
        writer += "@Suppress(\"unused\")";
        writer += "class {{struct_name}} : {{superclass}}() {\n";
        
        writer.IncrementIdentLevel();
        
        {
            // Generate the init() method that sets the field in a pre-existing
            // accessor object. This is to allow object reuse.
            GenerateFun(writer, "init", "_i: Int, _bb: ByteBuffer", "",
                        [&](CodeWriter &writer) {
                writer += "bb_pos = _i";
                writer += "bb = _bb";
                if (!struct_def.fixed) {
                    writer += "vtable_start = bb_pos - bb.getInt(bb_pos)";
                    writer += "vtable_size = bb.getShort(vtable_start).toInt()";
                }       
            });
            
            // Generate assign method
            GenerateFun(writer, "assign", "_i: Int, _bb: ByteBuffer",  
                        struct_def.name,
                        [&](CodeWriter &code) {
                code += "init(_i, _bb)";
                code += "return this";
            });
            
            // Generate all getters
            GenerateStructGetters(struct_def, writer);
            
            // Generate Static Fields
            GenerateCompanionObject(writer, [&](CodeWriter &writer){
                
                if (!struct_def.fixed) {
                    FieldDef *key_field = nullptr;
                    
                    GenerateGetRootAsAccessors(struct_def.name, writer);
                    GenerateBufferHasIdentifier(struct_def, writer);
                    GenerateTableCreator(struct_def, writer);
                    
                    GenerateStartStructMethod(struct_def, writer);
                    
                    // Static Add for fields
                    auto fields = struct_def.fields.vec;
                    int field_pos = -1;
                    for (auto it = fields.begin(); it != fields.end(); ++it) {
                        auto &field = **it;
                        field_pos++;
                        if (field.deprecated) continue;
                        if (field.key) key_field = &field;
                        GenerateAddField(NumToString(field_pos), field, writer);
                        
                        if (field.value.type.base_type == BASE_TYPE_VECTOR) {
                            auto vector_type = field.value.type.VectorType();
                            if (!IsStruct(vector_type)) {
                                GenerateCreateVectorField(field, writer);
                            }
                            GenerateStartVectorField(field, writer);
                        }
                    }
                    
                    GenerateEndStructMethod(struct_def, writer);
                    
                    if (parser_.root_struct_def_ == &struct_def) {
                        GenerateFinishStructBuffer(struct_def, 
                                                   parser_.file_identifier_, 
                                                   writer);
                        GenerateFinishSizePrefixedStructBuffer(struct_def, 
                                                               parser_.file_identifier_, 
                                                               writer);
                    }
                    
                    if (struct_def.has_key) {
                        GenerateLookupByKey(key_field, struct_def, writer);   
                    }
                } else {
                    GenerateStaticConstructor(struct_def, writer);
                }
            });
            
            code += writer.ToString();
        }
    
        // class closing
        writer.DecrementIdentLevel();
        code += "}\n";
    }
    
    // TODO: move key_field to reference instead of pointer
    void GenerateLookupByKey(FieldDef *key_field, StructDef &struct_def, 
                             CodeWriter &code) const {
        std::stringstream params;
        params << "obj: " << struct_def.name << "?" << ", ";
        params << "vectorLocation: Int, ";
        params << "key: " 
               <<  GenTypeNameDest(key_field->value.type)
                << ", ";
        params << "bb: ByteBuffer";
        
        auto statements = [&](CodeWriter &code) {
            auto base_type = key_field->value.type.base_type;
            code.SetValue("struct_name", struct_def.name);
            if (base_type == BASE_TYPE_STRING) {
                code += "val byteKey = key."
                        "toByteArray(Table.UTF8_CHARSET.get()!!)";
            }
            code += "var span = bb.getInt(vectorLocation - 4)";
            code += "var start = 0";
            code += "while (span != 0) {";
            code.IncrementIdentLevel();
            code += "var middle = span / 2";
            code += "val tableOffset = __indirect(vector"
                    "Location + 4 * (start + middle), bb)";
            if (key_field->value.type.base_type == BASE_TYPE_STRING) {
                code += "val comp = compareStrings(\\";
                code += GenOffsetGetter(key_field) + "\\";
                code += ", byteKey, bb)";
            } else {
                auto get_val = GenGetterForLookupByKey(key_field, "bb");
                code += "val value = " + get_val;
                code += "val comp = (value - key).sign";
            }
            code += "when {";
            code.IncrementIdentLevel();
            code += "comp > 0 -> span = middle";
            code += "comp < 0 -> {";
            code.IncrementIdentLevel();
            code += "middle++";
            code += "start += middle";
            code += "span -= middle";
            code.DecrementIdentLevel();
            code += "}"; // end comp < 0
            code += "else -> {";
            code.IncrementIdentLevel();
            code += "return (obj ?: {{struct_name}}()).assign(tableOffset, bb)";
            code.DecrementIdentLevel();
            code += "}"; // end else
            code.DecrementIdentLevel();
            code += "}"; // end when
            code.DecrementIdentLevel();
            code += "}"; // end while
            code += "return null";
        };
        GenerateFun(code, "__lookup_by_key", 
                    params.str(), 
                    struct_def.name + "?", 
                    statements);
    }
    
    void GenerateFinishSizePrefixedStructBuffer(StructDef &struct_def, 
                                                std::string identifier, 
                                                CodeWriter &code) const {
        auto id = identifier.length() > 0 ? ", " + identifier : "";
        auto params = "builder: FlatBufferBuilder, offset: Int";
        auto method_name = "finishSizePrefixed" + struct_def.name + "Buffer"; 
        auto one_line = "builder.finishSizePrefixed(offset" + id  + ")";
        GenerateFunOneLine(code, method_name, params, "",one_line);
    }
    void GenerateFinishStructBuffer(StructDef &struct_def, 
                                    std::string identifier, 
                                    CodeWriter &code) const {
        auto id = identifier.length() > 0  ? ", " + identifier : "";
        auto params = "builder: FlatBufferBuilder, offset: Int";
        auto method_name = "finish" + struct_def.name + "Buffer"; 
        auto one_line = "builder.finish(offset" + id + ")";
        GenerateFunOneLine(code, method_name, params, "", one_line);   
    }
    
    void GenerateEndStructMethod(StructDef &struct_def, CodeWriter &code) const {
        // Generate end{{TableName}}(builder: FlatBufferBuilder) method
        auto name = "end" + struct_def.name;
        auto params = "builder: FlatBufferBuilder";
        auto returns = "Int";
        auto field_vec = struct_def.fields.vec;
        
        GenerateFun(code, name, params, returns, [&](CodeWriter &code){
            code += "val o = builder.endObject()";
            code.IncrementIdentLevel();
            for (auto it = field_vec.begin(); it != field_vec.end(); ++it) {
                auto &field = **it;
                if (field.deprecated || !field.required) {
                    continue;
                }
                code.SetValue("offset", NumToString(field.value.offset));
                code += "builder.required(o, {{offset}})";
            }
            code.DecrementIdentLevel();
            code += "return o";
        });
    }
    
    // Generate a method to create a vector from a Kotlin array.    
    void GenerateCreateVectorField(FieldDef &field, CodeWriter &code) const {
        auto vector_type = field.value.type.VectorType();
        auto method_name = "create" + MakeCamel(field.name) + "Vector";
        auto params = "builder: FlatBufferBuilder, data: " + 
                GenTypeBasic(vector_type) + "Array";
        code.SetValue("size", NumToString(InlineSize(vector_type)));
        code.SetValue("align", NumToString(InlineAlignment(vector_type)));
        code.SetValue("root", GenMethod(vector_type));
        code.SetValue("cast", SourceCastBasic(vector_type, false));
        
        GenerateFun(code, method_name, params, "Int", [&](CodeWriter &code){
            code += "builder.startVector({{size}}, data.size, {{align}})";
            code += "for (i in data.size - 1 downTo 0) {";
            code.IncrementIdentLevel();
            code += "builder.add{{root}}(data[i]{{cast}})";
            code.DecrementIdentLevel();
            code += "}";
            code += "return builder.endVector()";
        });
    }
    
    void GenerateStartVectorField(FieldDef &field, CodeWriter &code) const {
        // Generate a method to start a vector, data to be added manually
        // after.
        auto vector_type = field.value.type.VectorType();
        auto params = "builder: FlatBufferBuilder, numElems: Int";
        auto statement = "builder.startVector({{size}}, numElems, {{align}})";
        
        code.SetValue("size", NumToString(InlineSize(vector_type)));
        code.SetValue("align", NumToString(InlineAlignment(vector_type)));
        
        GenerateFunOneLine(code, 
                           "start" + MakeCamel(field.name, true), params, "",
                           statement);
    }
    
    void GenerateAddField(std::string field_pos, FieldDef &field, 
                          CodeWriter &writer) const {
        auto field_type = GenTypeBasic(DestinationType(field.value.type, false));
        auto secondArg = MakeCamel(field.name, false) + ": " + field_type;
        GenerateFunOneLine(writer, "add" + MakeCamel(field.name, true), 
                           "builder: FlatBufferBuilder, " + secondArg, 
                           "", [&](CodeWriter &code){
            auto method = GenMethod(field.value.type);
            code.SetValue("field_name", MakeCamel(field.name, false));
            code.SetValue("method_name", method);
            code.SetValue("pos", field_pos);
            code.SetValue("default", GenDefaultValue(field, false));
            
            
            //code.SetValue("special_cast", DefaultValueByteBuffer(method, field));
            
            
            code += "builder.add{{method_name}}({{pos}}, \\";
            code += "{{field_name}}{{cast}}, {{default}}{{cast}})";
        });
    }
    
    std::string DefaultValueByteBuffer(std::string method, 
                                       const FieldDef &field) {
        if (method == "Int") {
            
        }
        return "";
    }
    
    // fun startMonster(builder: FlatBufferBuilder) = builder.startObject(11)
    void GenerateStartStructMethod(StructDef &struct_def, CodeWriter &code) const {
        GenerateFunOneLine(code, "start" + struct_def.name, 
                           "builder: FlatBufferBuilder", "", 
                           "builder.startObject("+ 
                           NumToString(struct_def.fields.vec.size()) + ")");
    }
    
    void GenerateTableCreator(StructDef &struct_def, CodeWriter &code) const {
        // Generate a method that creates a table in one go. This is only possible
        // when the table has no struct fields, since those have to be created
        // inline, and there's no way to do so in Java.
        bool has_no_struct_fields = true;
        int num_fields = 0;
        auto fields_vec = struct_def.fields.vec;
        
        for (auto it = fields_vec.begin(); it != fields_vec.end(); ++it) {
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
            
            auto name = "create" + struct_def.name;
            std::stringstream params;
            params << "builder: FlatBufferBuilder";
            for (auto it = fields_vec.begin(); it != fields_vec.end(); ++it) {
                auto &field = **it;
                if (field.deprecated) continue;
                params << ", " << field.name;
                if (!IsScalar(field.value.type.base_type)){ 
                    params << "Offset: ";
                } else {
                    params << ": ";
                }
                params << GenTypeBasic(DestinationType(field.value.type, false));
            }
            
            GenerateFun(code, name, params.str(), "Int", [&](CodeWriter & code) {
                auto fields_vec = struct_def.fields.vec;
                code.SetValue("vec_size", NumToString(fields_vec.size()));
                
                code += "builder.startObject({{vec_size}})";
                
                auto sortbysize = struct_def.sortbysize;
                auto largest = sortbysize ? sizeof(largest_scalar_t) : 1;
                for (size_t size = largest; size; size /= 2) {
                    for (auto it = fields_vec.rbegin(); it != fields_vec.rend();
                         ++it) {
                        auto &field = **it;
                        auto base_type_size = SizeOf(field.value.type.base_type);
                        if (!field.deprecated && 
                                (!sortbysize || size == base_type_size)) {
                            code.SetValue("camel_field_name", MakeCamel(field.name, true));
                            code.SetValue("field_name", MakeCamel(field.name, false));
                            
                            code += "add{{camel_field_name}}(builder, {{field_name}}\\";
                            if (!IsScalar(field.value.type.base_type)){
                                code += "Offset\\";
                            }
                            code += ")";
                        }
                    }
                }
              code += "return end{{struct_name}}(builder)";
            });
        }
        
    }
    void GenerateBufferHasIdentifier(StructDef &struct_def, 
                                     CodeWriter &writer) const {
        // Check if a buffer has the identifier.
        if (parser_.root_struct_def_ != &struct_def || !parser_.file_identifier_.length())
            return;
        auto name = struct_def.name;
        GenerateFunOneLine(writer, name + "BufferHasIdentifier",
                           "_bb: ByteBuffer",
                           "Boolean",
                           "__has_identifier(_bb, \"" + parser_.file_identifier_ + "\")"
                           );
    }
    
    void GenerateStructGetters(StructDef &struct_def, CodeWriter &writer) const {
        auto fields_vec = struct_def.fields.vec;
        FieldDef *key_field = nullptr;
        for (auto it = fields_vec.begin(); it != fields_vec.end(); ++it) {
            auto &field = **it;
            if (field.deprecated) continue;
            if (field.key) key_field = &field;
            
            std::string comment;
            GenComment(field.doc_comment, &comment, &comment_config, "  ");
            writer += comment;
            
            auto field_name = MakeCamel(field.name, false);
            auto field_type = GenTypeGet(field.value.type);
            auto field_default = GenDefaultValue(field);
            auto return_type = GenTypeNameDest(field.value.type);
            auto field_mask = DestinationMask(field.value.type, true);
            auto src_cast = SourceCast(field.value.type);
            auto dst_cast = DestinationCast(field.value.type);
            auto getter = GenGetter(field.value.type);
            auto getter_cast = TypeConversor(field_type, return_type);
            auto offset_val = NumToString(field.value.offset);
            auto offset_prefix = "val o = __offset(" + offset_val + "); return o != 0 ? ";
            
            // Most field accessors need to retrieve and test the field offset first,
            // this is the prefix code for that:
            writer.SetValue("offset", NumToString(field.value.offset));
            writer.SetValue("return_type", return_type);
            writer.SetValue("field_type", field_type);
            writer.SetValue("field_name", field_name);
            writer.SetValue("field_default", field_default);
            writer.SetValue("field_mask", field_mask);
            writer.SetValue("field_read_func", getter);
            writer.SetValue("cast", getter_cast);
            
            // Generate the accessors that don't do object reuse.
            if (field.value.type.base_type == BASE_TYPE_STRUCT) {
                // Calls the accessor that takes an accessor object with a new object.
                // val pos
                //     get() = pos(Vec3())
                GenerateGetterOneLine(writer, 
                                      field_name, 
                                      return_type + "?", [&](CodeWriter &writer){
                    writer += "{{field_name}}({{field_type}}())";
                });
            } else if (field.value.type.base_type == BASE_TYPE_VECTOR &&
                       field.value.type.element == BASE_TYPE_STRUCT) {
                // Accessors for vectors of structs also take accessor objects, this
                // generates a variant without that argument.
                // e.g. fun weapons(j: Int) = weapons(Weapon(), j)
                GenerateFunOneLine(writer, field_name, "j: Int", return_type + "?", 
                            [&](CodeWriter &writer){
                    writer += "{{field_name}}({{return_type}}(), j)";
                });
            }
            
            if (IsScalar(field.value.type.base_type)) {
                if (struct_def.fixed) {
                    GenerateGetterOneLine(writer, field_name, return_type, 
                                   [&](CodeWriter &writer){
                        writer += "{{field_read_func}}(bb_pos + {{offset}}){{cast}}{{field_mask}}";
                    }); 
                } else {
                    GenerateGetter(writer, field_name, return_type, 
                                   [&](CodeWriter &writer){
                        writer += "val o = __offset({{offset}})";
                        writer += "return if(o != 0) {{field_read_func}}(o + bb_pos){{cast}}{{field_mask}} else {{field_default}}";
                    });
                }
            } else {
                switch (field.value.type.base_type) {
                case BASE_TYPE_STRUCT:
                    
                    if (struct_def.fixed) {
                        // create getter with object reuse
                        // e.g.
                        //  fun pos(obj: Vec3) : Vec3? = obj.assign(bb_pos + 4, bb)
                        // ? adds nullability annotation
                        GenerateFunOneLine(writer, field_name, "obj: " + field_type , 
                                    return_type + "?", 
                                    [&](CodeWriter &writer){
                            writer += "obj.assign(bb_pos + {{offset}}, bb)";
                        });
                    } else {
                        // create getter with object reuse
                        // e.g.
                        //  fun pos(obj: Vec3) : Vec3? {
                        //      val o = __offset(4)
                        //      return if(o != 0) {
                        //          obj.assign(o + bb_pos, bb)
                        //      else {
                        //          null
                        //      }
                        //  }
                        // ? adds nullability annotation
                        GenerateFun(writer, field_name, "obj: " + field_type, 
                                    return_type + "?", [&](CodeWriter &writer){
                            auto fixed = field.value.type.struct_def->fixed;
                            
                            writer.SetValue("seek", Indirect("o + bb_pos", fixed));
                            OffsetWrapper(writer, 
                                          offset_val, 
                                          "obj.assign({{seek}}, bb)", 
                                          "null");
                        });
                    }
                    break;
                case BASE_TYPE_STRING:
                    // create string getter
                    // e.g.
                    //      val Name : String?
                    //          get() = {
                    //              val o = __offset(10)
                    //              return if (o != 0) __string(o + bb_pos) else null
                    //          }
                    // ? adds nullability annotation
                    GenerateGetter(writer, field_name, return_type + "?", 
                                   [&](CodeWriter &writer){
                        
                        writer += "val o = __offset({{offset}})";
                        writer += "return if (o != 0) __string(o + bb_pos) else null";
                    });
                    break;
                case BASE_TYPE_VECTOR: {
                    // e.g.
                    // fun inventory(int j) : Int { val o = __offset(14); return o != 0 ? bb.get(__vector(o) + j * 1).toInt().and(0xFF) : 0; }
                    
                    auto vectortype = field.value.type.VectorType();
                    std::string params = "j: Int";
                    std::string nullable = IsScalar(vectortype.base_type) ? "" 
                                                                          : "?";
                    
                    if (vectortype.base_type == BASE_TYPE_STRUCT || 
                            vectortype.base_type == BASE_TYPE_UNION) {
                        params = "obj: " + field_type + ", j: Int";
                    }
                    
                    
                    writer.SetValue("toType", TypeConversor(field_type, return_type));
                    
                    GenerateFun(writer, field_name, params, 
                                return_type + nullable, 
                                [&](CodeWriter &writer){
                        auto inline_size = NumToString(InlineSize(vectortype));
                        auto index = "__vector(o) + j * " + inline_size;
                        bool fixed = struct_def.fixed;
                        
                        writer.SetValue("index", Indirect(index, fixed));
                        
                        auto not_found = NotFoundReturn(field.value.type.element);
                        auto found = "";
                        switch(vectortype.base_type) {
                        case BASE_TYPE_STRUCT:
                            found = "obj.assign({{index}}, bb){{cast}}{{field_mask}}";
                            break;
                        case BASE_TYPE_UNION:
                            found = "{{field_read_func}}({{index}} - bb_pos){{cast}}{{field_mask}}";
                            break;
                        default:
                            found = "{{field_read_func}}({{index}}){{cast}}{{field_mask}}";
                        }
                        OffsetWrapper(writer, offset_val, found, not_found);
                    });
                    break;
                }
                case BASE_TYPE_UNION:
                    GenerateFun(writer, field_name, "obj: " + field_type, 
                                return_type + "?", 
                                [&](CodeWriter &writer){
                        writer += OffsetWrapperOneLine(offset_val, getter + "(obj, o)", 
                                                       "null");
                    });
                    break;
                default:
                    FLATBUFFERS_ASSERT(0);
                }
            }
            
            if (field.value.type.base_type == BASE_TYPE_VECTOR) {
                // Generate Lenght functions for vectors
                GenerateGetter(writer, field_name + "Length", "Int", 
                               [&](CodeWriter &writer){
                    writer += OffsetWrapperOneLine(offset_val, 
                                                   "__vector_len(o)", "0");
                });
                
                // See if we should generate a by-key accessor.
                if (field.value.type.element == BASE_TYPE_STRUCT &&
                        !field.value.type.struct_def->fixed) {
                    auto &sd = *field.value.type.struct_def;
                    auto &fields = sd.fields.vec;
                    for (auto kit = fields.begin(); kit != fields.end(); ++kit) {
                        auto &key_field = **kit;
                        if (key_field.key) {
                            auto qualified_name = WrapInNameSpace(sd);
                            auto name = MakeCamel(field.name, false) + "ByKey";
                            auto params = "key: " + GenTypeNameDest(key_field.value.type);
                            auto return_type = qualified_name + "?";
                            GenerateFun(writer,
                                        name, 
                                        params, 
                                        return_type, 
                                        [&] (CodeWriter &code) {
                                OffsetWrapper(code, 
                                              offset_val,
                                              qualified_name +
                                              ".__lookup_by_key(null, __vector(o), key, bb)", 
                                              "null");
                            });
                            
                            auto param2 = "obj: " + qualified_name + ", key: " + GenTypeNameDest(key_field.value.type);
                            GenerateFun(writer,
                                        name,
                                        param2,
                                        return_type,
                                        [&](CodeWriter &code){
                                OffsetWrapper(code, 
                                              offset_val, 
                                              qualified_name +
                                              ".__lookup_by_key(obj, __vector(o), key, bb)", 
                                              "null");
                            });

                            break;
                        }
                    }
                }
            }
            
            if ((field.value.type.base_type == BASE_TYPE_VECTOR &&
                 IsScalar(field.value.type.VectorType().base_type)) ||
                    field.value.type.base_type == BASE_TYPE_STRING) {
                
                auto end_idx = NumToString(field.value.type.base_type == BASE_TYPE_STRING
                                           ? 1
                                           : InlineSize(field.value.type.VectorType()));
                // Generate a ByteBuffer accessor for strings & vectors of scalars.
                // e.g.
                // val inventoryByteBuffer: ByteBuffer
                //     get =  __vector_as_bytebuffer(14, 1)
                
                GenerateGetterOneLine(writer, field_name + "AsByteBuffer", 
                                      "ByteBuffer", 
                               [&](CodeWriter &code){
                    code.SetValue("end", end_idx);
                    code += "__vector_as_bytebuffer({{offset}}, {{end}})";
                });
                
                // Generate a ByteBuffer accessor for strings & vectors of scalars.
                // e.g.
                // fun inventoryInByteBuffer(_bb: Bytebuffer): ByteBuffer = __vector_as_bytebuffer(_bb, 14, 1)
                GenerateFunOneLine(writer, field_name + "InByteBuffer", 
                                   "_bb: ByteBuffer", "ByteBuffer", 
                            [&](CodeWriter &writer){
                    writer.SetValue("end", end_idx);
                    writer += "__vector_in_bytebuffer(_bb, {{offset}}, {{end}})";
                });
            }
            
            // generate object accessors if is nested_flatbuffer
            //fun testnestedflatbufferAsMonster() : Monster? { return testnestedflatbufferAsMonster(new Monster()); }
            
            if (field.nested_flatbuffer) {
                auto nested_type_name = WrapInNameSpace(*field.nested_flatbuffer);
                auto nested_method_name =
                        field_name + "As" +
                        nested_type_name;
                
                GenerateGetterOneLine(writer, 
                                      nested_method_name, 
                                      nested_type_name + "?",
                                      [&](CodeWriter &code){
                    code += nested_method_name + "(" + nested_type_name + "())";
                });
                
                GenerateFun(writer, 
                            nested_method_name, 
                            "obj: " + nested_type_name,
                            nested_type_name + "?",
                            [&](CodeWriter &code){
                    OffsetWrapper(code, offset_val, "obj.assign(__indirect(__vector(o)), bb)", "null");
                });
            }
            
            // Generate mutators for scalar fields or vectors of scalars.
            if (parser_.opts.mutable_buffer) {
                auto value_type = field.value.type;
                auto value_base_type = value_type.base_type;
                auto underlying_type = value_base_type == BASE_TYPE_VECTOR
                        ? value_type.VectorType()
                        : value_type;
                auto name = "mutate" + MakeCamel(field.name, true);
                auto size = NumToString(InlineSize(underlying_type));
                auto params = field.name + ": " + GenTypeNameDest(underlying_type);
                // A vector mutator also needs the index of the vector element it should
                // mutate.
                if (value_base_type == BASE_TYPE_VECTOR)
                    params.insert(0, "j: Int, ");
                
                // Boolean parameters have to be explicitly converted to byte
                // representation.
                auto setter_parameter = underlying_type.base_type == BASE_TYPE_BOOL
                        ? "(if(" + field.name + ") 1 else 0).toByte()"
                        : field.name;
                
                auto setter_index = value_base_type == BASE_TYPE_VECTOR
                        ? "__vector(o) + j * " + size
                        : (struct_def.fixed
                           ? "bb_pos + " + offset_val
                           : "o + bb_pos");
                if (IsScalar(value_base_type) || (value_base_type == BASE_TYPE_VECTOR &&
                         IsScalar(value_type.VectorType().base_type))) {
                    
                    auto statements = [&] (CodeWriter & code) {
                        code.SetValue("setter", GenSetter(underlying_type));
                        code.SetValue("index", setter_index);
                        code.SetValue("params", setter_parameter);
                        code.SetValue("cast", src_cast);
                        if (struct_def.fixed) {
                            code += "{{setter}}({{index}}, {{params}}{{cast}})";
                        } else {
                            OffsetWrapper(code, offset_val, 
                                          "{{setter}}({{index}}, {{params}}{{cast}})\ntrue",
                                          "false");
                        }
                    };
                    
                    if (struct_def.fixed) {
                        GenerateFunOneLine(writer, name, params, "ByteBuffer", 
                                    statements);
                    } else {
                        GenerateFun(writer, name, params, "Boolean", 
                                    statements);                        
                    }
                }
            }
        }
        if (struct_def.has_key && !struct_def.fixed) {
            // Key Comparison method
            GenerateOverrideFun(
                        writer,
                        "keysCompare",
                        "o1: Int, o2: Int, _bb: ByteBuffer",
                        "Int", [&](CodeWriter &code){
                auto data_buffer = "_bb";
                if (key_field->value.type.base_type == BASE_TYPE_STRING) {
                    code.SetValue("offset", NumToString(key_field->value.offset));
                    code += " return compareStrings(__offset({{offset}}, o1, "
                            "_bb), __offset({{offset}}, o2, _bb), _bb)";
                    
                } else {
                    auto getter1 = GenGetterForLookupByKey(key_field, data_buffer, "o1");
                    auto getter2 = GenGetterForLookupByKey(key_field, data_buffer, "o2");
                    code += "val val_1 = " + getter1;
                    code += "val val_2 = " + getter2;
                    code += "return (val_1 - val_2).sign";
                }
            });
        }
    }
    
    static std::string TypeConversor(std::string from_type, 
                                     std::string to_type) {
        if (to_type == "Boolean") 
            return ".toInt() != 0";

        if (from_type == to_type)
            return "";
        
        if (to_type == "Int") {
            return ".toInt()";
        } else if (to_type == "Short") {
            return ".toShort()";
        } else if (to_type == "Long") {
            return ".toLong()";
        } else if (to_type == "Byte") {
           return ".toByte()";
        } else if (to_type == "Boolean") {
            return ".toInt() != 0";
        }
        return "";
    }
    
    void GenerateCompanionObject(CodeWriter &code, 
                                 CodeblockFunction callback) const {
        code += "companion object {";
        code.IncrementIdentLevel();
        callback(code);
        code.DecrementIdentLevel();
        code += "}";
    }
    
    static void GenerateGetRootAsAccessors(std::string struct_name, 
                                           CodeWriter &code) {
        // Generate a special accessor for the table that when used as the root
        // e.g. `fun getRootAsMonster(_bb: ByteBuffer): Monster {...}`
        code.SetValue("gr_name", struct_name);
        code.SetValue("gr_method", "getRootAs" + struct_name);
        
        // create convenience method that doesn't require an existing object
        code += "fun {{gr_method}}(_bb: ByteBuffer): {{gr_name}} = \\";
        code += "{{gr_method}}(_bb, {{gr_name}}())";
        
        // create method that allows object reuse
        // e.g. fun Monster getRootAsMonster(_bb: ByteBuffer, obj: Monster) {...}
        code += "fun {{gr_method}}"
                 "(_bb: ByteBuffer, obj: {{gr_name}}): {{gr_name}} {";
        code.IncrementIdentLevel();
        code += "_bb.order(ByteOrder.LITTLE_ENDIAN)";
        code += "return (obj.assign(_bb.getInt(_bb.position())"
                 " + _bb.position(), _bb))";
        code.DecrementIdentLevel();
        code += "}";
    }
    
    static void GenerateStaticConstructor(const StructDef &struct_def,
                                          CodeWriter &code) {
        // create a struct constructor function
        GenerateFun(code, 
                    "create" + struct_def.name, 
                    StructConstructorParams(struct_def),
                    "Int",
                    [&](CodeWriter &code){
            GenStructBody(struct_def, code, "");
            code += "return builder.offset()";
        });
    }
    
    static std::string StructConstructorParams(const StructDef &struct_def, 
                                               std::string prefix = "") {
        //builder: FlatBufferBuilder
        std::stringstream out;
        auto field_vec = struct_def.fields.vec;
        if (prefix.empty()) {
            out << "builder: FlatBufferBuilder";
        }
        for (auto it = field_vec.begin(); it != field_vec.end(); ++it) {
            auto &field = **it;
            if (IsStruct(field.value.type)) {
                // Generate arguments for a struct inside a struct. To ensure names
                // don't clash, and to make it obvious these arguments are constructing
                // a nested struct, prefix the name with the field name.
                out << StructConstructorParams(*field.value.type.struct_def,
                                                  prefix + (field.name + "_"));
            } else {
                out << ", " << prefix << MakeCamel(field.name, false) << ": "
                    << GenTypeBasic(DestinationType(field.value.type, false));
            }
        }
        return out.str();
    }
    
    static void GeneratePropertyOneLine(CodeWriter &code,
                               std::string name, 
                               std::string type,  
                               CodeblockFunction body) {
        // Generates Kotlin getter for properties
        // e.g.: 
        // val prop: Mytype = x
        code.SetValue("_name", name);
        code.SetValue("_type", type);
        code += "val {{_name}} : {{_type}} = \\";
        body(code);
    }
    static void GenerateGetterOneLine(CodeWriter &code,
                               std::string name, 
                               std::string type,  
                               CodeblockFunction body) {
        // Generates Kotlin getter for properties
        // e.g.: 
        // val prop: Mytype get() = x
        code.SetValue("_name", name);
        code.SetValue("_type", type);
        code += "val {{_name}} : {{_type}} get() = \\";
        body(code);
    }
    
    static void GenerateGetter(CodeWriter &code,
                               std::string name, 
                               std::string type,  
                               CodeblockFunction body) {
        // Generates Kotlin getter for properties
        // e.g.: 
        // val prop: Mytype
        //     get() = {
        //       return x
        //     }
        code.SetValue("name", name);
        code.SetValue("type", type);
        code += "val {{name}} : {{type}}";
        code.IncrementIdentLevel();
        code += "get() {";
        code.IncrementIdentLevel();
        body(code);
        code.DecrementIdentLevel();
        code += "}";
        code.DecrementIdentLevel();
    }
    
    static void GenerateFun(CodeWriter &code,
                            std::string name,
                            std::string params,
                            std::string returnType,  
                            CodeblockFunction body) {
        // Generates Kotlin function
        // e.g.: 
        // fun path(j: Int): Vec3 { 
        //     return path(Vec3(), j)
        // } 
        auto noreturn = returnType.empty();
        code.SetValue("name", name);
        code.SetValue("params", params);
        code.SetValue("return_type", noreturn ? "" : ": " + returnType);
        code += "fun {{name}}({{params}}) {{return_type}} {";
        code.IncrementIdentLevel();
        body(code);
        code.DecrementIdentLevel();
        code += "}";
    }
    
    static void GenerateFunOneLine(CodeWriter &code, 
                                   std::string name,
                                   std::string params,
                                   std::string returnType,  
                                   CodeblockFunction body) {
        // Generates Kotlin function
        // e.g.: 
        // fun path(j: Int): Vec3 = return path(Vec3(), j)
        code.SetValue("name", name);
        code.SetValue("params", params);
        code.SetValue("return_type_p", returnType.empty() ? "" : 
                                                          " : " + returnType);
        code += "fun {{name}}({{params}}){{return_type_p}} = \\";
        body(code);
    }
    
    static void GenerateOverrideFun(CodeWriter &code, 
                                   std::string name,
                                   std::string params,
                                   std::string returnType,  
                                   CodeblockFunction body) {
        // Generates Kotlin function
        // e.g.: 
        // override fun path(j: Int): Vec3 = return path(Vec3(), j)
        code += "override \\";
        GenerateFun(code, name, params, returnType, body);
    }
    
    static void GenerateOverrideFunOneLine(CodeWriter &code, 
                                   std::string name,
                                   std::string params,
                                   std::string returnType,  
                                   std::string statement) {
        // Generates Kotlin function
        // e.g.: 
        // override fun path(j: Int): Vec3 = return path(Vec3(), j)
        code.SetValue("name", name);
        code.SetValue("params", params);
        code.SetValue("return_type", returnType.empty() ? "" : 
                                                          " : " + returnType);
        code += "override fun {{name}}({{params}}){{return_type}} = \\";
        code += statement;
    }
    
    static void GenerateFunOneLine(CodeWriter &code, 
                                   std::string name,
                                   std::string params,
                                   std::string returnType,  
                                   std::string statement) {
        // Generates Kotlin function
        // e.g.: 
        // fun path(j: Int): Vec3 = return path(Vec3(), j)
        code.SetValue("name", name);
        code.SetValue("params", params);
        code.SetValue("return_type", returnType.empty() ? "" : 
                                                          " : " + returnType);
        code += "fun {{name}}({{params}}){{return_type}} = \\";
        code += statement;
    }
    
    static std::string OffsetWrapperOneLine(std::string offset, std::string found,
                                            std::string not_found) {
        return "val o = __offset(" + offset + "); return if (o != 0) " + found + 
                " else " + not_found;
    }
    
    static void OffsetWrapper(CodeWriter &code, 
                       std::string offset, 
                       std::string found,
                       std::string not_found) {
        code += "val o = __offset(" + offset + ")";
        code +="return if (o != 0) {";
        code.IncrementIdentLevel();
        code += found;
        code.DecrementIdentLevel();
        code += "} else {";
        code.IncrementIdentLevel();
        code += not_found;
        code.DecrementIdentLevel();
        code += "}";
    }
    
    static std::string Indirect(std::string index, bool fixed) {
        // We apply __indirect() and struct is not fixed.
        if (!fixed)
            return "__indirect(" + index + ")";
        return index;
    }
    static std::string NotFoundReturn(BaseType el) {
        if (el == BASE_TYPE_DOUBLE)
            return "0.0";
        if (el == BASE_TYPE_FLOAT)
            return "0.0f";
        if (el == BASE_TYPE_BOOL)
            return "false";
        if (IsScalar(el))
            return "0";
        return "null";
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
