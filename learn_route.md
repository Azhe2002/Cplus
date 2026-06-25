# .so 共享库开发学习路线

> 目标：把 C++ 代码封装成 `.so` 动态库，暴露稳定的输入/输出接口

---

## 第一阶段：概念地基（约 1-2 天）

| 概念 | 要点 | 当前项目对应 |
|------|------|-------------|
| **静态库 vs 动态库** | `.a` 编译时嵌入 vs `.so` 运行时加载；体积、更新、内存共享的权衡 | 5 个 `.cpp` 各自 `main()`，天然适合拆成 lib + 前端 |
| **符号可见性** | `__attribute__((visibility("default"/"hidden")))` + `-fvisibility=hidden` | Vec3/Sphere/Ray 这些 struct 需要标记哪些导出 |
| **ABI 稳定性** | C++ name mangling 导致不同编译器/g++ 版本不兼容 | `Vec3::dot()` 在 g++ 15 vs g++ 13 下符号名不同 |
| **`extern "C"`** | 导出纯 C 接口 = 跨语言/跨编译器兼容 | `void render_scene(int w, int h, float* out)` 这种接口 |

### 关键命令速查

```bash
# 查看 .o 里的符号（C++ mangled names）
nm foo.o

# 查看 .so 导出了哪些符号
nm -D libfoo.so

# 把 mangled name 还原成可读的 C++ 签名
nm foo.o | c++filt
```

---

## 第二阶段：编译技术（约 1 天）

| 技术点 | 关键命令 | 说明 |
|--------|---------|------|
| **PIC（位置无关代码）** | `g++ -fPIC -c foo.cpp -o foo.o` | `.so` 需要在任意内存地址运行，必须加 `-fPIC` |
| **链接成 .so** | `g++ -shared -o libfoo.so foo.o bar.o` | 把 `.o` 打包成动态库 |
| **链接使用 .so** | `g++ -L. -lfoo main.cpp -o app` | `-L` 指定路径，`-lfoo` 链接 `libfoo.so` |
| **运行时查找** | `LD_LIBRARY_PATH=. ./app` 或 `-Wl,-rpath,'$ORIGIN'` | 运行时找不到 .so 的解决方案 |
| **查看依赖** | `ldd ./app` | 列出 app 依赖的所有 .so |

### 最小示例

```bash
# 1. 编译成 .so
g++ -fPIC -c test.cpp -o test.o
g++ -shared -o libtest.so test.o

# 2. 写一个 main.cpp 调用 libtest.so
# 3. 编译并链接
g++ -L. -ltest main.cpp -o app

# 4. 运行
LD_LIBRARY_PATH=. ./app
```

---

## 第三阶段：接口设计（约 2-3 天，最核心）

| 模式 | 说明 | 示例 |
|------|------|------|
| **C 风格不透明句柄** | `void*` 指针 + 函数操作，头文件不暴露实现 | `RtScene* s = rt_create(); rt_render(s, buf); rt_destroy(s);` |
| **PIMPL 模式** | C++ 头文件只放指针，实现全藏 .cpp 里 | `class Foo { struct Impl; Impl* p; public: void bar(); };` |
| **输入输出契约** | 谁分配内存、谁释放、buffer 大小怎么算 | 用户传 `float* buf = malloc(w*h*3)` 还是库内部分配？ |
| **错误处理** | C++ 异常不能跨 .so 边界！ | 用 `enum RtError` + `const char* rt_last_error()` 代替 `throw` |
| **回调注册** | 让用户注入行为（进度回调、日志等） | `rt_set_progress_callback(scene, myFn, userData)` |

### 典型 C 接口设计模板

```cpp
// raytracer.h  (纯 C 头文件)
#ifdef __cplusplus
extern "C" {
#endif

// 不透明句柄
typedef struct RtContext RtContext;

// 生命周期
RtContext* rt_create(void);
void       rt_destroy(RtContext* ctx);

// 场景设置（输入接口）
void rt_set_resolution(RtContext* ctx, int w, int h);
void rt_add_sphere(RtContext* ctx, float cx, float cy, float cz, float r,
                   float r_col, float g_col, float b_col, float reflectivity);

// 渲染（输出接口）
// 调用者负责分配 buf，大小 = w * h * 3 (RGB float)
void rt_render(const RtContext* ctx, float* out_buf);

// 导出点云（输出接口）
// 返回点数，调用者负责分配 pts_buf (每点 6 float: xyz+rgb) 和 colors_buf
int  rt_export_pointcloud(const RtContext* ctx, float* pts_buf, float* colors_buf);

// 错误处理
int         rt_get_error(const RtContext* ctx);
const char* rt_get_error_string(const RtContext* ctx);

#ifdef __cplusplus
}
#endif
```

---

## 第四阶段：工程化（约 1 天）

| 技术 | 用途 |
|------|------|
| **CMake `add_library(foo SHARED ...)`** | 取代裸 g++ 命令，跨平台 |
| **`target_include_directories` + `PUBLIC` vs `PRIVATE`** | 公开 API 头文件用 `PUBLIC`，内部用 `PRIVATE` |
| **SONAME 版本管理** | `libraytracer.so → libraytracer.so.1 → libraytracer.so.1.0.0` |
| **`pkg-config` / `.pc` 文件** | 让使用者 `pkg-config --cflags --libs raytracer` 一键获取编译选项 |
| **安装规则 `install(TARGETS ...)`** | `make install` 把 .so 和 .h 复制到 `/usr/local` |

### CMakeLists.txt 模板

```cmake
cmake_minimum_required(VERSION 3.16)
project(raytracer_lib VERSION 1.0.0 LANGUAGES CXX)

# 动态库目标
add_library(raytracer SHARED
    src/raytracer.cpp
    src/vec3.cpp
)

# 公开头文件路径
target_include_directories(raytracer
    PUBLIC  include/          # 使用者可见
    PRIVATE src/              # 仅内部可见
)

# 隐藏内部符号，只导出标记过的
set_target_properties(raytracer PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# 版本
set_target_properties(raytracer PROPERTIES
    VERSION  ${PROJECT_VERSION}        # 1.0.0
    SOVERSION 1                        # .so.1
)

install(TARGETS raytracer DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/raytracer)
```

---

## 第五阶段：你的项目实战 — 目标架构

```
当前 (5个独立可执行文件)              目标 (lib + 前端)

pointcloud.cpp  ─┐                   ┌─────────────────────┐
raytracer.cpp   ─┤── 重复的:        │  libraytracer.so    │
project_pc.cpp  ─┤   Vec3, Camera,  │  ├─ Vec3 数学       │
draw_boxes.cpp  ─┤   Sphere, 投影   │  ├─ 光线追踪引擎     │
                ─┘                   │  ├─ 场景管理        │
                                     │  └─ PLY 导出       │
                                     └──────┬──────────────┘
                                            │ 调用
                                     ┌──────┴──────────────┐
                                     │  各前端可执行文件     │
                                     │  ├─ raytrace_app    │
                                     │  ├─ project_app     │
                                     │  └─ drawboxes_app   │
                                     └─────────────────────┘
```

### 库的输入输出契约

```
libraytracer.so
  输入: 场景参数 (球体列表、相机位置/FOV、光源方向、分辨率)
  输出: 像素 RGB buffer + PLY 点云数组 + 精度报告
```

---

## 推荐动手顺序

1. **`test.cpp` 最小实验** — 一个函数 → .so → 另一个程序调用（体会 `-fPIC -shared LD_LIBRARY_PATH`）
2. **提取 Vec3** — 写 `vec3.h/vec3.cpp`，做成 `libvec3.so`，`extern "C"` 导出几个运算
3. **封装 Raytracer** — 把 pointcloud.cpp 的渲染核心拆成 lib，设计 C 接口
4. **CMake 重构** — 用 CMake 管理整个项目
5. **跨语言验证** — 用 Python `ctypes` 加载你的 .so 并调用，验证接口稳定性

---

## 常见踩坑

| 坑 | 现象 | 解决 |
|----|------|------|
| 忘了 `-fPIC` | `relocation R_X86_64_32 against ... can not be used when making a shared object` | 编译 `.o` 时加 `-fPIC` |
| C++ 异常跨边界 | 程序直接 abort | 所有公开函数内部 try-catch，转成错误码返回 |
| STL 容器跨边界 | 一方 new，另一方 delete → 堆损坏 | 不暴露 `std::vector`，用 C 数组 + 大小参数 |
| `LD_LIBRARY_PATH` 忘设 | `error while loading shared libraries: libxxx.so` | `export LD_LIBRARY_PATH=.` 或 `-Wl,-rpath` |
| 符号冲突 | 两个 .so 各有一个全局 `WIDTH` | `-fvisibility=hidden` 默认隐藏，手动标记导出 |

---

## 参考资源

- [GCC Visibility](https://gcc.gnu.org/wiki/Visibility)
- [Ulrich Drepper: How To Write Shared Libraries](https://akkadia.org/drepper/dsohowto.pdf) (经典长文)
- [CMake: Creating a shared library](https://cmake.org/cmake/help/latest/command/add_library.html)
