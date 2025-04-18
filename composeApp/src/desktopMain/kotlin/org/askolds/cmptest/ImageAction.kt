package org.askolds.cmptest

sealed interface ImageAction {
    val mask: String

    fun execute()
}

data class ColorIndex(
    val red: Double,
    val green: Double,
    val blue: Double,
    override val mask: String
) : ImageAction {
    override fun execute() {
        TODO("Not yet implemented")
    }
}

data class Otsu(
    override val mask: String
) : ImageAction {
    override fun execute() {
        TODO("Not yet implemented")
    }
}