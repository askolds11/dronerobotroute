import org.gradle.internal.jvm.Jvm
import org.jetbrains.compose.desktop.application.dsl.TargetFormat
import java.io.ByteArrayOutputStream

plugins {
    alias(libs.plugins.kotlinMultiplatform)
    alias(libs.plugins.composeMultiplatform)
    alias(libs.plugins.composeCompiler)
//    id("cpp-library")
}

kotlin {
    jvm("desktop") {
//        val processResources = compilations["main"].processResourcesTaskName
//        (tasks[processResources] as ProcessResources).apply {
//            // A task that build the JNI shared library for all targets and
//            // copy each outputs into $buildDir/resources-jni
//            dependsOn("buildJniNative")
//            from("${layout.buildDirectory}/resources-jni")
//        }
    }
    
    sourceSets {
        val desktopMain by getting

        commonMain.dependencies {
            implementation(compose.runtime)
            implementation(compose.foundation)
            implementation(compose.material3)
            implementation(compose.ui)
            implementation(compose.components.resources)
            implementation(compose.components.uiToolingPreview)
            implementation(libs.androidx.lifecycle.viewmodel)
            implementation(libs.androidx.lifecycle.viewmodel.compose)
            implementation(libs.androidx.lifecycle.runtime.compose)
        }
        desktopMain.dependencies {
            implementation(compose.desktop.currentOs) {
                exclude("org.jetbrains.compose.material")
            }
            implementation(libs.kotlinx.coroutines.swing)
            implementation(libs.java.native.jssc)
            implementation(libs.javacv)
            implementation(libs.components.splitpane.desktop)
            runtimeOnly(fileTree("src/desktopMain/resources") { include("*.dll", "*.so", "*.dylib") })
        }
    }
}


compose.desktop {
    application {
        mainClass = "org.askolds.cmptest.MainKt"

        nativeDistributions {
            appResourcesRootDir = rootDir.resolve("assets")
            targetFormats(TargetFormat.Dmg, TargetFormat.Msi, TargetFormat.Deb)
            packageName = "org.askolds.cmptest"
            packageVersion = "1.0.0"
        }
    }
}

tasks.register("generateJniHeaders") {
    group = "_custom"
    dependsOn(tasks.getByName("build"))

    val jniHeaderDirectory = file("../cppStuff/src/main/cpp/jni")

    // For caching
    inputs.dir("src/desktopMain/kotlin")
    outputs.dir("../cppStuff/src/main/cpp")

    doLast {
        val javaHome = Jvm.current().javaHome
        val javap = javaHome.resolve("bin").walk().firstOrNull { it.name.startsWith("javap") }?.absolutePath ?: error("javap not found")
        val javac = javaHome.resolve("bin").walk().firstOrNull { it.name.startsWith("javac") }?.absolutePath ?: error("javac not found")
        val buildDir = file("build/classes/kotlin/desktop/main")
        val tmpDir = file("build/tmp/jvmJni").apply { mkdirs() }

        val bodyExtractingRegex = """^.+\Rpublic \w* ?class ([^\s]+).*\{\R((?s:.+))\}\R$""".toRegex()
        val nativeMethodExtractingRegex = """.*\bnative\b.*""".toRegex()

        buildDir.walkTopDown()
            .filter { "META" !in it.absolutePath }
            .forEach { file ->
                if (!file.isFile) return@forEach

                val output = ByteArrayOutputStream().use {
                    project.exec {
                        commandLine(javap, "-private", "-cp", buildDir.absolutePath, file.absolutePath)
                        standardOutput = it
                    }.assertNormalExitValue()
                    it.toString()
                }

                val (qualifiedName, methodInfo) = bodyExtractingRegex.find(output)?.destructured ?: return@forEach

                val lastDot = qualifiedName.lastIndexOf('.')
                val packageName = qualifiedName.substring(0, lastDot)
                val className = qualifiedName.substring(lastDot+1, qualifiedName.length)

                val nativeMethods =
                    nativeMethodExtractingRegex.findAll(methodInfo).map { it.groups }.flatMap { it.asSequence().mapNotNull { group -> group?.value } }.toList()
                if (nativeMethods.isEmpty()) return@forEach

                val source = buildString {
                    appendLine("package $packageName;")
                    appendLine("public class $className {")
                    for (method in nativeMethods) {
                        if ("()" in method) appendLine(method)
                        else {
                            val updatedMethod = StringBuilder(method).apply {
                                var count = 0
                                var i = 0
                                while (i < length) {
                                    if (this[i] == ',' || this[i] == ')') insert(i, " arg${count++}".also { i += it.length + 1 })
                                    else i++
                                }
                            }
                            appendLine(updatedMethod)
                        }
                    }
                    appendLine("}")
                }
                val outputFile = tmpDir.resolve(packageName.replace(".", "/")).apply { mkdirs() }.resolve("$className.java").apply { delete() }.apply { createNewFile() }
                outputFile.writeText(source)

                project.exec {
                    commandLine(javac, "-h", jniHeaderDirectory.absolutePath, outputFile.absolutePath)
                }.assertNormalExitValue()
            }
    }
}