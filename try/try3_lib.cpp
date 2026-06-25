/**
 * try3_lib.cpp — findSphereCenter 的实现
 *
 * 这个文件编译进 libtry3.so，使用者不可见
 */

#include "try3.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <opencv2/opencv.hpp>

// ---------- 颜色定义 (内部) ----------
struct ColorDef {
    std::string name;
    int         hLow, hHigh;
};

static const ColorDef COLORS[] = {
    {"Red",    0,  15},
    {"Green",  35, 85},
    {"Blue",   100, 130},
    {"Yellow", 20, 35},
};

static const ColorDef* findColor(const std::string& name) {
    for (const auto& c : COLORS)
        if (c.name == name) return &c;
    return nullptr;
}

static cv::Mat makeMask(const cv::Mat& hsv, const ColorDef& color) {
    cv::Mat m1, m2;
    cv::inRange(hsv, cv::Scalar(color.hLow, 60, 20),
                      cv::Scalar(color.hHigh, 255, 255), m1);
    if (color.name == "Red") {
        cv::inRange(hsv, cv::Scalar(170, 60, 20),
                          cv::Scalar(180, 255, 255), m2);
        return m1 | m2;
    }
    return m1;
}

// ---------- 对外接口 ----------
int findSphereCenter(const char* imagePath,
                     const char* colorName,
                     int* outX, int* outY) {

    *outX = *outY = -1;

    // ① 查颜色
    const ColorDef* color = findColor(colorName);
    if (!color) return -2;

    // ② 读图
    cv::Mat src = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (src.empty()) return -1;

    // ③ HSV → mask
    cv::Mat hsv;
    cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask = makeMask(hsv, *color);

    // ④ 形态学
    cv::Mat kClose = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::Mat kOpen  = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kClose);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kOpen);

    // ⑤ 找轮廓 → 取最大的有效轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) return -1;

    std::sort(contours.begin(), contours.end(),
              [](const auto& a, const auto& b) {
                  return cv::contourArea(a) > cv::contourArea(b);
              });

    int imgArea = src.cols * src.rows;
    const std::vector<cv::Point>* best = nullptr;
    double bestArea = 0;
    for (auto& c : contours) {
        double a = cv::contourArea(c);
        if (a >= 200 && a < imgArea * 0.40) {
            best = &c;
            bestArea = a;
            break;
        }
    }
    if (!best) return -1;

    // ⑥ 矩求质心
    cv::Moments m = cv::moments(*best);
    if (m.m00 < 1.0) return -1;
    *outX = static_cast<int>(m.m10 / m.m00);
    *outY = static_cast<int>(m.m01 / m.m00);

    std::cout << "  [libtry3.so] " << colorName
              << " 面积=" << (int)bestArea
              << "px  中心=(" << *outX << "," << *outY << ")" << std::endl;

    return 0;
}
