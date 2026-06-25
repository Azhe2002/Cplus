/**
 * try3.h — libtry3.so 的公开头文件
 *
 * 2输入 → 1输出: 图片 + 颜色名 → 球体中心坐标
 */

#ifndef TRY3_H
#define TRY3_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 在图片中定位指定颜色的球体中心
 *
 * @param imagePath  输入1: 图片路径
 * @param colorName  输入2: 颜色名 ("Red" / "Green" / "Blue" / "Yellow")
 * @param outX       输出:  球心 X 坐标 (像素)，未找到时为 -1
 * @param outY       输出:  球心 Y 坐标 (像素)，未找到时为 -1
 * @return           0 成功, -1 未找到, -2 颜色名无效
 */
int findSphereCenter(const char* imagePath,
                     const char* colorName,
                     int* outX, int* outY);

#ifdef __cplusplus
}
#endif

#endif // TRY3_H
