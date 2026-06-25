/**
 * try1.cpp — 最简单的 "3输入 → 1输出" 示例
 *
 * 功能：给定二次方程 ax² + bx + c = 0 的三个系数
 *       判断有多少个实数解（0 / 1 / 2）
 *
 * 这直接关联到光线追踪里的射线-球体相交判别式 (b² - 4ac)
 * 也是我们后续封装成 .so 的起点
 */

#include <iostream>
#include <cmath>

// ========== 核心函数：3输入 → 1输出 ==========
//
//  输入: a, b, c  (二次方程 ax² + bx + c = 0 的系数)
//  输出: 实数解的个数 (0, 1, 或 2)
//
int countRealRoots(double a, double b, double c) {
    // 退化情况: 不是二次方程
    if (std::abs(a) < 1e-12) {
        // bx + c = 0 → x = -c/b  (当 b != 0 时有 1 个解)
        return (std::abs(b) > 1e-12) ? 1 : 0;
    }

    double discriminant = b * b - 4.0 * a * c;

    if (discriminant > 1e-12)  return 2;   // 两个不同实根
    if (discriminant > -1e-12) return 1;   // 一个重根
    return 0;                               // 无实根
}

// ========== 测试 ==========
int main() {
    std::cout << "=== 3输入→1输出 演示 ===" << std::endl;

    struct TestCase {
        double a, b, c;
        const char* desc;
    };

    TestCase tests[] = {
        { 1, -3,  2, "x^2 - 3x + 2 = 0  (两个交点)"    },
        { 1, -2,  1, "x^2 - 2x + 1 = 0  (相切，一个根)" },
        { 1,  0,  1, "x^2 + 1 = 0      (无实根)"       },
        { 1, -5,  6, "x^2 - 5x + 6 = 0  (两个根: 2,3)" },
        { 0,  2, -4, "退化: 2x - 4 = 0  (一个根)"      },
    };

    for (const auto& t : tests) {
        int roots = countRealRoots(t.a, t.b, t.c);
        std::cout << "  " << t.desc << std::endl;
        std::cout << "    输入: a=" << t.a << " b=" << t.b << " c=" << t.c
                  << "  ->  输出: " << roots << " 个实根" << std::endl;
    }

    return 0;
}
