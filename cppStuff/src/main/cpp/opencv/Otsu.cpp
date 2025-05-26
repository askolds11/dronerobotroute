#include "../jni/org_askolds_cmptest_opencv_OpenCVStuff.h"
#include <opencv2/opencv.hpp>
#include "headers/Otsu.h"

using namespace std;
using namespace cv;

/**
 * Aprēķina threshold vērtību ar otsu metodi maskas apgabalos
 * @param src Attēls
 * @param mask Maska
 * @return Otsu threshold vērtība
 */
double otsu_8u_with_mask(const Mat &src, const Mat &mask) {
    int i;
    constexpr int N = 256;
    // int h[N] = {};

    auto hist = Mat(1, N, CV_32S);

    constexpr int histSize = 256;
    float range[] = {0, 256}; //the upper boundary is exclusive
    const float *histRange[] = {range};

    calcHist(
        /* images = */ &src,
                       /* nimages = */ 1,
                       /* channels = */ nullptr,
                       /* mask = */ mask,
                       /* hist = */ hist,
                       /* dims = */ 1,
                       /* histSize = */ &histSize,
                       /* ranges = */ histRange,
                       /* uniform = */ true,
                       /* accumulate = */ false
    );

    const int M = countNonZero(mask);

    double mu = 0;
    const double scale = 1. / (M);
    // for (i = 0; i < N; i++)
    //     mu += i*(double)h[i];
    for (i = 0; i < N; i++)
        mu += i * hist.at<float>(i, 0);

    mu *= scale;
    double mu1 = 0, q1 = 0;
    double max_sigma = 0, max_val = 0;

    for (i = 0; i < N; i++) {
        double q2;

        const double p_i = hist.at<float>(i, 0) * scale;
        mu1 *= q1;
        q1 += p_i;
        q2 = 1. - q1;

        if (std::min(q1, q2) < FLT_EPSILON || std::max(q1, q2) > 1. - FLT_EPSILON)
            continue;

        mu1 = (mu1 + i * p_i) / q1;
        double mu2 = (mu - q1 * mu1) / q2;
        double sigma = q1 * q2 * (mu1 - mu2) * (mu1 - mu2);
        if (sigma > max_sigma) {
            max_sigma = sigma;
            max_val = i;
        }
    }
    return max_val;
}

/**
 * Apply otsu to specified UMats.
 * Set maskUMat to resulting Otsu threshold
 * @param srcUMat Source UMat
 * @param maskUMat Mask UMat
 * @param tempUMat Temporary UMat
 */
void OtsuApply(
    UMat &srcUMat,
    UMat &maskUMat,
    UMat &tempUMat
) {
    // Clear temp
    tempUMat.setTo(Scalar::all(0));

    // Normalizējam
    normalize(srcUMat, tempUMat, 0, 255, NORM_MINMAX, -1, maskUMat);
    srcUMat.setTo(Scalar::all(0));
    tempUMat.copyTo(srcUMat);

    // Clear temp
    tempUMat.setTo(Scalar::all(0));

    // Aprēķinām otsu threshold
    const auto calculatedOtsuThreshold = otsu_8u_with_mask(srcUMat.getMat(ACCESS_READ), maskUMat.getMat(ACCESS_READ));
    // printf("calculatedOtsuThreshold: %f\n", calculatedOtsuThreshold);

    // Otsu threshold rezultāts tempUMat
    threshold(
        srcUMat,
        tempUMat,
        calculatedOtsuThreshold,
        255,
        THRESH_BINARY
    );

    // Clear mask
    maskUMat.setTo(Scalar::all(0));
    tempUMat.copyTo(maskUMat);
}

const string Otsuu(
    const char *src,
    const char *mask,
    const char *workDir,
    const int nr,
    const int repeat
) {
    // Get extension
    const auto srcString = std::string(src);
    std::string extension;
    if (const std::size_t indexLastSeparator = srcString.find_last_of('.'); indexLastSeparator != std::string::npos) {
        extension = srcString.substr(indexLastSeparator + 1);
    } else {
        throw "Couldn't find extension";
    }

    // Read images
    const auto srcMat = imread(src, IMREAD_GRAYSCALE);
    const auto maskMat = imread(mask, IMREAD_GRAYSCALE);

    // Get umats
    auto srcUMat = srcMat.getUMat(ACCESS_RW);
    auto maskUMat = maskMat.getUMat(ACCESS_RW);
    auto tempUMat = UMat(srcUMat.size(), CV_8UC1);

    // Clear areas that are not in mask
    srcMat.copyTo(tempUMat, maskUMat);
    srcUMat.setTo(Scalar::all(0));
    tempUMat.copyTo(srcUMat);

    // Finally do Otsu
    for (int i = 0; i < repeat; i++) {
        // Possible improvement: Apply color index to previous iteration's mask, and normalize for more resolution
        printf("Running otsu No. %d\n", i + 1);
        OtsuApply(srcUMat, maskUMat, tempUMat);
        imwrite(std::format("{}/{}_Otsu_{}.{}", workDir, nr, i + 1, extension), maskUMat);
    }

    auto outputFile = std::format("{}/{}_Otsu_{}.{}", workDir, nr, repeat, extension);
    return outputFile;
}

JNIEXPORT jstring JNICALL Java_org_askolds_cmptest_opencv_OpenCVStuff_otsu
(JNIEnv *env, jobject obj, jstring src, jstring mask, jstring workDir, jint nr, jint repeat) {
    // Get C++ strings (char*)
    const char *srcString = env->GetStringUTFChars(src, nullptr);
    const char *maskString = env->GetStringUTFChars(mask, nullptr);
    const char *workDirString = env->GetStringUTFChars(workDir, nullptr);

    // Call
    Otsuu(
        srcString,
        maskString,
        workDirString,
        nr,
        repeat
    );

    // Release things
    env->ReleaseStringUTFChars(src, srcString);
    env->ReleaseStringUTFChars(mask, maskString);
    env->ReleaseStringUTFChars(workDir, workDirString);

    // Return
    return env->NewStringUTF("Otsu");
}
