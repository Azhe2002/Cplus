/**
 * try1_lib.cpp — countRealRoots 的实现
 *
 * 这个文件将被编译进 libtry1.so
 * 使用者看不到这个文件，只知道 try1.h 里的接口
 */

#include "try1.h"
#include <cmath>

// 这就是 .so 内部的实现，对外不可见
int countRealRoots(double a, double b, double c) {
    if (std::abs(a) < 1e-12) {
        return (std::abs(b) > 1e-12) ? 1 : 0;
    }

    double disc = b * b - 4.0 * a * c;

    if (disc > 1e-12)  return 2;
    if (disc > -1e-12) return 1;
    return 0;
}
