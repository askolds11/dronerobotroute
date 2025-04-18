#include "../jni/org_askolds_cmptest_opencv_OpenCVStuff.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>

using namespace cv;
using namespace std;

void processImage(const cv::UMat& input, int stripHeight, int whiteThreshold, double displayScale = 0.05) {

    // input.copyTo(output);

    const int cols = input.cols;
    const int rows = input.rows;

    // Create output image (black background)
    cv::UMat output(rows, cols, CV_8UC1, cv::Scalar(0));

    for (int y = 0; y < rows; y += stripHeight) {
        const int currentStripHeight = std::min(stripHeight, rows - y);
        UMat strip = input(Range(y, y + currentStripHeight), Range(0, cols));
        //         cv::circle(output, cv::Point(col, currentStripHeight / 2), 1, cv::Scalar(0, 0, 255), -1);

        UMat histogram;
        // Count (white) pixels in columns
        cv::reduce(strip, histogram, 0, REDUCE_SUM, CV_64FC1);

        // Convert to count
        UMat histogram2;
        divide(histogram, 255, histogram2, 1, CV_16UC1);

        // Threshold the count to binary mask: 255 if > whiteThreshold, else 0
        cv::UMat mask8u;
        cv::threshold(histogram2, mask8u, whiteThreshold, 255, cv::THRESH_BINARY);
        mask8u.convertTo(mask8u, CV_8UC1); // Ensure 8-bit

        // Set the top row of this strip in the output image using the mask
        // mask8u.copyTo(output(cv::Range(y, y + 1), cv::Range::all())); // Write one row directly

        int noiseSize = 5;     // Remove small isolated white dots (opening)
        int gapSize = 20;       // Fill small black gaps between white pixels (closing)

        // Structuring elements for horizontal 1-row strips
        cv::Mat openKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noiseSize, 1));
        cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(gapSize + 1, 1));

        cv::UMat denoisedMask, cleanedMask;

        // Step 1: Opening — remove small noise
        cv::morphologyEx(mask8u, denoisedMask, cv::MORPH_OPEN, openKernel);

        // Step 2: Closing — fill small gaps
        cv::morphologyEx(denoisedMask, cleanedMask, cv::MORPH_CLOSE, closeKernel);

        // cleanedMask.copyTo(output(cv::Range(y, y + 1), cv::Range::all())); // Write one row directly

        // cleanedMask is 1 row high, and output is the same size as cleanedMask, but it's multi-row
        // cv::UMat outputMask = cv::UMat::zeros(output.size(), CV_8UC1);  // Initialize output mask

        cleanedMask.copyTo(output(cv::Range(y, y + 1), cv::Range::all())); // Write one row directly



        // Debug view
        // cv::UMat output2;
        // cv::resize(strip, output2, cv::Size(), displayScale, displayScale, cv::INTER_AREA);
        // cv::resize(histogram2, output2, cv::Size(), displayScale, 1, cv::INTER_AREA);

        // cv::imshow("Visualization", output2);
        // cv::waitKey(0);
        // printf("Next\n");
    }

    imwrite(R"(C:\Users\askolds\Downloads\DroniOtsu\3_FIND_LINES.jpg)", output);

    // Step 2: Run connected components with stats
    cv::Mat labels, stats, centroids;
    int nLabels = cv::connectedComponentsWithStats(output, labels, stats, centroids, 8, CV_32S);

    // Step 3: Create new image with white dots at centroids
    cv::Mat centerDotsMat = cv::Mat::zeros(output.rows, output.cols, CV_8UC1);

    // Loop through each label (skip label 0 = background)
    // for (int i = 1; i < nLabels; ++i) {
    //     int cx = static_cast<int>(centroids.getMat(cv::ACCESS_READ).at<double>(i, 0));
    //     int cy = static_cast<int>(centroids.getMat(cv::ACCESS_READ).at<double>(i, 1));
    //
    //     if (cx >= 0 && cx < centerDotsMat.cols && cy >= 0 && cy < centerDotsMat.rows) {
    //         centerDotsMat.at<uchar>(cy, cx) = 255;
    //     }
    // }
    // Step 1: Create a black 3-channel image to draw on (or use your original color image if available)
    cv::Mat colorOutput(output.size(), CV_8UC3, cv::Scalar(0, 0, 0));  // black background

    auto original = cv::imread(R"(C:\Users\askolds\Downloads\DroniOtsu\011_A_45m_Contour.jpg)", cv::IMREAD_COLOR);

    for (int i = 1; i < nLabels; ++i) {
        int cx = static_cast<int>(centroids.at<double>(i, 0));
        int cy = static_cast<int>(centroids.at<double>(i, 1));

        if (cx >= 0 && cx < original.cols && cy >= 0 && cy < original.rows) {
            cv::circle(original, cv::Point(cx, cy), 20, cv::Scalar(0, 0, 255), -1); // red (BGR)
        }
    }

    // Optional: Enlarge dots
    // cv::Mat dilated;
    // cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    // cv::dilate(centerDotsMat, dilated, kernel);

    // Convert result back to UMat
    // cv::UMat centerDots;
    // dilated.copyTo(centerDots);

    imwrite(R"(C:\Users\askolds\Downloads\DroniOtsu\3_FIND.jpg)", original);
    // Resize for display
    // cv::UMat resizedOutput;
    // cv::resize(output, resizedOutput, cv::Size(), displayScale, displayScale, cv::INTER_AREA);
    // cv::imshow("Visualization", resizedOutput);
    // cv::waitKey(0);

    // Resize for viewing
    // cv::UMat resizedOutput;
    // cv::resize(output, resizedOutput, cv::Size(), displayScale, displayScale, cv::INTER_AREA);
    //
    // cv::imshow("Visualization", resizedOutput);
    // cv::waitKey(0);
}

int main() {
    // Load image into UMat directly for GPU-acceleration
    cv::UMat img;
    cv::imread(R"(C:\Users\askolds\Downloads\DroniOtsu\woOutlier\011_A_45m_Contour_otsu_5.jpg)", cv::IMREAD_GRAYSCALE).copyTo(img);
    if (img.empty()) {
        std::cerr << "Failed to load image\n";
        return -1;
    }

    int stripHeight = 200;               // height of each horizontal strip
    int threshold = 5; //stripHeight / 2;     // white pixel count threshold

    processImage(img, stripHeight, threshold);

    return 0;
}

// R"(C:\Users\askolds\Downloads\DroniOtsu\2_Otsu_4.jpg)"
// R"(C:\Users\askolds\Downloads\DroniOtsu\2_Otsu_4_TESTING.jpg)"

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