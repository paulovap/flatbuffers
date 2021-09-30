/*
 * Copyright 2021 Google Inc. All rights reserved.
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
package com.google.flatbuffers.kotlin

import org.gradle.api.DefaultTask
import org.gradle.api.file.ConfigurableFileCollection
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputFiles
import org.gradle.api.tasks.TaskAction
import org.gradle.internal.component.external.model.ComponentVariant
import org.gradle.internal.impldep.com.amazonaws.services.kms.model.UnsupportedOperationException
import org.gradle.process.internal.ExecActionFactory
import javax.inject.Inject


abstract class GenerateFBTestClasses : DefaultTask() {
  @get:InputFiles
  abstract val inputFiles: ConfigurableFileCollection

  @get:Input
  abstract val includeFolder: Property<String>

  @get:Input
  abstract val outputFolder: Property<String>

  @get:Input
  abstract val variant: Property<String>

  @Inject
  protected open fun getExecActionFactory(): ExecActionFactory? {
    throw UnsupportedOperationException()
  }

  init {
    includeFolder.set("")
  }

  @TaskAction
  fun compile() {
    val execAction = getExecActionFactory()!!.newExecAction()
    val sources = inputFiles.asPath.split(":")
    val args = mutableListOf("/Users/ppinheiro/git_tree/flatbuffers/flatc","-o", outputFolder.get(), "--${variant.get()}")
    if (includeFolder.get().isNotEmpty()) {
      args.add("-I")
      args.add(includeFolder.get())
    }
    args.addAll(sources)
    println(args)
    execAction.setCommandLine(args)
    print(execAction.execute())
  }
}
