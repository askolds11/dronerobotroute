package org.askolds.cmptest

import androidx.compose.material3.MaterialTheme
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application

fun main() = application {
    Window(
        onCloseRequest = ::exitApplication,
        title = "CMPTest",
    ) {
//        App()
        MaterialTheme {
            MainScreen()
        }
    }
}