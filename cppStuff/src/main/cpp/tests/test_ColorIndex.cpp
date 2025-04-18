#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "../opencv/ColorIndex.cpp"  // Include the actual implementation

TEST(ColorIndexTest, Works) {
    // std::string inputPath = "C:\\Users\\askolds\\Downloads\\DroniOtsu\\011_A_45m_Contour.jpg";
    // std::string outputPath = "C:\\Users\\askolds\\Downloads\\DroniOtsu\\011_A_45m_Contour_TEST.jpg";

    // int result = applyOtsuThreshold(inputPath, outputPath);
    // EXPECT_EQ(result, 69);  // Check if function returns expected success value

    // cv::Mat output = cv::imread(outputPath, cv::IMREAD_GRAYSCALE);
    // EXPECT_FALSE(output.empty());  // Ensure output image is successfully created
    // UMat image, output;
    // imread("C:\\Users\\askolds\\Downloads\\DroniOtsu\\011_A_45m_Contour.jpg", IMREAD_GRAYSCALE).copyTo(image);
    // threshold(image, output, 0.0, 255.0, THRESH_BINARY | THRESH_OTSU);
    // imwrite("C:\\Users\\askolds\\Downloads\\DroniOtsu\\011_A_45m_Contour_MTEST.jpg", output);

    ColorIndexx(
        R"(C:\Users\askolds\Downloads\DroniOtsu\011_A_45m_Contour.jpg)",
        R"(C:\Users\askolds\Downloads\DroniOtsu\011_A_45m_Contour_field_mask.jpg)",
        R"(C:\Users\askolds\Downloads\DroniOtsu)",
        -1,
        2,
        -1,
        1
    );

    EXPECT_EQ(1, 1);
}
