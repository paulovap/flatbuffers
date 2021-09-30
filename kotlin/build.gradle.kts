import org.gradle.internal.impldep.org.testng.ITestResult.STARTED
import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import java.nio.charset.StandardCharsets

plugins {
  id("com.diffplug.spotless") version "6.1.0"
}

subprojects {

  repositories {
    maven { setUrl("https://plugins.gradle.org/m2/") }
    mavenCentral()
  }

  subprojects {

    tasks.withType<KotlinCompile>().configureEach {
      kotlinOptions {
        jvmTarget = JavaVersion.VERSION_1_8.toString()
        @Suppress("SuspiciousCollectionReassignment")
        freeCompilerArgs += "-Xjvm-default=all"
      }
    }

    tasks.withType<JavaCompile> {
      options.encoding = StandardCharsets.UTF_8.toString()
      sourceCompatibility = JavaVersion.VERSION_1_8.toString()
      targetCompatibility = JavaVersion.VERSION_1_8.toString()
    }
  }
}

buildscript {
  repositories {
    maven { setUrl("https://plugins.gradle.org/m2/") }
    gradlePluginPortal()
    mavenCentral()
  }
}

// plugin used to enforce code style
spotless {
  val klintConfig = mapOf("indent_size" to "2", "continuation_indent_size" to "2")
  kotlin {
    target("**/*.kt")
    ktlint("0.40.0").userData(klintConfig)
    trimTrailingWhitespace()
    indentWithSpaces()
    endWithNewline()
    licenseHeaderFile("$rootDir/spotless/spotless.kt").updateYearWithLatest(false)
    targetExclude("**/spotless.kt", "**/build/**")
  }
  kotlinGradle {
    target("*.gradle.kts")
    ktlint().userData(klintConfig)
  }
}

tasks.withType<KotlinCompile>().configureEach {
  kotlinOptions {
    jvmTarget = JavaVersion.VERSION_1_8.toString()
    @Suppress("SuspiciousCollectionReassignment")
    freeCompilerArgs += "-Xjvm-default=all -Xextended-compiler-checks"

  }
}

tasks.withType<JavaCompile> {
  options.encoding = StandardCharsets.UTF_8.toString()
  sourceCompatibility = JavaVersion.VERSION_1_8.toString()
  targetCompatibility = JavaVersion.VERSION_1_8.toString()
}
