# DB Manager

## 1. Project introduction / 项目介绍

- This is a database management tool written in C.
- 这是使用 C 编写的数据库管理工具。

- You can install, configure, initialize, start, or remove a database on your
  current Linux system, both online and offline.
- 通过这个工具，可以在联网或离线状态下在当前 Linux 系统中安装、配置、
  初始化、启动或移除数据库。

## 2. How to build / 如何构建

Requires CMake >= 3.10, a C11 compiler, and GNU Make.
需要 CMake >= 3.10、C11 编译器以及 GNU Make。

### 2.1 Debug build / 调试构建

```bash
make debug
```

Binary: `build/bin/debug/db-manager`
生成文件：`build/bin/debug/db-manager`

### 2.2 Release build / 发布构建

```bash
make release
```

Binary: `build/bin/release/db-manager`
生成文件：`build/bin/release/db-manager`

### 2.3 Install / 安装到系统

```bash
make install
```

### 2.4 Clean / 清除构建产物

```bash
make clean      # 删除 build/
make distclean  # 删除 build/ + compile_commands.json
```
