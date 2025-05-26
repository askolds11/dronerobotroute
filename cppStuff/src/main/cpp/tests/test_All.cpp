#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "../opencv/headers/ColorIndex.h"
#include "../opencv/headers/Otsu.h"
#include "../opencv/headers/Strip.h"

using namespace std;

std::string withHomeAll(const char* path)
{
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            return std::string(home) + std::string(path).substr(1);
        }
    }
    return std::string(path);
}

TEST(AllTest, Img1) {
    const auto workDirString = withHomeAll(R"(~/Downloads/DroniOtsu/Img1)");
    const auto workDir = workDirString.c_str();
    const string originalImg = string(workDir) + "/" + string("011_A_45m_Contour.jpg");

    const string bushMask = string(workDir) + "/" + string("011_A_45m_Contour_field_mask.jpg");
    const string mask = string(workDir) + "/" + string("011_A_45m_Contour_field_mask_improved.jpg");
    const string turnaroundMask = string(workDir) + "/" + string("011_A_45m_Contour_field_mask_turnarounds.jpg");

    cout << "Color index" << endl;
    const auto colorIndex = ColorIndexx(
        originalImg.c_str(),
        bushMask.c_str(),
        workDir,
        1.5,
        0.5,
        -2,
        1
    );
    cout << "Otsu" << endl;
    const auto otsu = Otsuu(
        colorIndex.c_str(),
        bushMask.c_str(),
        workDir,
        2,
        3
    );
    // const auto otsu = workDirString + "/2_Otsu_3.jpg";
    cout << "Strips" << endl;
    Strip(
        originalImg.c_str(),
        otsu.c_str(),
        mask.c_str(),
        turnaroundMask.c_str(),
        50,
        3,
        0.1,
        0.15,
        workDir,
        4
    );

    EXPECT_EQ(1, 1);
}

TEST(AllTest, Img1Exg) {
    const auto workDirString = withHomeAll(R"(~/Downloads/DroniOtsu/Img1Exg)");
    const auto workDir = workDirString.c_str();
    const string originalImg = string(workDir) + "/" + string("011_A_45m_Contour.jpg");

    const string bushMask = string(workDir) + "/" + string("011_A_45m_Contour_field_mask.jpg");
    const string mask = string(workDir) + "/" + string("011_A_45m_Contour_field_mask_improved.jpg");
    const string turnaroundMask = string(workDir) + "/" + string("011_A_45m_Contour_field_mask_turnarounds.jpg");

    cout << "Color index" << endl;
    const auto colorIndex = ColorIndexx(
        originalImg.c_str(),
        bushMask.c_str(),
        workDir,
        -1,
        2,
        -1,
        1
    );
    cout << "Otsu" << endl;
    const auto otsu = Otsuu(
        colorIndex.c_str(),
        bushMask.c_str(),
        workDir,
        2,
        4
    );
    // const auto otsu = workDirString + "/2_Otsu_4.jpg";
    cout << "Strips" << endl;
    Strip(
        originalImg.c_str(),
        otsu.c_str(),
        mask.c_str(),
        turnaroundMask.c_str(),
        50,
        3,
        0.1,
        0.15,
        workDir,
        4
    );

    EXPECT_EQ(1, 1);
}

TEST(AllTest, Img2) {
    const auto workDirString = withHomeAll(R"(~/Downloads/DroniOtsu/Img2)");
    const auto workDir = workDirString.c_str();
    const string originalImg = string(workDir) + "/" + string("Img2.jpg");

    const string bushMask = string(workDir) + "/" + string("bushMask.jpg");
    const string mask = string(workDir) + "/" + string("rowMask.jpg");
    const string turnaroundMask = string(workDir) + "/" + string("turnaroundMask.jpg");

    cout << "Color index" << endl;
    const auto colorIndex = ColorIndexx(
        originalImg.c_str(),
        bushMask.c_str(),
        workDir,
        1.5,
        0.5,
        -2,
        1
    );
    cout << "Otsu" << endl;
    const auto otsu = Otsuu(
        colorIndex.c_str(),
        bushMask.c_str(),
        workDir,
        2,
        3
    );
    // const auto otsu = workDirString + "/2_Otsu_3.jpg";
    cout << "Strips" << endl;
    Strip(
        originalImg.c_str(),
        otsu.c_str(),
        mask.c_str(),
        turnaroundMask.c_str(),
        50,
        5,
        0.12,
        0.18,
        workDir,
        4
    );

    EXPECT_EQ(1, 1);
}

TEST(AllTest, Img2Exg) {
    const auto workDirString = withHomeAll(R"(~/Downloads/DroniOtsu/Img2Exg)");
    const auto workDir = workDirString.c_str();
    const string originalImg = string(workDir) + "/" + string("Img2.jpg");

    const string bushMask = string(workDir) + "/" + string("bushMask.jpg");
    const string mask = string(workDir) + "/" + string("rowMask.jpg");
    const string turnaroundMask = string(workDir) + "/" + string("turnaroundMask.jpg");

    cout << "Color index" << endl;
    const auto colorIndex = ColorIndexx(
        originalImg.c_str(),
        bushMask.c_str(),
        workDir,
        -1,
        2,
        -1,
        1
    );
    cout << "Otsu" << endl;
    const auto otsu = Otsuu(
        colorIndex.c_str(),
        bushMask.c_str(),
        workDir,
        2,
        3
    );
    // const auto otsu = workDirString + "/2_Otsu_3.jpg";
    cout << "Strips" << endl;
    Strip(
        originalImg.c_str(),
        otsu.c_str(),
        mask.c_str(),
        turnaroundMask.c_str(),
        50,
        0.5,
        0.12,
        0.18,
        workDir,
        4
    );

    EXPECT_EQ(1, 1);
}
