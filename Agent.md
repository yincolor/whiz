# Agent.md

本文件面向 AI 编码助手。阅读后应能独立完成对 **whiz** 项目的理解、修改与验证。

## 项目定位

whiz 是基于 [saucer](https://github.com/saucer/saucer) C++ 库的轻量 Web 桌面运行时。
用户执行 `whiz <项目目录>`，whiz 读取该目录的 `package.json`，解析入口 HTML，创建原生窗口加载页面，并通过 `window.whiz` 向页面暴露原生能力。

不提供 Node 运行时，不做资源打包，不打包 Chromium——直接复用系统 WebView。

## 技术栈

- C++20 + CMake ≥ 3.20
- saucer v6.0.0（核心窗口/WebView 抽象）
- saucer/desktop v4.0.0（对话框等桌面能力）
- nlohmann/json v3.11.3（解析 `package.json` 与桥接消息）
- 依赖统一通过 `FetchContent` 引入，不手工安装

## 构建与运行

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/whiz examples/hello
```

Linux 前置依赖：`libgtk-4-dev libwebkitgtk-6.0-dev libglib2.0-dev`。

测试（可选）：`cmake -B build -DWHIZ_BUILD_TESTS=ON && ctest --test-dir build`。

## 目录职责

- `include/whiz/` — 头文件：`cli` / `init` / `manifest` / `window` / `paths` / `logger`
- `src/` — 与头文件对应的实现 + `main.cpp`
- `scripts/` — 注入页面的 JS 桥接脚本（定义 `window.whiz`）
- `examples/` — 可直接运行的示例项目
- `tests/` — 单元测试
- `third_party/` — 由 CMake 拉取，勿手改

## 架构分层（自顶向下）

1. **CLI 层**：解析参数，处理 `whiz init` 子命令，定位项目目录，处理覆盖项
2. **manifest 层**：解析 `package.json`，产出 `Manifest` / `WindowConfig`
3. **窗口构建层**：创建 `saucer::smartview`，应用配置，加载页面，注入 `window.whiz`，`ready` 后显示
4. **saucer**：跨平台 WebView

跨平台差异只能出现在窗口构建层，不得外溢到 CLI / manifest 层。

## 关键约定

- **命令行优先级 > `package.json`**。任何新增配置项都应支持 CLI 覆盖。
- **所有原生能力统一挂在 `window.whiz.*` 下**（`app` / `window` / `dialog` / `tray` / `menu` / `clipboard`），不得向 `window` 顶层添加字段。
- `webSecurity`（默认 `true`）语义对齐 Electron：设为 `false` 表示解除同源策略，等价 CLI 参数 `--no-web-security`。
- `main` 必须是 HTML 入口；若为 `.js` 直接报错退出。
- 桥接调用统一携带自增 `requestId`，前后端一一配对；数据以 JSON 字符串传递。
- 页面调用为 Promise；拒绝时抛出 `Error`。

## 退出码（勿随意改动）

`0` 正常 / `1` 参数错 / `2` 目录不可读 / `3` manifest 解析失败 / `4` 找不到入口 / `5` saucer 初始化失败。

## 代码风格

- C++20，`namespace whiz`
- 头文件 `#pragma once`；include 顺序：本模块 → 第三方 → 标准库
- 类型 `snake_case`，函数 `snake_case`，常量 `UPPER_SNAKE`，结构体字段 `snake_case`
- 禁止裸 `new` / `delete`，用智能指针与 RAII
- 日志经 `logger`，勿直接 `printf` / `std::cout`

## 修改检查清单

- [ ] CLI 新选项有 `--help` 文案与退出码约定
- [ ] `package.json` 新字段有默认值、类型校验、示例更新
- [ ] 新增 `window.whiz.*` 能力已同步 `whiz.expose` 白名单逻辑
- [ ] 跨平台分支只出现在窗口构建层
- [ ] 未引入 Node 依赖或运行时假设
- [ ] `examples/hello` 仍能一键跑通

## 常见坑

- `file://` 下 `fetch()` 受同源限制——需用户显式 `webSecurity: false`
- WebView2 Runtime 在部分 Windows 上未预装
- 无边框 / 透明窗口并非所有平台都支持，需降级并记日志
- saucer API 名称随版本可能微调，平台调用集中在窗口构建层便于升级
