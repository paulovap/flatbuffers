/*
 * Copyright 2015 Google Inc. All rights reserved.
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

#include <stdio.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#include "flatbuffers/flexbuffers.h"

/*
 *  This program is a simple tool to convert json file into
 *  FlexBuffer files and vice-versa.
 */

void json_to_flexbuffer(std::string &input_src, std::string &output_src) {
  flexbuffers::Builder slb(512,
                           flexbuffers::BUILDER_FLAG_SHARE_KEYS_AND_STRINGS);
  flatbuffers::Parser parser;
  slb.Clear();

  std::string data;
  flatbuffers::LoadFile(input_src.c_str(), false, &data);

  if (!parser.ParseFlexBuffer(data.c_str(), nullptr, &slb)) {
    printf("Unable to convert json to flexbuffer\n");
    exit(1);
  }
  auto data_vec = slb.GetBuffer();
  auto data_ptr = reinterpret_cast<char *>(data_vec.data());

  if (!output_src.empty() &&
      !flatbuffers::SaveFile(output_src.c_str(), data_ptr, slb.GetSize(),
                             true)) {
    printf("Unable to save binary flexbuffer on disk\n");
    exit(1);
  }
}

void flexbuffer_to_json(std::string &input_src, std::string &output_src) {
  flexbuffers::Builder slb(512,
                           flexbuffers::BUILDER_FLAG_SHARE_KEYS_AND_STRINGS);
  slb.Clear();
  std::string data;
  flatbuffers::LoadFile(input_src.c_str(), true, &data);
  auto root = flexbuffers::GetRoot(
      reinterpret_cast<const uint8_t *>(data.c_str()), data.size());
  std::string json;
  root.ToString(true, true, json);
  if (!flatbuffers::SaveFile(output_src.c_str(), json.c_str(), json.size(),
                             true)) {
    printf("Unable to save json from flexbuffer on disk\n");
    exit(1);
  }
}

void testFoo() {
    int start = 0;
    int entriesCount = 11;
    flexbuffers::Builder builder;
    auto map = builder.StartMap();
    for (int i = start; i < entriesCount; i++) {
        builder.Add((std::string("foo_param_") + std::to_string(i)).c_str(), std::string("foo_value_") + std::to_string(i));
    }
    builder.EndMap(map);
    builder.Finish();

    auto data = builder.GetBuffer();
    //auto size = (int)builder.GetSize();
//    std::cout << "[";
//    for (int i=0; i < size; i++)
//        std::cout << (int)data[i] << ",";
//    std::cout << "]\n";
    auto rootReference = flexbuffers::GetRoot(data);

    assert(rootReference.IsMap());

    auto flexMap = rootReference.AsMap();
    assert (!flexMap.IsTheEmptyMap());
    //auto keys = flexMap.Keys();
    //auto values = flexMap.Values();

    //assert(entriesCount == (int)keys.size());
    //assert(entriesCount == (int)values.size());
}

int main(int argc, char *argv[]) {
  const char *name = argv[0];
  testFoo();
  if (argc <= 2) {
    printf("%s INPUT_FILE OUTPUT_FILE\n", name);
    return 1;
  }

  std::string input_src = argv[1];
  std::string output_src = argv[2];

  bool json_input = input_src.rfind(".json") != std::string::npos;
  bool flexb_input = input_src.rfind(".flexbs") != std::string::npos;

  if (!json_input && !flexb_input) {
    printf("Input file must be \"*.json\" or \"*.flexbs\"\n");
    return 1;
  }

  if (json_input) {
    json_to_flexbuffer(input_src, output_src);
  } else if (flexb_input) {
    flexbuffer_to_json(input_src, output_src);
  }
  return 0;
}
