package org.askolds.cmptest

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import cmptest.composeapp.generated.resources.Res
import cmptest.composeapp.generated.resources.compose_multiplatform
import jssc.SerialPortList
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.bytedeco.javacv.JavaCV.FLT_EPSILON
import org.bytedeco.opencv.global.opencv_core.ACCESS_READ
import org.bytedeco.opencv.global.opencv_core.CV_32F
import org.bytedeco.opencv.global.opencv_core.CV_32S
import org.bytedeco.opencv.global.opencv_core.CV_8U
import org.bytedeco.opencv.global.opencv_core.NORM_MINMAX
import org.bytedeco.opencv.global.opencv_core.countNonZero
import org.bytedeco.opencv.global.opencv_core.extractChannel
import org.bytedeco.opencv.global.opencv_core.normalize
import org.bytedeco.opencv.global.opencv_core.subtract
import org.bytedeco.opencv.global.opencv_imgcodecs.IMREAD_GRAYSCALE
import org.bytedeco.opencv.global.opencv_imgcodecs.imread
import org.bytedeco.opencv.global.opencv_imgcodecs.imwrite
import org.bytedeco.opencv.global.opencv_imgproc.CV_THRESH_BINARY
import org.bytedeco.opencv.global.opencv_imgproc.calcHist
import org.bytedeco.opencv.global.opencv_imgproc.threshold
import org.bytedeco.opencv.opencv_core.Mat
import org.bytedeco.opencv.opencv_core.UMat
import org.jetbrains.compose.resources.painterResource
import org.jetbrains.compose.ui.tooling.preview.Preview
import kotlin.math.max
import kotlin.math.min

const val BASEPATH = "C:\\Users\\askolds\\Downloads\\DroniOtsu"

val done = mutableStateOf(false)

// Modified from thresh.cpp
// static double getThreshVal_Otsu_8u(const Mat& _src)
// https://stackoverflow.com/questions/33041900/opencv-threshold-with-mask/33069436#33069436
fun otsu_8u_with_mask(src: UMat, mask: UMat): Double {
    val N: Int = 256
    val h: IntArray = IntArray(N)

    val srcMat = src.getMat(ACCESS_READ)
    val maskMat = mask.getMat(ACCESS_READ)
    val hist = Mat(1, N, CV_32S)

    calcHist(
        /* images = */ srcMat,
        /* nimages = */ 1,
        /* channels = */ intArrayOf(0),
        /* mask = */ maskMat,
        /* hist = */ hist,
        /* dims = */ 1,
        /* histSize = */ intArrayOf(256),
        /* ranges = */ floatArrayOf(0f, 256f),
        /* uniform = */ true,
        /* accumulate = */ false
    )

    srcMat.release()
    maskMat.release()

    val M = countNonZero(mask)
    val hist2 = Mat()
    hist.convertTo(hist2, CV_32S)
    // Access the matrix data as a buffer (IntBuffer)
    val histData = hist2.data()  // BytePointer to the underlying buffer

    // Since the data is of type CV_32S, we need to treat it as an IntArray
    for (i in 0 until N) {
        h[i] = histData.getInt(i.toLong() * 4) // Each CV_32S element is 4 bytes
    }
    println(h.contentToString())

    hist.release()
    hist2.release()

    var mu: Double = 0.0
    val scale: Double = 1.toDouble() / (M)
    for (i in 0 until N) {
        mu += i * h[i].toDouble()
    }


    mu *= scale
    var mu1: Double = 0.0
    var q1: Double = 0.0
    var max_sigma: Double = 0.0
    var max_val: Double = 0.0

    for (i in 0 until N) {
        var p_i: Double
        var q2: Double
        var mu2: Double
        var sigma: Double

        p_i = h[i] * scale
        mu1 *= q1
        q1 += p_i
        q2 = 1.0 - q1

        if (min(q1, q2) < FLT_EPSILON || max(q1, q2) > 1.0 - FLT_EPSILON)  {
            continue
        }

        mu1 = (mu1 + i*p_i) / q1
        mu2 = (mu - q1*mu1) / q2
        sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2)
        if (sigma > max_sigma)
        {
            max_sigma = sigma
            max_val = i.toDouble()
        }
    }

    return max_val
}

fun stuff(imageMat: Mat, maskMat: Mat, n: Int, name: String): Pair<Mat, Mat> {
    println("Stuff $n")

    val mask = UMat(maskMat.size(), maskMat.type())
    maskMat.copyTo(mask)
    println("Mask Copied to UMat")

    val image = UMat(imageMat.size(), imageMat.type())
    imageMat.copyTo(image, mask)
    println("Mask applied, copied to UMat")

    println("Channels count: ${image.channels()}")

    val red = UMat(imageMat.size(), imageMat.type())
    val green = UMat(imageMat.size(), imageMat.type())
    val blue = UMat(imageMat.size(), imageMat.type())
    extractChannel(image, red, 2)
    extractChannel(image, green, 1)
    extractChannel(image, blue, 0)
    println("Channels extracted")

    val redScaled = UMat(red.size(), CV_32F)
    val greenScaled = UMat(green.size(), CV_32F)
    val blueScaled = UMat(blue.size(), CV_32F)

    println("Converting channels")

    green.convertTo(greenScaled, CV_32F, 2.0, 0.0) // greenScaled = G * 1.262; 2 for ExG
    red.convertTo(redScaled, CV_32F, 1.0, 0.0)     // redScaled = R * 0.884; 1 for ExG
    blue.convertTo(blueScaled, CV_32F, 1.0, 0.0)   // blueScaled = B * 0.311; 1 for ExG

    println("Channels scaled")

    red.release()
    green.release()
    blue.release()
    println("Channels released")

    // ExG index
    val exg = UMat(imageMat.size(), CV_32F)
    subtract(greenScaled, redScaled, exg)
    subtract(exg, blueScaled, exg)

    println("Index calculated")

    redScaled.release()
    greenScaled.release()
    blueScaled.release()
    println("Scaled released")

    // Normalize ExG to 0-255 (optional)
    val exgNormalized = UMat(imageMat.size(), CV_32F)
    normalize(exg, exgNormalized, 0.0, 255.0, NORM_MINMAX, -1, mask)
    println("Normalized")

    exg.release()
    println("exg released")

    val exgNormalized_8U = UMat(imageMat.size(), CV_8U)
    exgNormalized.convertTo(exgNormalized_8U, CV_8U)
    exgNormalized_8U.copyTo(exgNormalized_8U, mask)
    println("Converted to 8U")

    exgNormalized.release()
    println("exgNormalized released")


    imwrite(
        "$BASEPATH\\${name}_grayscale_${n}.jpg",
        exgNormalized_8U
    )
    println("Wrote grayscale")

    val otsuThreshold = UMat(imageMat.size(), CV_8U)
    val customOtsu = otsu_8u_with_mask(exgNormalized_8U, mask)
    val otsuThresh = threshold(
        exgNormalized_8U,
        otsuThreshold,
        customOtsu,
        255.0,
        CV_THRESH_BINARY
    )
    println("Builtin otsu: $otsuThresh    custom: $customOtsu")
    println("Threshold")

    mask.release()
    println("Mask released")

    exgNormalized_8U.release()
    println("exgNormalized_8U released")

    imwrite("$BASEPATH\\${name}_otsu_${n}.jpg", otsuThreshold)
    println("Wrote otsu")

    val maskedImage = UMat(imageMat.size(), imageMat.type())
    image.copyTo(maskedImage, otsuThreshold)
    println("Otsu mask applied")

    image.release()
    println("Image released")

    imwrite("$BASEPATH\\${name}_masked_${n}.jpg", maskedImage)
    println("Wrote masked")

    val maskedImageMat = Mat(maskedImage.size(), maskedImage.type())
    maskedImage.copyTo(maskedImageMat)
    val otsuThresholdMat = Mat(otsuThreshold.size(), otsuThreshold.type())
    otsuThreshold.copyTo(otsuThresholdMat)


    maskedImage.release()
    otsuThreshold.release()
    println("maskedImage, otsuThreshold released")

    return Pair(maskedImageMat, otsuThresholdMat)
}

fun doStuff() {
    CoroutineScope(Dispatchers.Default).launch {
        done.value = false
        val name = "011_A_45m_Contour"
        val image = imread("$BASEPATH\\${name}.jpg")
        val mask = imread("$BASEPATH\\011_A_45m_Contour_field_mask.jpg", IMREAD_GRAYSCALE)

        if (image != null && mask != null) {
            val pass1 = stuff(image, mask, 1, name)
            val pass2 = stuff(pass1.first, pass1.second, 2, name)
            val pass3 = stuff(pass2.first, pass2.second, 3, name)
            val pass4 = stuff(pass3.first, pass3.second, 4, name)
            val pass5 = stuff(pass4.first, pass4.second, 5, name)
            val pass6 = stuff(pass5.first, pass5.second, 6, name)
            image.release()
            mask.release()
            pass1.first.release(); pass1.second.release()
            pass2.first.release(); pass2.second.release()
            pass3.first.release(); pass3.second.release()
            pass4.first.release(); pass4.second.release()
            pass5.first.release(); pass5.second.release()
            pass6.first.release(); pass6.second.release()
        }
        done.value = true
    }
}

@Composable
@Preview
fun App() {
    MaterialTheme {
        val done by remember { done }
        var showContent by remember { mutableStateOf(false) }
        val ports = remember { SerialPortList.getPortNames() }
        ports.forEach { println(it) }
        LazyColumn(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
            items(ports) {
                Text(it)
            }

            if (done) {
                item {
                    Text("Done")
                }
            }

            item {
                Button(onClick = { doStuff() }) {
                    Text("opencv")
                }
            }

            item {
                Button(onClick = { showContent = !showContent }) {
                    Text("Click me!")
                }
            }
            item {
                AnimatedVisibility(showContent) {
                    val greeting = remember { Greeting().greet() }
                    Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                        Image(painterResource(Res.drawable.compose_multiplatform), null)
                        Text("Compose: $greeting")
                    }
                }
            }

            item {
                Box(modifier = Modifier.fillMaxSize().padding(12.dp).background(Color.Red))
            }
        }
    }
}