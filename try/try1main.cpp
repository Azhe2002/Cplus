/**
 * main.cpp — 调用 libtry1.so 的程序
 *
 * 编译时需要 try1.h，链接时需要 libtry1.so
 * 但不需要 try1_lib.cpp 的源码！
 */

#include <iostream>
#include "try1.h"

int main() {
    std::cout << "=== 调用 libtry1.so ===" << std::endl << std::endl;

    struct {
        double a, b, c;
        const char* desc;
    } tests[] = {
        { 1, -3, 2, "x^2 - 3x + 2 = 0" },
        { 1, -2, 1, "x^2 - 2x + 1 = 0" },
        { 1,  0, 1, "x^2 + 1 = 0" },
        { 0,  2, -4,"退化: 2x - 4 = 0" },
    };

    for (const auto& t : tests) {
        int n = countRealRoots(t.a, t.b, t.c);
        std::cout << "  countRealRoots(" << t.a << ", " << t.b << ", " << t.c
                  << ") = " << n << "  ← " << t.desc << std::endl;
    }

    std::cout << std::endl << "✅ .so 调用成功！" << std::endl;
    return 0;
}
