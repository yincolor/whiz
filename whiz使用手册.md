# whiz 使用手册

whiz 是一个基于 [saucer](https://github.com/saucer/saucer) 的**轻量级 Web 桌面应用运行时**：把一个普通的 Web 项目目录（`package.json` + HTML/CSS/JS）直接跑成原生桌面窗口，不内置 Chromium，也不内置 Node.js。

```bash
whiz <网页项目目录>
```

---

## 目录

1. [快速开始](#1-快速开始)
2. [安装与构建](#2-安装与构建)
3. [命令行使用](#3-命令行使用)
4. [初始化项目（`whiz init`）](#4-初始化项目whiz-init)
5. [`package.json` 配置](#5-packagejson-配置)
6. [`window.whiz` 前端 API](#6-windowwhiz-前端-api)
7. [平台差异](#7-平台差异)
8. [常见问题（FAQ）](#8-常见问题faq)
9. [附录：配置与 API 速查](#9-附录配置与-api-速查)

---

## 1. 快速开始

准备一个 Web 项目目录：

```
my-app/
├── package.json
├── index.html
├── style.css
└── app.js
```

`package.json`：

```json
{
  "name": "my-app",
  "version": "0.1.0",
  "main": "index.html",
  "window": { "title": "我的应用", "width": 1024, "height": 768 }
}
```

运行：

```bash
whiz ./my-app
```

窗口即会弹出并加载 `my-app/index.html`。

> 目标目录下**必须存在 `package.json`**，且 `name` 字段必须存在并合法，否则 whiz 会直接报错退出（退出码 `3`）。`name` 校验规则见 [5.1](#51-顶层字段)。

---

## 2. 安装与构建

### 2.1 环境要求

| 项目 | 要求 |
|------|------|
| CMake | ≥ 3.25 |
| 编译器 | 支持 C++23，推荐 **GCC 14+**（`g++-14`） |
| 网络 | 构建时通过 `FetchContent` 下载依赖，需要联网 |

> 项目使用了 `std::print` / `std::println`，因此需要 GCC 14 及以上；Ubuntu 24.04 默认的 `g++-13` 无法编译。

### 2.2 Linux 构建（Ubuntu 24.04）

安装依赖：

```bash
sudo apt install -y build-essential cmake git pkg-config g++-14 \
  libgtk-4-dev libadwaita-1-dev libwebkitgtk-6.0-dev
```

编译：

```bash
CXX=g++-14 cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

或使用项目自带的 CMakePresets：

```bash
CXX=g++-14 cmake --preset default
cmake --build --preset default
```

运行示例：

```bash
./build/whiz examples/hello
```

### 2.3 构建并运行单元测试

```bash
CXX=g++-14 cmake --preset tests
cmake --build --preset tests
ctest --test-dir build --output-on-failure
```

### 2.4 安装（可选）

```bash
cmake --install build --prefix /usr/local
whiz ./my-app
```

---

## 3. 命令行使用

### 3.1 基本用法

```text
whiz [选项] <项目目录>
whiz init [-y]
```

- `<项目目录>` 可以是**目录**、**`package.json` 文件路径**，也可以省略（默认使用当前目录 `.`）。
- 目录中必须存在 `package.json`，否则报错退出。
- 命令行选项优先级**高于** `package.json` 中的配置。

### 3.2 全部选项

| 选项 | 简写 | 说明 |
|------|------|------|
| `--help` | `-h` | 打印帮助并退出 |
| `--version` | `-v` | 打印版本号并退出 |
| `--devtools` | `-d` | 启动时打开开发者工具 |
| `--url <url>` | | 直接加载指定 URL，跳过 `package.json` 解析 |
| `--title <text>` | | 覆盖窗口标题 |
| `--width <n>` | | 覆盖窗口宽度（像素，非负整数） |
| `--height <n>` | | 覆盖窗口高度（像素，非负整数） |
| `--no-web-security` | | 强制解除跨域限制（等价 `webSecurity: false`） |
| `--show` | | 立即显示窗口，不等 `ready` 事件 |
| `--verbose` | | 输出调试日志 |
| `init` | | 在当前目录交互式创建 `package.json` |
| `--yes` | `-y` | 配合 `init`：跳过询问，全部使用默认值 |

### 3.3 退出码

| 码 | 含义 |
|----|------|
| `0` | 正常退出 |
| `1` | 参数错误 |
| `2` | 项目目录不存在或不可读 |
| `3` | `package.json` 不存在、解析失败，或 `name` 字段缺失/非法 |
| `4` | 无法定位入口 HTML |
| `5` | saucer 初始化失败（如 WebView 运行库缺失） |

### 3.4 使用示例

```bash
# 加载当前目录
whiz .

# 加载指定目录
whiz ./my-app

# 直接指定 package.json 文件
whiz ./my-app/package.json

# 打开调试窗口并输出日志
whiz ./my-app --devtools --verbose

# 覆盖窗口标题与尺寸
whiz ./my-app --title "调试版" --width 1280 --height 720

# 直接加载一个网址（不需要 package.json）
whiz --url https://example.com

# 解除跨域限制
whiz ./my-app --no-web-security
```

---

## 4. 初始化项目（`whiz init`）

`whiz init` 参考 `npm init`，在当前目录交互式生成 `package.json`：

```bash
mkdir my-app && cd my-app
whiz init
```

交互流程会依次询问：

1. `package name`（默认取目录名）
2. `version`（默认 `0.1.0`）
3. `description`（默认留空）
4. `license`（默认 `MIT`）
5. `entry HTML file (main)`（默认 `index.html`）
6. `webSecurity`（默认 `Y`）
7. `whiz` 扩展配置：`devTools`、`userAgent`、`backgroundColor`、`expose`

括号内为默认值，直接回车即采用默认值；按 `Ctrl+C` 可随时退出。

跳过全部询问，直接使用默认值：

```bash
whiz init -y
```

> 说明：
> - 窗口配置（`window`）不再逐项询问，直接采用内置默认值（1024×768、可缩放、居中、等 `ready` 后显示）。
> - `whiz init` 只写入 whiz 会读取的字段，以及 `license`、`description` 等标准元数据字段；其余 npm 字段不会生成。
> - `package name` 默认取当前**目录名**；若目录名无法推断（为空、`.`、`..` 或全为非法字符）或推断结果为保留名 `whiz-app`，会直接报错退出并提示具体原因，请修改目录名后重试。

---

## 5. `package.json` 配置

whiz 只读取以下字段，其余字段（`dependencies`、`scripts` 等）一律忽略，也不会执行任何 npm 逻辑。

### 5.1 顶层字段

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `name` | string | 必填 | 应用名称，同时兜底窗口标题；规则见下方说明 |
| `version` | string | `"0.0.0"` | 应用版本，日志中显示 |
| `main` | string | `"index.html"` | 入口 HTML 相对路径 |
| `webSecurity` | bool | `true` | 是否启用同源策略 / 跨域限制 |
| `window` | object | 见 5.2 | 窗口配置 |
| `whiz` | object | 见 5.3 | whiz 专属扩展配置 |

> **`name` 字段校验规则**（不满足则直接报错退出，退出码 `3`）：
> - 必须存在，且为**非空字符串**；
> - 不能包含 `/` 或 `\`，不能为 `.` 或 `..`；
> - 不能使用保留名 `whiz-app`（与 `--url` 直连模式的默认名冲突）。

### 5.2 `window` 字段

| 字段 | 类型 | 默认值 | 状态 | 说明 |
|------|------|--------|------|------|
| `title` | string | `name` | ✅ 生效 | 窗口标题 |
| `width` / `height` | number | `1024` / `768` | ✅ 生效 | 初始尺寸 |
| `minWidth` / `minHeight` | number | `0` | ✅ 生效 | 最小尺寸（`0` = 不限制） |
| `maxWidth` / `maxHeight` | number | `0` | ✅ 生效 | 最大尺寸（`0` = 不限制） |
| `resizable` | bool | `true` | ✅ 生效 | 是否允许缩放 |
| `frameless` | bool | `false` | ✅ 生效 | 无边框窗口 |
| `alwaysOnTop` | bool | `false` | ✅ 生效 | 置顶 |
| `show` | bool | `false` | ✅ 生效 | `false` = 等 DOM `ready` 后再显示，避免白屏 |
| `icon` | string | — | ✅ 生效 | 图标路径（相对项目根目录） |
| `fullscreen` | bool | `false` | ⚠️ 降级 | 当前降级为“最大化窗口”（saucer 无原生全屏 API） |
| `x` / `y` | number | `-1` | ⚠️ 未生效 | 已解析，暂未应用（初始位置由系统决定） |
| `center` | bool | `true` | ⚠️ 未生效 | 已解析，暂未应用 |
| `transparent` | bool | `false` | ⚠️ 未生效 | 已解析，暂未应用 |

### 5.3 `whiz` 扩展字段

| 字段 | 类型 | 默认值 | 状态 | 说明 |
|------|------|--------|------|------|
| `devTools` | bool | `false` | ✅ 生效 | 启动时打开开发者工具 |
| `backgroundColor` | string | `"#FFFFFF"` | ✅ 生效 | 窗口背景色，形如 `#RRGGBB` |
| `userAgent` | string | — | ⚠️ 未生效 | 已解析，UA 覆盖暂未生效 |
| `expose` | string[] | 全量 | ⚠️ 未生效 | 已解析，模块白名单限制暂未生效（当前始终暴露全部模块） |

### 5.4 `webSecurity` 说明

| 值 | 行为 |
|----|------|
| `true`（默认） | 保持浏览器安全策略；`file://` 下 `fetch()` / `XHR` / ES Module 动态 `import()` 受同源限制 |
| `false` | 解除跨域限制，允许本地读取与跨源请求，适合本地工具类应用 |

> 在 WebKitGTK 后端，`webSecurity: false` 已实现；其他平台会记录“暂未实现”的警告。

### 5.5 完整示例

```json
{
  "name": "markdown-editor",
  "version": "0.3.1",
  "main": "src/index.html",
  "webSecurity": false,
  "window": {
    "title": "Markdown Editor",
    "width": 1280,
    "height": 800,
    "center": true,
    "show": false,
    "icon": "assets/icon.png"
  },
  "whiz": {
    "devTools": true,
    "backgroundColor": "#1e1e1e",
    "expose": ["dialog", "window", "app"]
  }
}
```

### 5.6 入口 HTML 解析规则

1. `main` 指向 `.html` / `.htm` 文件 → 直接使用；
2. `main` 指向目录 → 追加 `index.html`；
3. 无 `main` 或解析失败 → 依次尝试 `index.html` → `index.htm` → `main.html`；
4. `main` 指向 `.js` 文件 → **直接报错**（whiz 不提供 Node.js 运行时，请改用 HTML 入口并通过 `<script>` 引入 JS）。

---

## 6. `window.whiz` 前端 API

whiz 在页面加载前注入全局对象 `window.whiz`，所有原生能力都挂在该命名空间下。

```text
window.whiz
├── version          // whiz 运行时版本
├── platform         // 'linux' | 'darwin' | 'win32'
├── app              // 应用级控制
├── window           // 当前窗口控制
└── dialog           // 文件对话框
```

通用约定：

- 所有异步方法返回 `Promise`；
- 拒绝时抛出 `Error`，`error.message` 为原因；
- 参数与返回值均为 JSON 可序列化对象。

### 6.1 `window.whiz.app`

| 成员 | 说明 |
|------|------|
| `version` | whiz 版本号字符串 |
| `platform` | 平台标识 |
| `quit()` | 退出应用 |
| `getPath(name)` | 获取常见目录，`name` 支持：`"home"`、`"temp"` / `"tmp"`、`"cwd"` |

```js
await window.whiz.app.quit();
const home = await window.whiz.app.getPath("home");
```

### 6.2 `window.whiz.window`

| 方法 | 状态 | 说明 |
|------|------|------|
| `setTitle(text)` | ✅ | 修改窗口标题 |
| `getTitle()` | ✅ | 读取窗口标题 |
| `setSize(w, h)` | ✅ | 修改窗口尺寸 |
| `getSize()` | ✅ | 读取窗口尺寸，返回 `[w, h]` |
| `minimize()` | ✅ | 最小化 |
| `maximize()` | ✅ | 最大化 |
| `unmaximize()` | ✅ | 取消最大化 |
| `setAlwaysOnTop(bool)` | ✅ | 置顶开关 |
| `focus()` | ✅ | 聚焦窗口 |
| `close()` | ✅ | 关闭窗口 |
| `setFullscreen(bool)` | ⚠️ | 当前映射为“最大化/还原”（无原生全屏） |
| `setPosition(x, y)` | ❌ | 暂未实现，调用会 reject |
| `getPosition()` | ❌ | 暂未实现，固定返回 `[-1, -1]` |
| `center()` | ❌ | 暂未实现，调用会 reject |
| `blur()` | ❌ | 暂未实现，调用会 reject |

```js
await window.whiz.window.setTitle("新标题");
const [w, h] = await window.whiz.window.getSize();
```

### 6.3 `window.whiz.dialog`

| 方法 | 说明 |
|------|------|
| `showOpenDialog(options)` | 打开文件 / 文件夹选择器，返回 `{ canceled, filePaths }` |
| `showSaveDialog(options)` | 保存文件选择器，返回 `{ canceled, filePath }` |

`options` 支持字段：

| 字段 | 类型 | 说明 |
|------|------|------|
| `defaultPath` | string | 默认路径 |
| `filters` | array | 文件类型过滤器，形如 `[{ name, extensions: ["jpg","png"] }]` |
| `properties` | array | 支持 `"openDirectory"`、`"multiSelections"` |

> `title`、`buttonLabel` 等字段当前会被忽略。

示例：

```js
const res = await window.whiz.dialog.showOpenDialog({
  defaultPath: "/home/me/Pictures",
  filters: [{ name: "Images", extensions: ["jpg", "png", "gif"] }],
  properties: ["openFile", "multiSelections"]
});
if (!res.canceled) {
  console.log("已选择：", res.filePaths);
}
```

```js
const res = await window.whiz.dialog.showSaveDialog({
  defaultPath: "config.json",
  filters: [{ name: "JSON", extensions: ["json"] }]
});
if (!res.canceled) {
  console.log("保存到：", res.filePath);
}
```

---

## 7. 平台差异

| 平台 | 底层 WebView | 备注 |
|------|-------------|------|
| Linux | WebKitGTK 6.0（GTK4） | `webSecurity`、窗口图标均已实现；`fullscreen` 降级为最大化 |
| macOS | WKWebView | Dock 图标需打包成 `.app` 并配置 `icns`，仅设置 `window.icon` 对 Dock 不生效 |
| Windows | WebView2 | 依赖 WebView2 Runtime（Win10/11 通常预装） |

窗口图标（`window.icon`）：

- Linux：通过 `GdkToplevel` 的图标列表设置，任务栏/窗口图标生效；
- Windows：通过 saucer 的 `set_icon` 生效；
- macOS：窗口标题栏图标可能生效，Dock 图标需应用包 `icns`。

### 数据存储位置

whiz 将 WebView 的持久化数据（Cookie 等）统一存放到用户数据目录，**不会在项目目录中生成 `.saucer` 文件**：

| 平台 | 存储目录 |
|------|----------|
| Linux / macOS | `$HOME/.local/share/whiz/<name>/` |
| Windows | `%USERPROFILE%\.local\share\whiz\<name>\` |

其中 `<name>` 取自 `package.json` 的 `name` 字段；`--url` 直连模式使用默认名 `whiz-app`。

---

## 8. 常见问题（FAQ）

### 8.1 提示“未找到 package.json”

whiz 现在要求目录中必须存在 `package.json`，否则报错退出（退出码 `3`）。请确认运行目录正确，或先用 `whiz init` 生成配置。

### 8.2 窗口图标不显示

- 请确认 `package.json` 中 `window.icon` 路径正确（相对项目根目录），文件为 PNG 等受支持格式；
- 可加 `--verbose` 查看图标加载日志（会打印加载成功、纹理尺寸，或失败原因）；
- 注意：用 `xprop` 查看 `_NET_WM_ICON` 时，较大的图标数组可能显示为空（这是 xprop 的显示限制，不代表属性未设置）；
- macOS 的 Dock 图标需要 `.app` 包与 `icns`。

### 8.3 本地 `fetch()` 读取 JSON 被拦截

默认 `webSecurity: true` 下，`file://` 页面的 `fetch()` 受同源策略限制。可在 `package.json` 中设置：

```json
{ "webSecurity": false }
```

或临时用 `whiz ./my-app --no-web-security` 运行。

### 8.4 窗口启动时有白屏闪烁

`window.show` 默认为 `false`，表示等 DOM `ready` 后再显示窗口（避免白屏）。如果你需要立即显示，设置：

```json
{ "window": { "show": true } }
```

或命令行加 `--show`。

### 8.5 构建报错缺少 `<print>` 头文件

当前代码使用 C++23 的 `std::print`/`std::println`，需要 GCC 14+。请安装并显式使用 `g++-14`：

```bash
CXX=g++-14 cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 8.6 提示 `name` 字段缺失或非法

whiz 要求 `package.json` 的 `name` 必须是非空字符串：不含 `/`、`\`，不能为 `.` / `..`，也不能使用保留名 `whiz-app`（与 `--url` 直连模式默认名冲突）。请检查 `name` 字段后重试，详见 [5.1](#51-顶层字段)。

### 8.7 项目的 Cookie / 数据存在哪里？

whiz 会把 WebView 持久化数据（Cookie 等）写入用户数据目录，不再在项目目录生成 `.saucer` 文件。具体路径见 [7. 平台差异](#7-平台差异) 中的“数据存储位置”。

---

## 9. 附录：配置与 API 速查

### 9.1 `package.json` 字段速查

| 字段 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `name` | string | 必填 | 应用名称（必填，不能为保留名 `whiz-app`） |
| `version` | string | `"0.0.0"` | 应用版本 |
| `main` | string | `"index.html"` | 入口 HTML |
| `webSecurity` | bool | `true` | 同源策略 |
| `window.title` | string | `name` | 窗口标题 |
| `window.width` / `height` | number | 1024 / 768 | 初始尺寸 |
| `window.minWidth` / `minHeight` | number | 0 | 最小尺寸 |
| `window.maxWidth` / `maxHeight` | number | 0 | 最大尺寸 |
| `window.resizable` | bool | `true` | 可缩放 |
| `window.frameless` | bool | `false` | 无边框 |
| `window.alwaysOnTop` | bool | `false` | 置顶 |
| `window.show` | bool | `false` | 立即显示（否则等 `ready`） |
| `window.icon` | string | — | 图标路径（相对项目根） |
| `window.fullscreen` | bool | `false` | 全屏（当前降级为最大化） |
| `window.x` / `y` | number | -1 | 初始位置（暂未生效） |
| `window.center` | bool | `true` | 居中（暂未生效） |
| `window.transparent` | bool | `false` | 透明背景（暂未生效） |
| `whiz.devTools` | bool | `false` | 启动 DevTools |
| `whiz.backgroundColor` | string | `"#FFFFFF"` | 窗口背景色 |
| `whiz.userAgent` | string | — | 覆盖 UA（暂未生效） |
| `whiz.expose` | string[] | 全量 | 模块白名单（暂未生效） |

### 9.2 `window.whiz` 速查

| 路径 | 状态 | 说明 |
|------|------|------|
| `whiz.version` | ✅ | 运行时版本 |
| `whiz.platform` | ✅ | 平台标识 |
| `whiz.app.quit()` | ✅ | 退出应用 |
| `whiz.app.getPath(name)` | ✅ | `home` / `temp` / `cwd` |
| `whiz.window.setTitle/getTitle` | ✅ | 标题 |
| `whiz.window.setSize/getSize` | ✅ | 尺寸 |
| `whiz.window.minimize/maximize/unmaximize` | ✅ | 窗口状态 |
| `whiz.window.setAlwaysOnTop/focus/close` | ✅ | 置顶 / 聚焦 / 关闭 |
| `whiz.window.setFullscreen` | ⚠️ | 映射为最大化 |
| `whiz.window.setPosition/getPosition/center/blur` | ❌ | 暂未实现 |
| `whiz.dialog.showOpenDialog` | ✅ | 打开对话框 |
| `whiz.dialog.showSaveDialog` | ✅ | 保存对话框 |

---

**文档版本**：1.0
**适用 whiz 版本**：0.1.x（实验性）
**运行库**：saucer v6.0.0、saucer/desktop v3.0.0、nlohmann/json v3.11.3
**许可证**：GNU Affero General Public License v3.0
