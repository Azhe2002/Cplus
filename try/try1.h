/**
 * try1.h — 方案A: 1个接口，3个参数，1个返回值
 *
 * 这是 libtry1.so 的公开头文件
 * 使用者只需要 #include 这个头文件 + 链接 libtry1.so
 * 完全不需知道内部实现
 */

#ifndef TRY1_H
#define TRY1_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 计算二次方程 ax² + bx + c = 0 的实数解个数
 *
 * @param a  二次项系数
 * @param b  一次项系数
 * @param c  常数项
 * @return   实数解个数: 0 (无) / 1 (单根或退化) / 2 (两个)
 */
int countRealRoots(double a, double b, double c);

#ifdef __cplusplus
}
#endif

#endif // TRY1_H
