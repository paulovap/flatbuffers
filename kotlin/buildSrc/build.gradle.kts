plugins {
  groovy
  `kotlin-dsl`
}

repositories {
  mavenCentral()
  maven {
    url = uri("https://plugins.gradle.org/m2/")
  }
  gradlePluginPortal()
  mavenCentral()
}

dependencies {
  implementation("org.jetbrains.kotlin.jvm:org.jetbrains.kotlin.jvm.gradle.plugin:1.5.31")
  implementation(kotlin("gradle-plugin-api", version = "1.4.0"))
  implementation(gradleApi())
  implementation(localGroovy())
}
