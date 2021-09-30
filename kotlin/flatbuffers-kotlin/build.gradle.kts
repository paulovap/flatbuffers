import org.gradle.internal.impldep.org.fusesource.jansi.AnsiRenderer.test
import org.jetbrains.kotlin.gradle.plugin.mpp.apple.XCFramework

plugins {
  kotlin("multiplatform")
}

group = "com.google.flatbuffers.kotlin"
version = "2.0.0-SNAPSHOT"

kotlin {
  explicitApi()
  jvm()
//  js {
//    browser {
//      binaries.executable()
//      testTask {
//        useKarma {
//          useChromeHeadless()
//        }
//      }
//    }
//  }
  macosX64()
  val ios32 = iosArm32()
  val ios64 = iosArm64()
  //val iosX64 = iosX64()
  val xcf = XCFramework()
  iosX64() {
    binaries.framework {
      baseName = "Flatbuffers"
      xcf.add(this)
    }
  }

  sourceSets {
    all {
      languageSettings.optIn("kotlin.ExperimentalUnsignedTypes")
    }
    val commonMain by getting {
      dependencies {
        implementation(kotlin("stdlib-common"))
      }
    }

    val commonTest by getting {
      dependencies {
        implementation(kotlin("test-common"))
        implementation(kotlin("test-annotations-common"))
      }
    }
    val jvmTest by getting {
      dependencies {
        implementation(kotlin("test-junit"))
        implementation("com.google.flatbuffers:flatbuffers-java:2.0.3")
      }
    }
    val jvmMain by getting {
      kotlin.srcDir("java")
      dependencies {
        implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core-jvm:1.4.1")
      }
    }

//    val jsMain by getting {
//      dependsOn(commonMain)
//    }
//    val jsTest by getting {
//      dependsOn(commonTest)
//      dependencies {
//        implementation(kotlin("test-js"))
//      }
//    }
    val nativeMain by creating {
        dependsOn(commonMain)
    }
    val nativeTest by creating {
      dependsOn(nativeMain)
    }
    val macosX64Main by getting {
      dependsOn(nativeMain)
    }

    val iosArm32Main by getting {
      dependsOn(nativeMain)
    }
    val iosArm64Main by getting {
      dependsOn(nativeMain)
    }
    val iosX64Main by getting {
      dependsOn(nativeMain)
    }
  }

  /* Targets configuration omitted.
   *  To find out how to configure the targets, please follow the link:
   *  https://kotlinlang.org/docs/reference/building-mpp-with-gradle.html#setting-up-targets */
  targets {
    targetFromPreset(presets.getAt("jvm"))
    //targetFromPreset(presets.getAt("js"))
    targetFromPreset(presets.getAt("macosX64"))
    targetFromPreset(presets.getAt("iosArm32"))
    targetFromPreset(presets.getAt("iosArm64"))
    targetFromPreset(presets.getAt("iosX64"))
  }
}

// Use the default greeting
tasks.register<com.google.flatbuffers.kotlin.GenerateFBTestClasses>("generateFBTestClassesKt") {
  inputFiles.setFrom("$rootDir/../tests/monster_test.fbs")
  includeFolder.set("$rootDir/../tests/include_test")
  outputFolder.set("${projectDir}/src/commonTest/kotlin/")
  variant.set("kotlin")
}

//../flatc --cpp --java --kotlin --csharp --ts --php  -o union_vector ./union_vector/union_vector.fbs
tasks.register<com.google.flatbuffers.kotlin.GenerateFBTestClasses>("generateFBTestClassesKtVec") {
  inputFiles.setFrom("$rootDir/../tests/union_vector/union_vector.fbs")
  outputFolder.set("${projectDir}/src/commonTest/kotlin/union_vector")
  variant.set("kotlin")
}

//../flatc --java --kotlin --lobster --ts optional_scalars.fbs
tasks.register<com.google.flatbuffers.kotlin.GenerateFBTestClasses>("generateFBTestClassesKtOptionalScalars") {
  inputFiles.setFrom("$rootDir/../tests/optional_scalars.fbs")
  outputFolder.set("${projectDir}/src/commonTest/kotlin/")
  variant.set("kotlin")
}

//../flatc --java --kotlin --lobster --ts optional_scalars.fbs
tasks.register<com.google.flatbuffers.kotlin.GenerateFBTestClasses>("generateFBTestClassesKtNameSpace") {
  inputFiles.setFrom("$rootDir/../tests/namespace_test/namespace_test1.fbs", "$rootDir/../tests/namespace_test/namespace_test2.fbs")
  outputFolder.set("${projectDir}/src/commonTest/kotlin/")
  variant.set("kotlin")
}


project.tasks.named("compileKotlinJvm") {
  dependsOn("generateFBTestClassesKt")
  dependsOn("generateFBTestClassesKtVec")
  dependsOn("generateFBTestClassesKtOptionalScalars")
  dependsOn("generateFBTestClassesKtNameSpace")
}
