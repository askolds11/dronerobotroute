package org.askolds.cmptest.opencv

import java.nio.file.Files.*
import java.nio.file.StandardCopyOption

object LibraryLoader {
    fun loadLibraryFromResources(libraryName: String) {
        val os = System.getProperty("os.name").lowercase()
        val libExtension = when {
            os.contains("win") -> "dll"
            os.contains("mac") -> "dylib"
            os.contains("linux") -> "so"
            else -> throw IllegalStateException("Invalid system $os")
        }

        val resourcePath = "/files/libs/$libraryName.$libExtension"
        val inputStream = object {}.javaClass.getResourceAsStream(resourcePath)
            ?: throw IllegalStateException("Library $resourcePath not found in resources")

        val tempFile = createTempFile(libraryName, ".$libExtension").toFile()
        tempFile.deleteOnExit()
        inputStream.use { copy(it, tempFile.toPath(), StandardCopyOption.REPLACE_EXISTING) }

        System.load(tempFile.absolutePath)
    }
}
