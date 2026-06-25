/**
 * try3main.cpp — 调用 libtry3.so 的程序
 *
 * 编译: g++ try3main.cpp -o try3main -L. -ltry3
 *       需要 libtry3.so 和 try3.h
 *       不需要 try3_lib.cpp 源码！
 */

#include <iostream>
#include "try3.h"

int main(int argc, char* argv[]) {
    std::cout << "=== 调用 libtry3.so (2输入→1输出) ===" << std::endl << std::endl;

    const char* imagePath = (argc > 1) ? argv[1] : "../test.png";
    const char* colorName = (argc > 2) ? argv[2] : "Red";

    if (argc < 3) {
        std::cout << "  用法: ./try3main <图片> <颜色>" << std::endl;
        std::cout << "  颜色: Red Green Blue Yellow" << std::endl;
        std::cout << "  默认: " << imagePath << " " << colorName << std::endl << std::endl;
    }

    int x, y;
    int ret = findSphereCenter(imagePath, colorName, &x, &y);

    if (ret == 0) {
        std::cout << "\n  → 输出: (" << x << ", " << y << ")" << std::endl;
    } else if (ret == -1) {
        std::cout << "\n  → 输出: 未找到" << std::endl;
    } else {
        std::cout << "\n  → 输出: 颜色名无效 (ret=" << ret << ")" << std::endl;
    }

    return ret;
}
