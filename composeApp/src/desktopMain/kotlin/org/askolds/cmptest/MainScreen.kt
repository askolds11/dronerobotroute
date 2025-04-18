package org.askolds.cmptest

import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.lifecycle.viewmodel.compose.viewModel
import org.askolds.cmptest.composables.ColorIndex

@Composable
fun MainScreen(
    viewModel: MainScreenViewModel = viewModel()
) {
    var red by remember { mutableFloatStateOf(0f )}
    var green by remember { mutableFloatStateOf(0f )}
    var blue by remember { mutableFloatStateOf(0f )}
    LazyColumn {
        item {
            ColorIndex(
                red,
                { red = it },
                green,
                { green = it },
                blue,
                { blue = it }
            )
        }
    }
}

