/**
 * try2.cpp — 调用 OpenCV .so 的 "1输入→1输出" 示例
 *
 * 功能: 输入一张图片 → 输出图中检测到的彩色球体数量
 *
 * 原理 (纯检测，不依赖场景先验):
 *   HSV 分色提取 → 每颜色独立形态学 → 轮廓检测 → 面积+圆形度筛选 → 计数
 *
 *   球体饱和度 S≈200, 背景(棋盘格/影子/天空) S≈0。
 *   S>60 一刀切断所有灰色/黑色/低饱和反光，只留高饱和彩色区域。
 *   每个颜色独立处理，避免重叠球体被合并。
 *
 * 编译: g++ try2.cpp -o try2 $(pkg-config --cflags --libs opencv4)
 * 运行: ./try2 <图片路径>
 * 示例: ./try2 ../test.png
 */

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <opencv2/opencv.hpp>

/**
 * 核心函数：1个输入 → 1个输出
 */
int countObjects(const std::string& imagePath) {
    // ① 读取图片
    cv::Mat src = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (src.empty()) {
        std::cerr << "无法读取图片: " << imagePath << std::endl;
        return -1;
    }
    int imgArea = src.cols * src.rows;
    std::cout << "  输入: " << imagePath
              << " (" << src.cols << "x" << src.rows << ")" << std::endl;

    // ② BGR → HSV
    cv::Mat hsv;
    cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);

    // ③ 四个颜色各自提取 mask
    //    H 范围 (OpenCV: 0~180), S>60 (球体~200, 背景~0), V>20 (保留暗面)
    cv::Mat mr1, mr2;
    cv::inRange(hsv, cv::Scalar(0,   60, 20), cv::Scalar(15,  255, 255), mr1);
    cv::inRange(hsv, cv::Scalar(170, 60, 20), cv::Scalar(180, 255, 255), mr2);
    cv::Mat maskRed = mr1 | mr2;

    cv::Mat maskGreen;
    cv::inRange(hsv, cv::Scalar(35,  60, 20), cv::Scalar(85,  255, 255), maskGreen);

    cv::Mat maskBlue;
    cv::inRange(hsv, cv::Scalar(100, 60, 20), cv::Scalar(130, 255, 255), maskBlue);

    cv::Mat maskYellow;
    cv::inRange(hsv, cv::Scalar(20,  60, 20), cv::Scalar(35,  255, 255), maskYellow);

    // ④ 调试：保存每个颜色的 mask
    cv::imwrite("try2_mask_red.jpg",    maskRed);
    cv::imwrite("try2_mask_green.jpg",  maskGreen);
    cv::imwrite("try2_mask_blue.jpg",   maskBlue);
    cv::imwrite("try2_mask_yellow.jpg", maskYellow);

    // ⑤ 每个颜色独立处理
    struct ColorInfo {
        std::string name;
        cv::Mat    mask;
        cv::Scalar bgr;
    };
    ColorInfo colors[] = {
        {"Red",    maskRed,    cv::Scalar(0,   0, 255)},
        {"Green",  maskGreen,  cv::Scalar(0, 255,   0)},
        {"Blue",   maskBlue,   cv::Scalar(255, 0,   0)},
        {"Yellow", maskYellow, cv::Scalar(0, 255, 255)},
    };

    cv::Mat kernel  = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::Mat kernel2 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));

    int totalCount = 0;
    cv::Mat result = src.clone();

    for (auto& c : colors) {
        cv::morphologyEx(c.mask, c.mask, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(c.mask, c.mask, cv::MORPH_OPEN, kernel2);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(c.mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::cout << "  [" << c.name << "] 轮廓数=" << contours.size() << std::endl;

        for (size_t i = 0; i < contours.size(); i++) {
            double area = cv::contourArea(contours[i]);
            double perim = cv::arcLength(contours[i], true);
            double circularity = (perim > 0)
                ? (4.0 * M_PI * area / (perim * perim)) : 0;

            // 打印所有轮廓信息，方便调试
            const char* status = "";
            if (area < 200)          status = " → 太小";
            else if (area > imgArea * 0.40) status = " → 太大";
            else if (circularity < 0.5 && area < 20000)  status = " → 圆形度低";
            else if (circularity < 0.15)                  status = " → 圆形度过低";
            else                             status = " ✅";
            std::cout << "    #" << i << " 面积=" << (int)area
                      << "  圆形度=" << circularity << status << std::endl;

            // 面积过滤
            if (area < 200) continue;
            if (area > imgArea * 0.40) continue;
            // 大物体(>20000px)宽进：被遮挡后圆形度会很低但面积大，仍是真实球体
            // 小物体严进：必须是近圆形
            if (area >= 20000) {
                if (circularity < 0.15) continue;  // 大物体最低圆形度
            } else {
                if (circularity < 0.5) continue;   // 小物体必须近圆
            }

            totalCount++;
            cv::drawContours(result, contours, i, c.bgr, 2);
            cv::Moments m = cv::moments(contours[i]);
            if (m.m00 > 0) {
                int cx = static_cast<int>(m.m10 / m.m00);
                int cy = static_cast<int>(m.m01 / m.m00);
                cv::putText(result, c.name,
                            cv::Point(cx - 15, cy),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, c.bgr, 2);
            }
        }
    }

    std::cout << "  物体总数: " << totalCount << std::endl;

    std::string outputPath = "try2_result.jpg";
    cv::imwrite(outputPath, result);
    std::cout << "  可视化: " << outputPath << std::endl;

    return totalCount;
}

// ========== 测试 ==========
int main(int argc, char* argv[]) {
    std::cout << "=== 1输入→1输出: 图片物体计数 ===" << std::endl << std::endl;

    const char* imagePath = (argc > 1) ? argv[1] : "../test.png";

    if (argc < 2) {
        std::cout << "  用法: ./try2 <图片路径>" << std::endl;
        std::cout << "  默认使用: " << imagePath << std::endl << std::endl;
    }

    int n = countObjects(imagePath);
    std::cout << "\n  → 输出: " << n << " 个球体" << std::endl;

    return (n < 0) ? 1 : 0;
}
