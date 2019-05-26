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

static TypedFloatConstantGenerator JavaFloatGen("Double.", "Float.", "NaN",
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
            code += "import java.lang.*\n";
            code += "import java.util.*\n";
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
            return "(Long)";
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
            if (castFromDest) {
                if (type.base_type == BASE_TYPE_UINT)
                    return "(Int)";
                else if (type.base_type == BASE_TYPE_USHORT)
                    return "(Short)";
                else if (type.base_type == BASE_TYPE_UCHAR)
                    return "(Byte)";
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
            if (IsFloat(value.type.base_type))
                return JavaFloatGen.GenFloatConstant(field);
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
        code += "class " + enum_def.name;
        code += "{\n";
        code += "  private " + enum_def.name + "() { }\n";
        for (auto it = enum_def.Vals().begin(); it != enum_def.Vals().end(); ++it) {
            auto &ev = **it;
            GenComment(ev.doc_comment, code_ptr, &comment_config, "  ");
            code += "  static final";
            code += GenTypeBasic(enum_def.underlying_type);
            code += " " + ev.name + " = ";
            code += enum_def.ToString(ev) + "\n";
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
            code += "\n  static final";
            code += "String";
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
            code += "String";
            code += " " + MakeCamel("name");
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
            return "__string";
        case BASE_TYPE_STRUCT:
            return "__struct";
        case BASE_TYPE_UNION:
            return "__union";
        case BASE_TYPE_VECTOR:
            return GenGetter(type.VectorType());
        default: {
            std::string getter = "bb.get";
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
            std::string setter = std::string("") + "bb.put";
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
                        nameprefix + MakeCamel(field.name);
                code += argname + "\\";
                code += ")";
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
        
        writer += "class {{struct_name}} : {{superclass}} {\n";
        
        writer.IncrementIdentLevel();
        
        {
            // Generate the __init() method that sets the field in a pre-existing
            // accessor object. This is to allow object reuse.
            GenerateFun(writer, "__init", "_i: Int, _bb: ByteBuffer", "",
                        [&](CodeWriter &writer) {
                writer += "bb_pos = _i";
                writer += "bb = _bb";
                if (!struct_def.fixed) {
                    writer += "vtable_start = bb_pos - bb.getInt(bb_pos)";
                    writer += "vtable_size = bb.getShort(vtable_start)";
                }       
            });
            
            // Generate __assign method
            GenerateFunOneLine(writer, "__assign", "_i: Int, _bb: ByteBuffer",  
                        struct_def.name,
                        [&](CodeWriter &writer) {
                writer += "__init(_i, _bb); return this";
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
                        std::stringstream params;
                        params << "obj: " << struct_def.name << ", ";
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
                                        "getBytes(Table.UTF8_CHARSET.get())";
                            }
                            code += "val span = bb.getInt(vectorLocation - 4)";
                            code += "val start = 0";
                            code += "while (span != 0) {";
                            code.IncrementIdentLevel();
                            code += "val middle = span / 2";
                            code += "int tableOffset = __indirect(vector"
                                    "Location + 4 * (start + middle), bb);";
                            if (key_field->value.type.base_type == BASE_TYPE_STRING) {
                                code += "int comp = compareStrings(\\";
                                code += GenOffsetGetter(key_field) + "\\";
                                code += ", byteKey, bb)";
                            } else {
                                auto get_val = GenGetterForLookupByKey(key_field, "bb");
                                code += "val value = " + get_val;
                                code += "val comp = (value - key).sign";
                            }
                            code += "if (comp > 0) {";
                            code.IncrementIdentLevel();
                            code += "span = middle";
                            code.DecrementIdentLevel();
                            code += "} else if (comp < 0) {";
                            code.IncrementIdentLevel();
                            code += "middle++";
                            code += "start += middle";
                            code += "span -= middle";
                            code.DecrementIdentLevel();
                            code += "} else {";
                            code.IncrementIdentLevel();
                            code += "return (if (obj == null) "
                                    "{{struct_name}}() else obj)"
                                    ".__assign(tableOffset, bb)";
                            code.DecrementIdentLevel();
                            code += "return null";
                            code.DecrementIdentLevel();
                            code += "}";
                        };
                        GenerateFun(writer, "__lookup_by_key", 
                                    params.str(), 
                                    struct_def.name, 
                                    statements);
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
                code += field.name;
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
            code += "builder.startVector({{size}}, data.length, {{align}})";
            code += "for (i in length - 1 downTo 0) {";
            code.IncrementIdentLevel();
            code += "add{{root}}({{cast}} data[i])";
            code += "return bulder.endVector()";
            code.DecrementIdentLevel();
            code += "}";
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
            code.SetValue("method_name", GenMethod(field.value.type));
            code.SetValue("pos", field_pos);
            code.SetValue("default", GenDefaultValue(field, false));
            code.SetValue("cast", SourceCastBasic(field.value.type));
            
            code += "builder.add{{method_name}}({{pos}}, \\";
            code += "{{cast}}{{field_name}}, {{cast}}{{default}}))";
        });
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
                            code.SetValue("field_name", MakeCamel(field.name));
                            
                            code += "add{{field_name}}(builder, {{field_name}}\\";
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
            
            std::string code_ptr;
            
            GenComment(field.doc_comment, &code_ptr, &comment_config, "  ");
            
            writer += code_ptr;
            
            auto field_name = MakeCamel(field.name, false);
            auto field_type = GenTypeGet(field.value.type);
            auto field_default = GenDefaultValue(field);
            auto return_type = GenTypeNameDest(field.value.type);
            auto field_mask = DestinationMask(field.value.type, true);
            auto src_cast = SourceCast(field.value.type);
            auto getter = DestinationCast(field.value.type) + GenGetter(field.value.type);
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
            
            // Generate the accessors that don't do object reuse.
            if (field.value.type.base_type == BASE_TYPE_STRUCT) {
                // Calls the accessor that takes an accessor object with a new object.
                // val pos
                //     get() = return pos(Vec3())
                GenerateGetterOneLine(writer, field_name, field_type, [&](CodeWriter &writer){
                    writer += "return {{field_name}}({{field_type}}())";
                });
            } else if (field.value.type.base_type == BASE_TYPE_VECTOR &&
                       field.value.type.element == BASE_TYPE_STRUCT) {
                // Accessors for vectors of structs also take accessor objects, this
                // generates a variant without that argument.
                // e.g. fun weapons(j: Int) = weapons(Weapon(), j)
                GenerateFunOneLine(writer, field_name, "j: Int", return_type, 
                            [&](CodeWriter &writer){
                    writer += "return {{return_type}}(), j)";
                });
            }
            
            writer += "";
            
            if (IsScalar(field.value.type.base_type)) {
                if (struct_def.fixed) {
                    GenerateGetterOneLine(writer, field_name, field_type, 
                                   [&](CodeWriter &writer){
                        writer += "return {{field_read_func}}(bb_pos + {{offset}}) {{field_mask}}";
                    }); 
                } else {
                    GenerateGetter(writer, field_name, field_type, 
                                   [&](CodeWriter &writer){
                        writer += "val o = __offset({{offset}})";
                        writer += "return if(o != 0) {{field_read_func}}(o + bb_pos) {{field_mask}} else {{field_default}}";
                    });
                }
            } else {
                switch (field.value.type.base_type) {
                case BASE_TYPE_STRUCT:
                    
                    if (struct_def.fixed) {
                        // create getter with object reuse
                        // e.g.
                        //  fun pos(obj: Vec3) : Vec3? = obj.__assign(bb_pos + 4, bb)
                        // ? adds nullability annotation
                        GenerateFunOneLine(writer, field_name, "obj: " + field_type , 
                                    field_type + "?", 
                                    [&](CodeWriter &writer){
                            writer += "obj.__assign(bb_pos + {{offset}}, bb)";
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
                        GenerateFun(writer, field_name, "obj: " + field_type, 
                                    field_type + "?", [&](CodeWriter &writer){
                            auto fixed = field.value.type.struct_def->fixed;
                            
                            writer.SetValue("seek", Indirect("o + bb_pos", fixed));
                            
                            writer += "val o = __offset({{offset}})";
                            writer += "return if (o != 0) {";
                            writer.IncrementIdentLevel();
                            writer += "obj.__assign({{seek}}, bb)";
                            writer.DecrementIdentLevel();
                            writer += "else {";
                            writer.IncrementIdentLevel();
                            writer += "null";
                            writer.DecrementIdentLevel();
                            writer += "}";
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
                    GenerateGetter(writer, field_name, field_type + "?", 
                                   [&](CodeWriter &writer){
                        
                        writer += "val o = __offset({{offset}})";
                        writer += "return if (o != 0) __string(o + bb_pos) else null";
                    });
                    break;
                case BASE_TYPE_VECTOR: {
                    // e.g.
                    // public int inventory(int j) { int o = __offset(14); return o != 0 ? bb.get(__vector(o) + j * 1) & 0xFF : 0; }
                    
                    auto vectortype = field.value.type.VectorType();
                    std::string params = "j: Int";
                    std::string nullable = "";
                    
                    if (vectortype.base_type == BASE_TYPE_STRUCT || 
                            vectortype.base_type == BASE_TYPE_UNION) {
                        params = "obj: " + field_type + ", j: Int";
                        // make return nullable for objects
                        nullable = "?";
                    }
                    
                    GenerateFun(writer, field_name, params, 
                                field_type + nullable, 
                                [&](CodeWriter &writer){
                        auto inline_size = NumToString(InlineSize(vectortype));
                        auto index = "__vector(o) + j * " + inline_size;
                        bool fixed = struct_def.fixed || 
                                vectortype.base_type != BASE_TYPE_STRUCT;
                        
                        writer.SetValue("index", Indirect(index, fixed));
                        
                        writer += "val o = __offset({{offset}})";
                        writer += "return if (o != 0) {";
                        writer.IncrementIdentLevel();
                        if (vectortype.base_type == BASE_TYPE_STRUCT) {
                            writer += "obj.__assign({{index}}, bb) {{field_mask}}";    
                        }
                        else if (vectortype.base_type == BASE_TYPE_UNION) {
                            writer += "{{field_read_func}}({{index}} - bb_pos) {{field_mask}}";
                        } else {
                            writer += "{{field_read_func}}({{index}}) {{field_mask}}"; 
                        }
                        writer.DecrementIdentLevel();
                        writer += "else {";
                        writer.IncrementIdentLevel();
                        writer += NotFoundReturn(field.value.type.element);
                        writer.DecrementIdentLevel();
                        writer += "}";
                    });
                    break;
                }
                case BASE_TYPE_UNION:
                    GenerateFun(writer, field_name, "obj: " + field_type, 
                                field_type + "?", 
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
                            writer += "fun " +field_name + "ByKey(\\";
                            writer += "key: " + GenTypeNameDest(key_field.value.type) + ") : " + qualified_name + "{\n" + offset_prefix + "\\";
                            writer += qualified_name + ".__lookup_by_key(null, __vector(o), key, bb) : null";
                            writer += "}";
                            
                            writer += "fun " +field_name + "ByKey(\\";
                            writer += "obj: " + qualified_name + ", key: " + GenTypeNameDest(key_field.value.type) + ") : " + qualified_name + "{\n" + offset_prefix + qualified_name + ".__lookup_by_key(obj, __vector(o), key, bb) : null; ";
                            writer += "}\n";
                            break;
                        }
                    }
                }
            }
            
            if ((field.value.type.base_type == BASE_TYPE_VECTOR &&
                 IsScalar(field.value.type.VectorType().base_type)) ||
                    field.value.type.base_type == BASE_TYPE_STRING) {
                
                auto fieldName = field_name + "ByteBuffer";
                auto end_idx = NumToString(field.value.type.base_type == BASE_TYPE_STRING
                                           ? 1
                                           : InlineSize(field.value.type.VectorType()));
                // Generate a ByteBuffer accessor for strings & vectors of scalars.
                // e.g.
                // val inventoryByteBuffer: ByteBuffer
                //     get =  __vector_as_bytebuffer(14, 1)
                
                GenerateGetterOneLine(writer, fieldName, "ByteBuffer", 
                               [&](CodeWriter &code){
                    code.SetValue("end", end_idx);
                    code += "__vector_as_bytebuffer({{offset}}, {{end}})";
                });
                
                // Generate a ByteBuffer accessor for strings & vectors of scalars.
                // e.g.
                // fun inventoryByteBuffer(_bb: Bytebuffer): ByteBuffer = __vector_as_bytebuffer(_bb, 14, 1)
                GenerateFunOneLine(writer, fieldName, "_bb: ByteBuffer", "ByteBuffer", 
                            [&](CodeWriter &writer){
                    writer.SetValue("end", end_idx);
                    writer += "__vector_as_bytebuffer(_bb, {{offset}}, {{end}})";
                });
            }
            
            //TODO: PV not done
            // generate object accessors if is nested_flatbuffer
            if (field.nested_flatbuffer) {
                auto nested_type_name = WrapInNameSpace(*field.nested_flatbuffer);
                auto nested_method_name =
                        field_name + "As" +
                        nested_type_name;
                auto get_nested_method_name = nested_method_name;
                
                writer += nested_type_name + " " + nested_method_name + "() { return " + get_nested_method_name + "(" + nested_type_name + "()) }";
                writer += nested_type_name + " " + get_nested_method_name + "(" + nested_type_name + " obj"") { int o = __offset({{offset}}); ";
                writer += "return o != 0 ? obj.__assign(__indirect(__vector(o)), bb) : null; }\n";
            }
            
            //TODO: PV not done
            // Generate mutators for scalar fields or vectors of scalars.
            if (parser_.opts.mutable_buffer) {
                auto underlying_type = field.value.type.base_type == BASE_TYPE_VECTOR
                        ? field.value.type.VectorType()
                        : field.value.type;
                // Boolean parameters have to be explicitly converted to byte
                // representation.
                auto setter_parameter = underlying_type.base_type == BASE_TYPE_BOOL
                        ? "(Byte)(" + field.name + " ? 1 : 0)"
                        : field.name;
                // A vector mutator also needs the index of the vector element it should
                // mutate.
                auto params = field.name + ": " + GenTypeNameDest(underlying_type);
                if (field.value.type.base_type == BASE_TYPE_VECTOR)
                    params.insert(0, "j: Int, ");
                
                auto return_type = struct_def.fixed ? "" : "Boolean";
                auto setter_index =
                        field.value.type.base_type == BASE_TYPE_VECTOR
                        ? std::string("") + "__vector(o) + j * " +
                          NumToString(InlineSize(underlying_type))
                        : (struct_def.fixed
                           ? "bb_pos + {{offset}}"
                           : "o + bb_pos");
                if (IsScalar(field.value.type.base_type) ||
                        (field.value.type.base_type == BASE_TYPE_VECTOR &&
                         IsScalar(field.value.type.VectorType().base_type))) {
                    GenerateFun(writer,"mutate" + MakeCamel(field.name, true), 
                                params, return_type, [&] (CodeWriter & writer) {
                        writer.SetValue("gensetter", GenSetter(underlying_type));
                        if (struct_def.fixed) {
                            writer += "{{gensetter}}(" + setter_index + ", ";
                            writer += src_cast + setter_parameter + ")";
                        } else {
                            writer += "val o = __offset({{offset}})";
                            writer += "return if (o != 0) {";
                            writer.IncrementIdentLevel();
                            writer += "{{gensetter}}(" + setter_index + ", " + src_cast + setter_parameter +
                                    ")";
                            writer +="true";
                            writer.DecrementIdentLevel();
                            writer += "} else {";
                            writer.IncrementIdentLevel();
                            writer +="false";
                            writer.DecrementIdentLevel();
                            writer += "}";
                        }
                    });
                }
            }
        }
        if (struct_def.has_key) {
            // Key Comparison method
            GenerateOverrideFunOneLine(
                        writer,
                        "keysCompare",
                        "Integer o1, Integer o2, ByteBuffer _bb",
                        "Int", GenKeyGetter(key_field));
        }
    }
    
    void GenerateCompanionObject(CodeWriter &code, 
                                 CodeblockFunction callback) const {
        code += "companion object {\n";
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
        code += "_bb.order(ByteOrder.LITTLE_ENDIAN);";
        code += "return (obj.__assign(_bb.getInt(_bb.position())"
                 " + _bb.position(), _bb))";
        code.DecrementIdentLevel();
        code += "}";
    }
    
    static void GenerateStaticConstructor(const StructDef &struct_def,
                                          CodeWriter &code) {
        // create a struct constructor function
        code.SetValue("gsc_name", struct_def.name);
        code += "fun create{{gsc_name}}(builder: FlatBufferBuilder\\";
        GenStructArgs(struct_def, code, "");
        code += "): Int {\n";
        GenStructBody(struct_def, code, "");
        code += "    return ";
        code += "builder.offset()";
        code += "\n  }\n";
    }
    
    static void GenerateGetterOneLine(CodeWriter &code,
                               std::string name, 
                               std::string type,  
                               CodeblockFunction body) {
        // Generates Kotlin getter for properties
        // e.g.: 
        // val prop: Mytype
        //     get() = return x
        code.SetValue("_name", name);
        code.SetValue("_type", type);
        code += "val {{_name}} : {{_type}}";
        code.IncrementIdentLevel();
        code += "get() = \\";
        body(code);
        code.DecrementIdentLevel();
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
        code.SetValue("name", name);
        code.SetValue("params", params);
        code.SetValue("return_type", returnType);
        code += "fun {{name}}({{params}}) : {{return_type}} {";
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
    
    static std::string Indirect(std::string index, bool fixed) {
        // We apply __indirect() and struct is not fixed.
        if (!fixed)
            return "__indirect(" + index + ")";
        return index;
    }
    static std::string NotFoundReturn(BaseType el) {
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
