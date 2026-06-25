# x86_64 vs ARM64 架构知识

> 从 C++ 开发者的视角理解 CPU 架构差异

---

## 1. 指令集哲学：CISC vs RISC

```
x86_64 (CISC — 复杂指令集)          ARM64 (RISC — 精简指令集)

一条指令做很多事:                    一条指令只做一件事:
  add [rax], rbx                     ldr w0, [x1]
  (从内存读 → 加 → 写回，一步)         ldr w2, [x3]
                                      add w0, w0, w2
                                      str w0, [x1]
                                      (加载 → 加载 → 加 → 存储，四步)

优势: 指令少，代码体积小              优势: 每条指令定长4字节，解码快，省电
劣势: 指令变长(1-15字节)，解码复杂     劣势: 同样功能需要更多指令
```

你写 `a = b + c` → g++ 分别翻译成不同的机器码序列，但**完全不用关心**。

---

## 2. 寄存器：名字不同，干活一样

```
x86_64 (16个通用寄存器)              ARM64 (31个通用寄存器)

rax  rbx  rcx  rdx                 x0  x1  x2  ... x30
rsi  rdi  rbp  rsp                 
r8   r9   r10  r11                 更多寄存器 = 函数参数用寄存器传
r12  r13  r14  r15                 不用压栈，函数调用更快

特点: 少，部分有特殊历史用途         特点: 多，几乎全是通用，设计干净
      (rcx 自动用于循环计数)               (没有 x86 的历史包袱)
```

ARM 寄存器多是编译器的最爱——局部变量更多留在寄存器里，少访问内存，更快。

---

## 3. 内存模型：强序 vs 弱序

```
x86 (强内存序)                       ARM (弱内存序)

写操作对其他CPU立即可见              写操作可能延迟可见
线程A: x = 1; y = 2;               线程B 可能看到 y=2 但 x 还是旧值
线程B: 看到 x=1 则 y=2 一定成立      需要显式加 barrier / atomic
```

这就是为什么 C++ 有 `std::atomic` 和 `std::memory_order`——写一次代码，两种 CPU 都能正确跑：

```cpp
// 跨平台正确写法 (编译器会根据架构插入正确的屏障指令)
std::atomic<bool> ready{false};
int data = 42;
ready.store(true, std::memory_order_release);  // ARM 上会加 DMB 指令
```

---

## 4. 性能特征对比

| | x86_64 (你的电脑) | ARM64 (RK3588) |
|------|------|------|
| 典型场景 | 服务器/台式机/笔记本 | 手机/嵌入式/边缘AI |
| 功耗 | 15-300W | 1-15W |
| 主频 | 3-5 GHz | 1.5-2.5 GHz |
| 核心设计 | 8-32 个同构大核 | big.LITTLE (4大+4小) |
| 向量指令 | SSE4 / AVX2 / AVX-512 | NEON |
| 分支预测 | 激进 (误判代价高) | 保守 (误判代价低) |

同一段代码在两种 CPU 上都正确运行，但性能特征完全不同：

- **x86**：靠暴力——高频、复杂指令、激进预测，单线程极快
- **ARM**：靠不浪费——更多寄存器、简单解码、低功耗，多线程并行优势大

---

## 5. 数据模型（相同点）

这些两项完全一致，代码不需要修改：

| 类型 | x86_64 | ARM64 |
|------|------|------|
| `sizeof(void*)` | 8 | 8 |
| `sizeof(int)` | 4 | 4 |
| `sizeof(long)` | 8 | 8 |
| `sizeof(long long)` | 8 | 8 |
| 字节序 | 小端 (little-endian) | 小端 |
| `double` 格式 | IEEE 754 | IEEE 754 |

---

## 6. 数据模型（不同点）⚠️

| | x86_64 (g++) | ARM64 (g++) |
|------|------|------|
| `char` 有无符号 | **signed** (默认 -128~127) | **unsigned** (默认 0~255) |
| `wchar_t` 大小 | 4 字节 | 4 字节 (通常) |

### `char` 符号差异是最常见的交叉编译 bug

```cpp
// 这段代码在 x86 和 ARM 上行为不同！
char c = 0xFF;
if (c >> 4 == 0x0F) {  // x86: 算术右移 → true
    ...                 // ARM: 逻辑右移 → false (可能)
}

// 解决方案：用明确符号的类型
int8_t  c1 = 0xFF;  // 明确 signed
uint8_t c2 = 0xFF;  // 明确 unsigned
```

---

## 7. 为什么 ARM 省电

```
同一道计算: double a = sqrt(b*b + c*c);

x86_64:
  ├─ 解码: 变长指令，解码器复杂 → 功耗高
  ├─ 执行: 1 条融合乘加 + 1 条 sqrt (指令少)
  └─ 单核功耗: 15-30W

ARM64:
  ├─ 解码: 定长4字节，解码电路极简 → 功耗极低
  ├─ 执行: 3 条指令 mul + add + sqrt (指令多但每步简单)
  └─ 单核功耗: 1-5W

省在哪:
  ① 指令定长 → 解码电路简单 → 晶体管少 → 省电
  ② 更多寄存器 → 少访问内存 → 省电 (访存比计算费电得多)
  ③ 没有 30 年 x86 兼容包袱 → 设计干净 → 省电
  ④ big.LITTLE → 后台任务跑小核 (2W)，前台跑大核 (5W)
```

---

## 8. big.LITTLE 调度

ARM 芯片常见配置：4 个大核 (高性能) + 4 个小核 (高能效)

```
大小核调度示例:
  ├─ 待机/后台: 小核 A53/A55 @ 1.8GHz, ~1W
  ├─ 一般任务: 2-3 个大核 + 小核
  └─ 满负载:   全部大核 A76/A78 @ 2.4GHz, ~8W

Linux 内核自动调度，开发时一般不用管
但如果做实时推理，可以:
  taskset -c 0-3 ./llmdemo   # 只绑大核，避免推理中途切到小核
```

---

## 9. 向量指令对比

```
// 同一个向量加法: C[0..3] = A[0..3] + B[0..3]

x86_64 (AVX2, 256位, 一次 4 个 double):
  vmovupd ymm0, [rdi]    // 加载 4 个 double
  vaddpd  ymm0, ymm0, [rsi]
  vmovupd [rdx], ymm0

ARM64 (NEON, 128位, 一次 2 个 double):
  ldp q0, [x0]           // 加载 2 个 double
  fadd v0.2d, v0.2d, v1.2d
  stp q0, [x2]

x86 一次算 4 个，ARM 一次算 2 个
但 OpenCV / 编译器自动处理，你不必手写这些
```

---

## 10. 交叉编译速查

```
主机 (x86_64)                         目标 (ARM64)
─────────────────────────────────────────────────────
g++                     ←→     aarch64-linux-gnu-g++
nm                      ←→     aarch64-linux-gnu-nm
objdump                 ←→     aarch64-linux-gnu-objdump
file 命令查格式:                    file 命令查格式:
  ELF 64-bit x86-64                ELF 64-bit ARM aarch64

编译:
  aarch64-linux-gnu-g++           \
    -I./arm64_include              \   ARM64 头文件
    -L./arm64_lib                  /   ARM64 .so
    main.cpp -lrkllm -o llmdemo   /

验证:
  file llmdemo
  # 输出: llmdemo: ELF 64-bit LSB executable, ARM aarch64, ...
```

---

## 参考

- [ARM Architecture Reference Manual](https://developer.arm.com/documentation/ddi0487/latest/)
- [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler)
- [ARM NEON Programming Quick Reference](https://community.arm.com/arm-community-blogs/b/architectures-and-processors-blog/posts/coding-for-neon---part-1-load-and-stores)
