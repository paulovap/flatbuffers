#!/bin/sh

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
thisdir=$(pwd)

if [[ "$testdir" != "$thisdir" ]]; then
	echo error: must be run from inside the ${testdir} directory
	echo you ran it from ${thisdir}
	exit 1
fi

kotlinc KotlinTest.kt ${testdir}/../kotlin/* ${testdir}/MyGame/Example/*.kt -include-runtime -d KotlinTest.jar 
if [ $? -ne 0 ]; then
    echo "Failed Kotlin compilation, look into logs.txt for error output"
    exit 1
fi
java -jar KotlinTest.jar
if [ $? -ne 0 ]; then
    echo "Failed Test compilation or failed tests"
fi
