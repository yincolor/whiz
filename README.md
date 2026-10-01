# whiz

whiz 是一个基于 [saucer](https://github.com/saucer/saucer) C++ 库的**轻量级 Web 桌面应用运行时**。用户只需准备一个普通的 Web 项目目录（含 `package.json` 与入口 HTML），执行一条命令即可将其变为原生桌面应用：

```bash
whiz <网页项目目录>
```

不打包 Chromium、不内置 Node.js，直接复用系统原生 WebView（WebKitGTK / WKWebView / WebView2），体积小、启动快。

## 构建

### 环境要求

- CMake ≥ 3.25
- 支持 C++23 的编译器：项目使用了 `std::print` / `std::println`，需要 **GCC 14+**（如 `g++-14`）或对应的 Clang/libc++ 版本
- 构建时需要联网（通过 `FetchContent` 自动下载 `nlohmann/json`、`saucer`、`saucer-desktop` 及其传递依赖）

### Linux 前置依赖

Ubuntu 24.04 示例：

```bash
sudo apt install -y build-essential cmake git pkg-config g++-14 \
  libgtk-4-dev libadwaita-1-dev libwebkitgtk-6.0-dev
```

> 说明：
> - saucer 的 WebKitGTK 后端强依赖 `libadwaita-1-dev`；
> - 若系统默认 `g++` 低于 GCC 14，请显式指定 `g++-14`（见下方编译命令）。

### 编译

方式一：直接构建（显式指定编译器）：

```bash
CXX=g++-14 cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

方式二：使用项目自带的 CMakePresets：

```bash
CXX=g++-14 cmake --preset default
cmake --build --preset default
```

### 构建并运行单元测试

```bash
CXX=g++-14 cmake --preset tests
cmake --build --preset tests
ctest --test-dir build --output-on-failure
```

### 运行

```bash
./build/whiz examples/hello
```

### 安装（可选）

```bash
cmake --install build --prefix /usr/local
```

安装后可直接使用 `whiz <项目目录>`。

## 使用

```bash
whiz [选项] <项目目录>
```

省略 `<项目目录>` 时默认使用当前工作目录 `.`。常用选项：

| 选项 | 说明 |
|------|------|
| `-h, --help` | 打印帮助 |
| `-v, --version` | 打印版本 |
| `-d, --devtools` | 打开开发者工具 |
| `--url <url>` | 直接加载 URL |
| `--title <text>` | 覆盖窗口标题 |
| `--width <n>` / `--height <n>` | 覆盖窗口尺寸 |
| `--no-web-security` | 解除跨域限制 |
| `--verbose` | 调试日志 |

### 创建新项目（`whiz init`）

参考 `npm init`，whiz 提供交互式初始化命令：

```bash
mkdir my-app && cd my-app
whiz init
```

命令会逐项询问 `package.json` 的配置（`name`、`version`、`description`、`license`、入口 HTML、`webSecurity` 与 `whiz` 扩展配置等），括号中为默认值，直接回车即采用默认值；窗口配置直接采用内置默认值，不再逐项询问；最后确认后写入 `package.json`。

```bash
whiz init -y   # 跳过询问，全部使用默认值
```

## `package.json` 规范

```json
{
  "name": "my-app",
  "version": "0.3.1",
  "main": "index.html",
  "webSecurity": false,
  "window": { "title": "My App", "width": 1280, "height": 800 },
  "whiz": { "devTools": true, "backgroundColor": "#1e1e1e", "expose": ["dialog"] }
}
```

其中 `name` 为**必填**字段，且需满足：

- 非空字符串；
- 不含 `/`、`\`，且不能是 `.` 或 `..`；
- 不能使用保留名 `whiz-app`（与 `--url` 直连模式的默认名冲突）。

不满足任一条件时，whiz 会直接报错退出（退出码 `3`）。

详细字段见用户手册 `whiz使用手册.md`，开发者视角见 `whiz 项目文档.md`。

## 数据存储位置

whiz 会把 WebView 的持久化数据（Cookie 等）统一存放到用户数据目录，**不会在项目目录中生成 `.saucer` 文件**：

- Linux / macOS：`$HOME/.local/share/whiz/<name>/`
- Windows：`%USERPROFILE%\.local\share\whiz\<name>\`

其中 `<name>` 取自 `package.json` 的 `name` 字段；`--url` 直连模式没有 `package.json`，使用默认名 `whiz-app`。

## `window.whiz` 命名空间

whiz 在页面加载前注入 `window.whiz`，向 Web 前端暴露原生能力：

- `window.whiz.version` / `window.whiz.platform`
- `window.whiz.app`：应用级控制（quit / getPath）
- `window.whiz.window`：窗口控制
- `window.whiz.dialog`：文件对话框

示例：

```js
const res = await window.whiz.dialog.showOpenDialog({});
if (!res.canceled) console.log(res.filePaths);
```

## 目录结构

```
whiz/
├── CMakeLists.txt
├── CMakePresets.json
├── include/whiz/          # cli / init / manifest / window / paths / logger
├── src/                   # 实现 + main.cpp
├── scripts/               # 注入页面的 window.whiz 桥接脚本
├── examples/hello/        # 示例项目
└── tests/                 # 单元测试
```

## 许可证

GNU Affero General Public License v3.0
