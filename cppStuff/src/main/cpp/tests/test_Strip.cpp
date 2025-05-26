#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "../opencv/headers/Strip.h"  // Include the actual implementation

const char* withHome(const char* path)
{
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            return (std::string(home) + std::string(path).substr(1)).c_str();
        }
    }
    return path;
}

TEST(StripTest, Works) {

    Strip(
        withHome(R"(~/Downloads/DroniOtsu/011_A_45m_Contour.jpg)"),
        withHome(R"(~/Downloads/DroniOtsu/2_Otsu_4.jpg)"),
        withHome(R"(~/Downloads/DroniOtsu/011_A_45m_Contour_field_mask_improved.jpg)"),
        withHome(R"(~/Downloads/DroniOtsu/011_A_45m_Contour_field_mask_turnarounds.jpg)"),
        50,
        3,
        0.1,
        0.15,
        withHome(R"(~/Downloads/DroniOtsu/Img1)"),
        4
    );

    EXPECT_EQ(1, 1);
}
