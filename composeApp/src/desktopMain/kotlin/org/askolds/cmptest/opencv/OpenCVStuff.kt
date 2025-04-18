package org.askolds.cmptest.opencv

class OpenCVStuff {
    fun Otsu1(): String = otsu("", "", "", 0, 0)

    private external fun otsu(
        src: String,
        mask: String,
        workDir: String,
        nr: Int,
        repeat: Int
    ): String
    private external fun colorIndex(
        src: String,
        workDir: String,
        nr: String,
        red: Double,
        green: Double,
        blue: Double
    ): String

    init {
        LibraryLoader.loadLibraryFromResources("OpenCVStuff")
    }
}