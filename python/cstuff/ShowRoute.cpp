#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <opencv2/opencv.hpp>

using namespace cv;

Point parsePoint(const std::string& token) {
    // token format: "(x,y)"
    size_t comma = token.find(',');
    int x = std::stoi(token.substr(1, comma - 1));
    int y = std::stoi(token.substr(comma + 1, token.size() - comma - 2));
    return Point{x, y};
}

int main(int argc, char** argv )
{
    if ( argc != 3 )
    {
        printf("usage: ShowRoute.out <Image_Path> <Route_Path>\n");
        return -1;
    }

    Mat image = imread(argv[1], IMREAD_COLOR);

    if ( !image.data )
    {
        printf("No image data \n");
        return -1;
    }

    std::ifstream file(argv[2]);
    if (!file.is_open()) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string fromToken, toToken;

        ss >> fromToken >> toToken;

        Point from = parsePoint(fromToken);
        Point to = parsePoint(toToken);

        cv::arrowedLine(image, from, to, Scalar(0, 0, 255), 5, 8, 0, 0.05);
    }

    file.close();

    imwrite("output.jpg", image);

    return 0;
}
