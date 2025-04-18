#include "../jni/org_askolds_cmptest_opencv_OpenCVStuff.h"
#include <opencv2/opencv.hpp>

using namespace cv;

const char *ColorIndexx(
    const char *src,
    const char *mask,
    const char *workDir,
    const double blue,
    const double green,
    const double red,
    const int nr
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
    const auto srcMat = imread(src, IMREAD_COLOR);
    const auto maskMat = imread(mask, IMREAD_GRAYSCALE);

    // Get umats
    // auto srcUMat = srcMat.getUMat(ACCESS_RW);
    // auto maskUMat = maskMat.getUMat(ACCESS_RW);
    // auto resultUMat = UMat(srcUMat.size(), CV_32FC1);
    // auto tempUMat = UMat(srcUMat.size(), CV_32FC1);

    printf("Running color index\n");

    UMat srcUMat;
    srcMat.convertTo(srcUMat, CV_32FC3);
    UMat maskUMat;
    maskMat.copyTo(maskUMat);
    auto resultUMat = UMat(srcUMat.size(), CV_32FC1);
    auto tempUMat = UMat(srcUMat.size(), CV_32FC1);

    //Apply to channels
    transform(srcUMat, resultUMat, Matx13f(
                  static_cast<float>(blue),
                  static_cast<float>(green),
                  static_cast<float>(red)
              )
    );

    // Normalize (within the mask)
    normalize(resultUMat, tempUMat, 0, 255, NORM_MINMAX, -1, maskUMat);

    // Clear resultUMat
    resultUMat.setTo(Scalar::all(0));
    // Convert to CV_8UC1
    tempUMat.convertTo(resultUMat, CV_8UC1);

    imwrite(std::format("{}\\{}_ColorIndex.{}", workDir, nr, extension), resultUMat);

    return "Test";
}

JNIEXPORT jstring JNICALL Java_org_askolds_cmptest_opencv_OpenCVStuff_colorIndex
(JNIEnv *env, jobject obj, jstring src, jstring mask, jstring workDir, jdouble red, jdouble green, jdouble blue) {
    return env->NewStringUTF("ColorIndex");
}
