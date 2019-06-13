#!/bin/sh

set -x 
# Copyright 2014 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

echo Compile then run the Kotlin test.

testdir=$(dirname $0)
targetdir="${testdir}/target"

if [[ -e "${targetdir}" ]]; then
    echo "cleaning target"
    rm -rf "${targetdir}"
fi

mkdir -v "${targetdir}"

if ! find "${testdir}/../java" -type f -name "*.class" -delete; then
    echo "failed to clean .class files from java directory" >&2
    exit 1
fi

namespace_files=`find ./namespace_test -name "*.kt" -print`
union_files=`find ./union_vector -name "*.kt" -print`
all_kt_files=`find . -name "*.kt" -print`
javac ${testdir}/../java/com/google/flatbuffers/*.java -d kotlin
kotlinc KotlinTest.kt  $namespace_files $union_files ${testdir}/MyGame/*.kt ${testdir}/MyGame/Example/*.kt -classpath "${testdir}/kotlin" -include-runtime -d kotlin
#kotlinc KotlinTest.kt -classpath "${testdir}/kotlin" -include-runtime -d kotlin_test.jar
#kotlin  -classpath "${testdir}/kotlin:${testdir}/kotlin/com/google/flatbuffers/*" kotlin_test.jar

jar cvf kotlin_test.jar -C kotlin .
kotlin -cp kotlin_test.jar KotlinTest
#rm -rf kotlinTest.jar

