package org.askolds.cmptest.composables

import androidx.compose.desktop.ui.tooling.preview.Preview
import androidx.compose.foundation.background
import androidx.compose.foundation.hoverable
import androidx.compose.foundation.indication
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.LocalMinimumInteractiveComponentSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.SliderDefaults.Thumb
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.DpSize
import androidx.compose.ui.unit.dp


@Composable
fun ColorIndex(
    red: Float,
    updateRed: (Float) -> Unit,
    green: Float,
    updateGreen: (Float) -> Unit,
    blue: Float,
    updateBlue: (Float) -> Unit
) {
    val maxValue by remember { mutableFloatStateOf(10f) }
    Column(
        Modifier.padding(all = 16.dp)
    ) {
        Text("Color Index")
        ColorRow(red, updateRed, maxValue, Color.Red)
        ColorRow(green, updateGreen, maxValue, Color.Green)
        ColorRow(blue, updateBlue, maxValue, Color.Blue)
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ColorRow(
    value: Float,
    changeValue: (Float) -> Unit,
    maxValue: Float,
    color: Color,
    modifier: Modifier = Modifier
) {
    var text by remember { mutableStateOf("") }
    LaunchedEffect(Unit) {
        text = value.toString()
    }
    Row(
        modifier,
        verticalAlignment = Alignment.CenterVertically
    ) {
        val colors = SliderDefaults.colors(
            inactiveTrackColor = color.copy(alpha = 0.2f),
            activeTrackColor = color,
            thumbColor = color
        )
        val sliderInteractionSource = remember { MutableInteractionSource() }
        CompositionLocalProvider(LocalMinimumInteractiveComponentSize provides 0.dp) {
            Slider(
                value = value,
                onValueChange = {
                    val rounded = "%.2f".format(it)
                    changeValue(rounded.toFloat())
                    text = rounded
                },
                valueRange = 0f..maxValue,
                modifier = Modifier.weight(1f),
                colors = SliderDefaults.colors(
                    inactiveTrackColor = color.copy(alpha = 0.2f),
                    activeTrackColor = color,
                    thumbColor = color
                ),
                thumb = {
//                    Thumb(
//                        interactionSource = sliderInteractionSource,
//                        colors = colors,
//                        enabled = true,
//                        thumbSize = DpSize(16.dp, 16.dp)
//                    )
                    Spacer(
                        modifier = Modifier
                            .size(20.dp)
                            .hoverable(interactionSource = sliderInteractionSource)
                            .shadow(6.dp, CircleShape, clip = false)
                            .background(color, CircleShape)
                    )
                },
                track = { sliderState ->
//                    SliderDefaults.Track(
//                        sliderState = sliderState,
//                        thumbTrackGapSize = 0.dp,
//                        colors = colors
//                    )
                    // Calculate fraction of the slider that is active
                    val fraction by remember {
                        derivedStateOf {
                            (sliderState.value - sliderState.valueRange.start) / (sliderState.valueRange.endInclusive - sliderState.valueRange.start)
                        }
                    }

                    Box(Modifier.fillMaxWidth()) {
                        Box(
                            Modifier
                                .fillMaxWidth(fraction)
                                .align(Alignment.CenterStart)
                                .height(6.dp)
                                .background(colors.activeTrackColor, CircleShape)
                        )
                        Box(
                            Modifier
                                .fillMaxWidth(1f - fraction)
                                .align(Alignment.CenterEnd)
                                .height(6.dp)
                                .background(colors.inactiveTrackColor, CircleShape)
                        )
                    }
                }
            )
        }
        Spacer(Modifier.width(8.dp))
        val interactionSource = remember { MutableInteractionSource() }
        BasicTextField(
            value = text,
            onValueChange = {
                text = it
                changeValue(it.toFloatOrNull() ?: 0f)
            },
            keyboardOptions = KeyboardOptions.Default.copy(
                keyboardType = KeyboardType.Decimal
            ),
            singleLine = true,
            modifier = Modifier.width(60.dp),
            decorationBox = @Composable { innerTextField ->
                // places leading icon, text field with label and placeholder, trailing icon
                TextFieldDefaults.DecorationBox(
                    value = text,
                    innerTextField = innerTextField,
                    enabled = true,
                    singleLine = true,
                    visualTransformation = VisualTransformation.None,
                    interactionSource = interactionSource,
                    contentPadding = PaddingValues(4.dp)
                )
            }
        )
    }
}


@Preview
@Composable
private fun ColorIndexPreview() {
    MaterialTheme {
        ColorIndex(1f, {}, 2f, {}, 3f, {})
    }
}