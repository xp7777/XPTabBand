# MinHook 库

本目录用于存放 [MinHook](https://github.com/TsudaKageyu/minhook) 的源码。

MinHook 是一个用于 x64/x86 的 API Hooking 库，XPTabHook 项目依赖它来实现
Windows API 函数拦截。

## 安装步骤

由于无法在创建项目时自动下载，请手动获取 MinHook 源码：

1. 访问 MinHook 官方仓库：https://github.com/TsudaKageyu/minhook
2. 下载源码（克隆或下载 ZIP）
3. 将以下文件复制到本目录（`MinHook\`），使所有源码文件平铺在同一层：

   | 源文件路径（仓库内）              | 目标路径              |
   |-----------------------------------|-----------------------|
   | `include/MinHook.h`               | `MinHook/MinHook.h`   |
   | `src/buffer.c`                    | `MinHook/buffer.c`    |
   | `src/buffer.h`                    | `MinHook/buffer.h`    |
   | `src/hook.c`                      | `MinHook/hook.c`      |
   | `src/MinHook.c`                   | `MinHook/MinHook.c`   |
   | `src/HDE/hde64.c`                 | `MinHook/hde64.c`     |
   | `src/HDE/hde64.h`                 | `MinHook/hde64.h`     |

4. 用下载的 `MinHook.h` **替换**当前的占位文件（占位文件含 `#error`，
   替换后才能编译）。

## 编译说明

- MinHook 的 `.c` 文件会通过 `XPTabHook.vcxproj` 中的通配符
  `$(SolutionDir)MinHook\*.c` 自动加入编译，**无需手动添加到项目**。
- 这些 `.c` 文件以 C 语言方式编译（`CompileAsC`），且不使用预编译头。

## 验证安装

安装完成后，本目录应包含以下文件：

```
MinHook/
├── MinHook.h      （从 include/ 复制，替换占位文件）
├── MinHook.c
├── buffer.c
├── buffer.h
├── hook.c
├── hde64.c
├── hde64.h
└── README.md      （本文件）
```

运行 `build.bat` 时会自动检查 `hook.c` 是否存在，未安装会给出提示。
