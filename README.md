# YooBox Input Tester - SDL2 Demo

## 项目概述

这是一个基于SDL2的Hello World演示程序，展示基本的窗口管理、事件处理和渲染功能。

## 硬件规格

- **屏幕尺寸**: 4.5英寸
- **分辨率**: 1620×1080
- **PPI**: 约450 (计算公式: √(1620² + 1080²) / 4.5 ≈ 450)

## 功能需求

### 1. 窗口设置
- 窗口标题: "YooBox Input Tester"
- 窗口分辨率: 1620×1080
- 全屏或窗口模式可选

### 2. 界面布局
```
┌─────────────────────────────────────┐
│           [LOGO图片]                │  <- 顶部Logo区域
├─────────────────────────────────────┤
│         YooBox Input Tester             │  <- 标题区域
│                                     │
│         [按键显示区域]               │  <- 中央显示区域
│                                     │
└─────────────────────────────────────┘
```

### 3. 输入处理
支持以下输入方式:
- **键盘输入**: 显示按键名称 (如: A, B, Enter, Space等)
- **触摸输入**: 显示触摸坐标 (如: Touch(540, 360))
- **游戏手柄**: 显示按钮名称 (如: Joystick A, Start等)

### 4. 显示逻辑
- 每次输入后刷新显示新的按键值
- 不累加历史记录,只显示最新输入
- 居中显示按键信息

### 5. 视觉风格
采用赛博朋克(Cyberpunk)风格:
- 深色背景 (黑色/深蓝)
- 霓虹色文字 (青色/品红/黄色)
- 发光效果
- 科技感UI元素

## 技术栈

- **SDL2**: 核心图形库
- **SDL2_ttf**: 字体渲染

## 资源文件

```
res/
└── fonts/
    └── font.ttf        # 主字体文件
```

## 字体适配

根据4.5英寸屏幕和450PPI计算:
- 标题字体大小: 48px (物理尺寸约2.7mm)
- 按键显示字体: 36px (物理尺寸约2mm)
- 辅助文字: 24px (物理尺寸约1.3mm)

## 构建说明

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make

# 运行
./YooBoxInputTester
```

## Ubuntu 依赖安装指南

### 1. 更新系统包

```bash
sudo apt update && sudo apt upgrade -y
```

### 2. 安装编译工具链

```bash
sudo apt install -y build-essential cmake pkg-config
```

### 3. 安装 SDL2 核心库

```bash
sudo apt install -y libsdl2-dev
```

> 包含: SDL2 头文件、动态库 `libSDL2.so`、静态库

### 4. 安装 SDL2_ttf 字体渲染库

```bash
sudo apt install -y libsdl2-ttf-dev
```

> 包含: TTF 字体渲染支持，用于显示文字

### 5. 一键安装所有依赖

```bash
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libsdl2-ttf-dev
```

### 6. 验证安装

```bash
pkg-config --modversion sdl2
# 输出示例: 2.0.20

pkg-config --modversion SDL2_ttf
# 输出示例: 2.0.18
```

### 7. Wayland 支持（可选）

如果需要 Wayland 显示后端：

```bash
sudo apt install -y libwayland-dev libxkbcommon-dev
```

### 常见问题

| 问题 | 解决方案 |
|------|----------|
| `SDL_Init failed` | 检查是否安装完整，尝试 `export SDL_VIDEODRIVER=x11` |
| 字体不显示 | 确认 `res/fonts/font.ttf` 文件存在 |
| Wayland 不可用 | 程序会自动回退到 X11 |

## 交叉编译指南 (RK3326 aarch64)

### 工具链信息

- **目标平台**: RK3326 (ARM aarch64)
- **工具链路径**: `/home/ice/distribution/build.ROCKNIX-RK3326.aarch64/toolchain`
- **编译器前缀**: `aarch64-rocknix-linux-gnueabi-`
- **编译器版本**: GCC 14.2.0
- **Sysroot**: `toolchain/aarch64-rocknix-linux-gnueabi/sysroot`

### 工具链目录结构

```
/home/ice/distribution/build.ROCKNIX-RK3326.aarch64/toolchain/
├── bin/
│   ├── aarch64-rocknix-linux-gnueabi-gcc
│   ├── aarch64-rocknix-linux-gnueabi-g++
│   └── ...
├── aarch64-rocknix-linux-gnueabi/sysroot/
│   └── usr/
│       ├── include/
│       │   ├── SDL2/          # SDL2 头文件
│       │   └── ...
│       └── lib/
│           ├── libSDL2.so
│           ├── libSDL2_ttf.so
│           ├── libSDL2_image.so
│           └── ...
└── cmake/
```

### 交叉编译步骤

#### 1. 创建工具链文件

项目已包含 `toolchain.cmake`，内容如下：

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_DIR /home/ice/distribution/build.ROCKNIX-RK3326.aarch64/toolchain)
set(CROSS_PREFIX ${TOOLCHAIN_DIR}/bin/aarch64-rocknix-linux-gnueabi)
set(SYSROOT ${TOOLCHAIN_DIR}/aarch64-rocknix-linux-gnueabi/sysroot)

set(CMAKE_C_COMPILER ${CROSS_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${CROSS_PREFIX}-g++)
set(CMAKE_AR ${CROSS_PREFIX}-gcc-ar)
set(CMAKE_RANLIB ${CROSS_PREFIX}-gcc-ranlib)
set(CMAKE_STRIP ${CROSS_PREFIX}-strip)

set(CMAKE_SYSROOT ${SYSROOT})

set(CMAKE_FIND_ROOT_PATH ${SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

#### 2. 执行交叉编译

```bash
# 创建交叉编译目录
mkdir build_cross && cd build_cross

# 使用工具链文件配置
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..

# 编译
make -j$(nproc)

# 验证生成的二进制文件
file YooBoxInputTester
# 输出: ELF 64-bit LSB executable, ARM aarch64, ...
```

#### 3. 一键编译脚本

```bash
#!/bin/bash
# cross_build.sh

TOOLCHAIN_DIR=/home/ice/distribution/build.ROCKNIX-RK3326.aarch64/toolchain
BUILD_DIR=build_cross

rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake .. && make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "========================================="
    echo "交叉编译成功!"
    echo "二进制文件: ${BUILD_DIR}/YooBoxInputTester"
    file YooBoxInputTester
    echo "========================================="
else
    echo "交叉编译失败!"
    exit 1
fi
```

使用方法：
```bash
chmod +x cross_build.sh
./cross_build.sh
```

### 部署到目标设备

```bash
# 方法1: 使用 scp 传输
scp YooBoxInputTester user@device:/home/user/

# 方法2: 使用 adb 推送 (Android设备)
adb push YooBoxInputTester /data/local/tmp/

# 方法3: 挂载 NFS 共享目录
cp YooBoxInputTester /nfs_shared/
```

### 在目标设备运行

```bash
# SSH 登录设备后
chmod +x YooBoxInputTester
./YooBoxInputTester
```

### 交叉编译常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `SDL2 not found` | pkg-config 找到主机库 | CMakeLists.txt 已处理，交叉编译时跳过 pkg-config |
| `cannot find -lSDL2` | 库路径不正确 | 检查 `CMAKE_SYSROOT/usr/lib` 目录 |
| `undefined reference` | 链接顺序问题 | 确保 SDL2 在 SDL2_ttf 之前链接 |
| 运行时 `No such file` | 动态链接器路径问题 | 设置 `LD_LIBRARY_PATH=/usr/lib` 或使用静态链接 |

### 静态链接（可选）

如需生成静态链接的可执行文件（无需依赖目标设备的 .so 文件）：

```bash
# 需要工具链中有静态库 (.a 文件)
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake \
      -DCMAKE_EXE_LINKER_FLAGS="-static" ..
make -j$(nproc)
```

## macOS

在 macOS 上开发本程序的完整流程。注意:原生路径走 `pkg-config`(见 `CMakeLists.txt` 的非交叉分支),
所以需要机器上同时有 `sdl2`、`SDL2_ttf` 两个模块的 `.pc` 文件,缺一个 `cmake` 配置就会失败(它们是 `REQUIRED`)。

### 1. 安装前置工具

```sh
# Xcode 命令行工具(提供 clang / clang++)
xcode-select --install

# Homebrew(如未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# CMake、pkg-config
brew install cmake pkg-config
```

### 2. 安装 SDL2 依赖

```sh
brew install sdl2_ttf sdl2_image
```

> 注意:Homebrew 里的 `sdl2` 公式近年已由 `sdl2-compat` 取代并提供 `sdl2` 包名(pkg-config 里 Name 是 `sdl2_compat`)。
> 它的 `.pc` 会声明 `-lSDL2main` 却不提供这个库,因此 `CMakeLists.txt` 原生分支特意用
> `list(FILTER _SDL2_LIBS EXCLUDE REGEX "SDL2main")` 把它剔除。**不要删掉这行**,否则会链接失败。

验证依赖是否就位:

```sh
pkg-config --modversion sdl2 SDL2_ttf
```

三个都打印出版本号即可。

### 3. 配置、编译、运行

```sh
# 配置 + 编译(构建产物在 build/)
cmake -S . -B build && cmake --build build -j

# 运行 —— 必须从仓库根目录运行,资源是相对 CWD 加载的(res/fonts/font.ttf 等)
./build/YooBoxInputTester
```

### 4. 已知问题

- **链接告警**:`ld: warning: dylib (...libSDL2*.dylib) was built for newer macOS version (26.0) than being linked (14.0)`
  说明构建的部署目标(默认 14.0)低于 Homebrew 所装 dylib 的版本。功能不受影响,若想消除可在配置时指定更高目标:
  ```sh
  cmake -S . -B build -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0
  ```
- **图形后端**:程序启动时依次尝试 `wayland` → `cocoa` → `x11`(见 `main.cpp` 的 `SDL_setenv("SDL_VIDEODRIVER", ...)` 链)。
  macOS 上没有 wayland/x11,会自然落到 `cocoa`,正常打开窗口。
