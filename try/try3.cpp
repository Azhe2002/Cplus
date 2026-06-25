/**
 * try3.cpp — 调用 OpenCV .so 的 "2输入→1输出" 示例
 *
 * 输入1: 图片路径
 * 输入2: 颜色名 (Red / Green / Blue / Yellow)
 * 输出:  该颜色球体的像素中心坐标 (x, y)，未找到返回 (-1, -1)
 *
 * 原理 (基于 try2 的分色检测):
 *   输入颜色 → HSV inRange → 形态学 → 最大轮廓 → 矩计算质心
 *
 * 编译: g++ try3.cpp -o try3 $(pkg-config --cflags --libs opencv4)
 * 运行: ./try3 <图片> <颜色>
 * 示例: ./try3 ../test.png Red
 */

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <opencv2/opencv.hpp>

// ---------- 颜色定义 ----------
struct ColorDef {
    std::string name;
    int         hLow, hHigh;   // HSV 色相范围 (OpenCV 0~180)
    cv::Scalar  bgr;            // 可视化颜色
};

const ColorDef COLORS[] = {
    {"Red",    0,  15,  cv::Scalar(0,   0, 255)},   // + 170~180
    {"Green",  35, 85,  cv::Scalar(0, 255,   0)},
    {"Blue",   100, 130, cv::Scalar(255, 0,   0)},
    {"Yellow", 20, 35,  cv::Scalar(0, 255, 255)},
};

/**
 * 根据颜色名查表，返回对应 ColorDef，未找到返回 nullptr
 */
const ColorDef* findColor(const std::string& name) {
    for (const auto& c : COLORS) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

/**
 * 为指定颜色生成 HSV mask
 * S>60 排除灰色背景，V>20 保留暗面
 */
cv::Mat makeMask(const cv::Mat& hsv, const ColorDef& color) {
    cv::Mat m1, m2;
    cv::inRange(hsv,
                cv::Scalar(color.hLow, 60, 20),
                cv::Scalar(color.hHigh, 255, 255), m1);
    // 红色在 HSV 中是两端 (0~15 和 170~180)
    if (color.name == "Red") {
        cv::inRange(hsv,
                    cv::Scalar(170, 60, 20),
                    cv::Scalar(180, 255, 255), m2);
        return m1 | m2;
    }
    return m1;
}

/**
 * 核心函数：2个输入 → 1个输出
 *
 * @param imagePath  输入1: 图片路径
 * @param colorName  输入2: 颜色名 (Red/Green/Blue/Yellow)
 * @param outX       输出: 球心 X 坐标 (像素)
 * @param outY       输出: 球心 Y 坐标 (像素)
 * @return           0 成功, -1 未找到, -2 颜色名无效
 */
int findSphereCenter(const std::string& imagePath,
                     const std::string& colorName,
                     int& outX, int& outY) {
    outX = outY = -1;

    // ① 查颜色
    const ColorDef* color = findColor(colorName);
    if (!color) {
        std::cerr << "未知颜色: " << colorName
                  << " (可选: Red Green Blue Yellow)" << std::endl;
        return -2;
    }

    // ② 读图
    cv::Mat src = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (src.empty()) {
        std::cerr << "无法读取图片: " << imagePath << std::endl;
        return -1;
    }

    // ③ BGR → HSV → 取 mask
    cv::Mat hsv;
    cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask = makeMask(hsv, *color);

    // ④ 形态学：闭运算填高光洞，开运算去噪
    cv::Mat kClose = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::Mat kOpen  = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kClose);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kOpen);

    // ⑤ 找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        std::cout << "  未找到 " << colorName << " 区域" << std::endl;
        return -1;
    }

    // 按面积排序，取最大的有效轮廓
    // 限制: 200px < area < 40%画面 (排除噪声和误检的大片背景)
    int imgArea = src.cols * src.rows;
    std::sort(contours.begin(), contours.end(),
              [](const auto& a, const auto& b) {
                  return cv::contourArea(a) > cv::contourArea(b);
              });

    int bestIdx = -1;
    double bestArea = 0;
    for (size_t i = 0; i < contours.size(); i++) {
        double a = cv::contourArea(contours[i]);
        if (a >= 200 && a < imgArea * 0.40) {
            bestIdx = i;
            bestArea = a;
            break;
        }
    }
    if (bestIdx < 0) {
        std::cout << "  " << colorName << " 无有效轮廓 (共" << contours.size() << "个)" << std::endl;
        return -1;
    }

    const auto& best = contours[bestIdx];
    double area = bestArea;

    // ⑥ 用图像矩计算质心 (比 minEnclosingCircle 更准确)
    cv::Moments m = cv::moments(best);
    if (m.m00 < 1.0) return -1;
    outX = static_cast<int>(m.m10 / m.m00);
    outY = static_cast<int>(m.m01 / m.m00);

    std::cout << "  检测到 " << colorName << " 球体: "
              << "面积=" << (int)area << "px"
              << "  中心=(" << outX << ", " << outY << ")" << std::endl;

    // ⑦ 可视化：画十字标记
    cv::Mat result = src.clone();
    cv::drawMarker(result, cv::Point(outX, outY), color->bgr,
                   cv::MARKER_CROSS, 30, 2);
    cv::circle(result, cv::Point(outX, outY), 5, color->bgr, -1);
    cv::putText(result, color->name + " (" + std::to_string(outX) + "," + std::to_string(outY) + ")",
                cv::Point(outX + 15, outY - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, color->bgr, 2);
    std::string outputPath = "try3_" + color->name + ".jpg";
    cv::imwrite(outputPath, result);
    std::cout << "  可视化: " << outputPath << std::endl;

    return 0;
}

// ========== 测试 ==========
int main(int argc, char* argv[]) {
    std::cout << "=== 2输入→1输出: 球体定位 ===" << std::endl << std::endl;

    const char* imagePath = (argc > 1) ? argv[1] : "../test.png";
    const char* colorName = (argc > 2) ? argv[2] : "Red";

    if (argc < 3) {
        std::cout << "  用法: ./try3 <图片路径> <颜色>" << std::endl;
        std::cout << "  颜色: Red Green Blue Yellow" << std::endl;
        std::cout << "  默认: " << imagePath << " " << colorName << std::endl << std::endl;
    }

    int x, y;
    int ret = findSphereCenter(imagePath, colorName, x, y);

    if (ret == 0) {
        std::cout << "\n  → 输出: (" << x << ", " << y << ")" << std::endl;
    } else {
        std::cout << "\n  → 输出: 未找到" << std::endl;
    }

    return ret;
}
