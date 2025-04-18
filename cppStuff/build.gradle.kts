import org.gradle.internal.jvm.Jvm

plugins {
    id("cpp-library")
}

toolChains{
    withType<VisualCpp>().configureEach {
        setInstallDir("C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools")
    }
}

library {
    baseName = "OpenCVStuff"
    source.from(file("src/main/cpp/opencv/Otsu.cpp"))
    publicHeaders.from(file("src/main/cpp/jni/org_askolds_cmptest_opencv_OpenCV.h"))

    binaries.configureEach {
        compileTask.get().includes.from(compileTask.get().targetPlatform.map {
            listOf(File("${Jvm.current().javaHome.canonicalPath}/include")) + when {
                it.operatingSystem.isMacOsX -> listOf(File("${Jvm.current().javaHome.absolutePath}/include/darwin"))
                it.operatingSystem.isLinux -> listOf(File("${Jvm.current().javaHome.absolutePath}/include/linux"))
                it.operatingSystem.isWindows -> listOf(File("${Jvm.current().javaHome.absolutePath}/include/win32"))
                else -> emptyList()
            } + File("C:\\opencv\\build\\include") // TODO: Fix this
        })
        compileTask.get().isPositionIndependentCode = true
    }

    linkage.add(Linkage.SHARED)
    targetMachines.addAll(machines.linux.x86_64, machines.windows.x86_64, machines.macOS.x86_64)
}

tasks.withType<LinkSharedLibrary>().configureEach {
    linkerArgs.addAll(
        listOf(
            "/LIBPATH:C:\\opencv\\build\\x64\\vc16\\lib", // Adjust based on your setup
            "opencv_world4110.lib" // Change to match the OpenCV version you have
        )
    )
}

tasks.register<Copy>("copyJniLibraries") {
    from(layout.buildDirectory.dir("lib/main/debug")) {
        include("**/*.so", "**/*.dll", "**/*.dylib")
    }
    eachFile { path = name }
    includeEmptyDirs = false
    into("../composeApp/src/desktopMain/resources/files/libs")
}

tasks.named("build") {
    finalizedBy("copyJniLibraries")
}