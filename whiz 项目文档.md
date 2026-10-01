# whiz 项目文档

## 1. 项目概述

**whiz** 是一个基于 [saucer](https://github.com/saucer/saucer) C++ 库构建的**轻量级 Web 桌面应用运行时**。它借鉴 [NW.js](https://nwjs.io/) 的使用方式：用户只需准备一个普通的 Web 项目目录（含 `package.json` 与入口 HTML），然后执行一条命令即可将其变为原生桌面应用。

```
whiz <网页项目目录>
```

whiz 会自动完成：

1. 读取目标目录下的 `package.json`；
2. 解析入口 HTML（`main` 字段）；
3. 提取窗口配置（标题、尺寸、`webSecurity` 等）；
4. 使用 saucer 创建原生窗口并加载该 HTML；
5. 在页面中注入 `window.whiz` 命名空间，向 Web 前端暴露原生能力；
6. 进入事件循环，直到窗口关闭或用户退出。

**核心定位**：

- 面向 **Web 前端开发者**：只用写 HTML/CSS/JS，不用碰 C++；
- 面向 **脚本化桌面工具**：一个目录 + 一条命令就是一个桌面程序；
- 相比 NW.js / Electron，**体积更小、依赖更少、启动更快**——whiz 不打包 Chromium，直接复用系统原生 WebView（WebKitGTK / WKWebView / WebView2）。

**非目标**：

- 不内置 Node.js 运行时（无 `require`、无 `fs`、无 npm 模块解析）；
- 不提供浏览器级深度定制；
- 不做资源打包（whiz 直接读磁盘目录，不把资源嵌入可执行文件）。

---

## 2. 快速开始

### 2.1 准备一个 Web 项目

```
my-app/
├── package.json
├── index.html
├── style.css
└── app.js
```

`package.json` 示例（片段）：

```json
{
  "name": "my-app",
  "main": "index.html",
  "window": { "title": "我的 whiz 应用", "width": 1024, "height": 768 }
}
```

`index.html`（片段）：

```html
<!DOCTYPE html>
<html>
  <body>
    <h1>Hello, whiz!</h1>
    <script src="app.js"></script>
  </body>
</html>
```

### 2.2 运行

```bash
whiz ./my-app
```

窗口弹出，加载 `my-app/index.html`。

### 2.3 帮助与版本

```bash
whiz --help
whiz --version
```

---

## 3. 命令行接口

### 3.1 用法

```
whiz [选项] <项目目录>
whiz init [-y]
```

省略 `<项目目录>` 时默认使用当前工作目录 `.`。

`whiz init` 参考 `npm init`，在当前目录交互式创建 `package.json`；`-y` / `--yes` 跳过询问并直接使用默认值。

### 3.2 选项

| 选项 | 简写 | 说明 |
|------|------|------|
| `--help` | `-h` | 打印帮助并退出 |
| `--version` | `-v` | 打印版本号并退出 |
| `--devtools` | `-d` | 启动时打开开发者工具 |
| `--url <url>` | | 直接加载指定 URL，跳过 `package.json` 解析 |
| `--title <text>` | | 覆盖窗口标题 |
| `--width <n>` | | 覆盖窗口宽度（像素） |
| `--height <n>` | | 覆盖窗口高度（像素） |
| `--no-web-security` | | 强制解除跨域限制（等价于配置项 `webSecurity: false`） |
| `--show` | | 立即显示窗口，不等 `ready` 事件 |
| `--verbose` | | 输出调试日志 |
| `init` | | 在当前目录交互式创建 `package.json` |
| `--yes` | `-y` | 配合 `init`：跳过询问，全部使用默认值 |

### 3.3 参数解析规则

- `<项目目录>` 可以是**目录**、**`package.json` 文件路径**，或省略（使用当前目录）。
- 目录中必须存在 `package.json`，否则直接报错退出；入口 HTML 通过 `main` 字段解析，若未设置或路径无效则回退查找 `index.html` / `index.htm` / `main.html`。
- 提供 `--url` 时忽略目录参数。
- `init` 不接受额外的项目目录参数，始终在当前工作目录执行。
- `whiz init` 会从当前目录名自动推断项目名；若目录名无法推断（为空、`.`、`..` 或全为非法字符）或推断结果为保留名 `whiz-app`，将直接报错退出，请修改目录名后重试。
- **命令行选项优先级高于 `package.json` 中的配置。**

### 3.4 退出码

| 码 | 含义 |
|----|------|
| `0` | 正常退出 |
| `1` | 参数错误 |
| `2` | 项目目录不存在或不可读 |
| `3` | `package.json` 不存在、解析失败，或 `name` 字段缺失/非法 |
| `4` | 无法定位入口 HTML |
| `5` | saucer 初始化失败（如 WebView 运行库缺失） |

---

## 4. `package.json` 规范

whiz 只读取以下字段，其余字段（`dependencies`、`scripts`、`license`、`description` 等）一律忽略，也不会执行任何 npm 相关逻辑。不过 `whiz init` 会按 npm 惯例写入 `license`、`description` 等标准元数据字段，方便阅读与后续复用。

### 4.1 顶层字段

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `name` | `string` | 必填 | 应用名称，兜底窗口标题；规则见下方说明 |
| `version` | `string` | `"0.0.0"` | 应用版本，日志中显示 |
| `main` | `string` | `"index.html"` | 入口 HTML 相对路径 |
| `webSecurity` | `bool` | `true` | **是否启用同源策略 / 跨域限制**（对齐 Electron 的 `webPreferences.webSecurity`） |
| `window` | `object` | 见下 | 窗口配置 |
| `whiz` | `object` | 见下 | whiz 专属扩展配置 |
| `license` | `string` | — | 标准 npm 元数据字段（如 `MIT`、`Apache-2.0`），由 `whiz init` 写入，运行时忽略 |

> **`name` 字段校验规则**（不满足则直接报错退出，退出码 `3`）：
> - 必须存在，且为**非空字符串**；
> - 不能包含 `/` 或 `\`，不能为 `.` 或 `..`；
> - 不能使用保留名 `whiz-app`（与 `--url` 直连模式的默认名冲突）。

### 4.2 `webSecurity` 字段

`webSecurity` 用于控制 WebView 是否启用**同源策略与跨域限制**，语义对齐 Electron 的 `webPreferences.webSecurity`：

| 值 | 行为 |
|----|------|
| `true`（默认） | 保持浏览器安全策略。`file://` 下 `fetch()` / `XHR` / ES Module 动态 `import()` 会受同源限制。 |
| `false` | **解除跨域限制**：允许页面跨源请求、访问本地文件、绕过 CORS 检查。适合本地工具、内嵌面板、调试场景。 |

**使用建议**：

- 生产环境默认保持 `true`；
- 若项目需要从本地 HTML 通过 `fetch()` 读取磁盘数据，或跨域访问外部服务，可显式设为 `false`；
- 该字段也可以由命令行 `--no-web-security` 临时覆盖，优先级更高；
- 在部分平台上（WKWebView / WebView2），解除方式通过底层 WebView 的开发者开关实现，可能需要在打包/签名时声明相应能力。

示例（片段）：

```json
{ "webSecurity": false }
```

### 4.3 `window` 字段

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `title` | `string` | `name` | 窗口标题 |
| `width` / `height` | `number` | `1024` / `768` | 初始尺寸 |
| `minWidth` / `minHeight` | `number` | `0` | 最小尺寸（`0` 表示不限制） |
| `maxWidth` / `maxHeight` | `number` | `0` | 最大尺寸 |
| `x` / `y` | `number` | `-1` | 初始坐标，`-1` 表示由系统决定 |
| `resizable` | `bool` | `true` | 是否允许缩放 |
| `frameless` | `bool` | `false` | 无边框窗口 |
| `transparent` | `bool` | `false` | 背景透明 |
| `alwaysOnTop` | `bool` | `false` | 置顶 |
| `fullscreen` | `bool` | `false` | 启动即全屏 |
| `show` | `bool` | `false` | `false` 表示等 `ready` 后再显示（避免白屏） |
| `center` | `bool` | `true` | 启动时居中 |
| `icon` | `string` | — | 图标路径（相对项目根） |

### 4.4 `whiz` 扩展字段

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `devTools` | `bool` | `false` | 启动时打开开发者工具 |
| `userAgent` | `string` | — | 覆盖默认 User-Agent |
| `backgroundColor` | `string` | `"#FFFFFF"` | 窗口背景色 |
| `expose` | `string[]` | 全量 | 白名单：限制 `window.whiz` 下暴露哪些模块（如 `["dialog"]`） |

### 4.5 完整示例

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

### 4.6 入口解析规则

1. 若 `main` 存在且指向 `.html` / `.htm` 文件 → 使用它；
2. 若 `main` 指向目录 → 追加 `index.html`；
3. 若无 `main` 或解析失败 → 依次尝试 `index.html` / `index.htm` / `main.html`；
4. 若 `main` 指向 `.js` 文件 → **直接报错**（whiz 不提供 Node 运行时，无法执行 JS 入口脚本）。

---

## 5. 目录结构

```
whiz/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── include/whiz/          # 头文件：cli / init / manifest / window / paths / logger
├── src/                   # 对应实现
├── third_party/           # 通过 FetchContent 引入 saucer / saucer-desktop / nlohmann_json
├── scripts/               # 注入页面的 JS 桥接脚本（window.whiz）
├── examples/hello/        # 示例项目
├── tests/                 # 单元测试
└── build/                 # 构建产物
```

`scripts/` 下的桥接脚本由 whiz 在页面创建时注入，用于定义 `window.whiz`（见第 7 章）。

---

## 6. 系统架构

```
┌───────────────────────────────────────────────────────────────┐
│                        用户命令行                              │
│                    $ whiz ./my-app                            │
└────────────────────────────┬──────────────────────────────────┘
                             │
                             ▼
┌───────────────────────────────────────────────────────────────┐
│                       whiz CLI 层                              │
│  • 参数解析（cli）                                             │
│  • 项目目录定位（paths）                                       │
│  • --url / --title / --no-web-security 等覆盖项                │
└────────────────────────────┬──────────────────────────────────┘
                             │
                             ▼
┌───────────────────────────────────────────────────────────────┐
│                     manifest 解析层                            │
│  • 读取 package.json（nlohmann/json）                          │
│  • 填充 Manifest / WindowConfig / 安全策略                     │
│  • 解析入口 HTML 绝对路径                                       │
└────────────────────────────┬──────────────────────────────────┘
                             │
                             ▼
┌───────────────────────────────────────────────────────────────┐
│                      窗口构建层                                │
│  • 创建 saucer::application                                    │
│  • 创建 saucer::smartview（应用 WindowConfig）                 │
│  • 应用 webSecurity 策略（通过平台 API 关闭同源策略）           │
│  • 加载本地 HTML（file:// URI）                                │
│  • 注入 window.whiz 桥接脚本                                    │
│  • 监听 ready → show()（无闪烁启动）                           │
└────────────────────────────┬──────────────────────────────────┘
                             │
                             ▼
┌───────────────────────────────────────────────────────────────┐
│                    saucer（C++ 库）                            │
│  • 跨平台窗口抽象                                              │
│  • WebKitGTK 6.0 / WKWebView / WebView2                        │
│  • saucer::desktop 模块（dialog 能力）                         │
└───────────────────────────────────────────────────────────────┘
```

---

## 7. `window.whiz` —— 向页面暴露的原生能力

whiz 在页面加载前**统一注入**一个全局对象 `window.whiz`，所有原生能力（对话框、托盘、菜单、窗口控制、应用控制等）都挂在这个命名空间下，避免污染全局、方便类型声明。

```
window.whiz
├── version          // whiz 运行时版本
├── platform         // 'linux' | 'darwin' | 'win32'
├── app              // 应用级控制
├── window           // 当前窗口控制
├── dialog           // 文件对话框（基于 saucer/desktop）
├── tray             // 系统托盘（路线图）
├── menu             // 应用菜单（路线图）
└── clipboard        // 剪贴板（路线图）
```

> **白名单**：`package.json` 中的 `whiz.expose` 可以限制只暴露部分模块（例如仅 `["dialog"]`），未在名单中的模块不会挂载，用于收紧安全面。

### 7.1 通用约定

- 所有异步方法返回 `Promise`；
- 拒绝时抛出 `Error`，`error.message` 为简短原因；
- 所有参数与返回值均为 JSON 可序列化对象；
- 调用不存在的模块或方法会抛出 `TypeError`；
- **在 `file://` 下**，若 `webSecurity` 未关闭，跨源请求会被浏览器拦截；whiz 本身不改变这一点。

### 7.2 `window.whiz.app`

| 方法 / 属性 | 说明 |
|-------------|------|
| `version` | whiz 版本号字符串 |
| `platform` | 平台标识 |
| `quit()` | 退出应用 |
| `getPath(name)` | 获取常见目录（`home`、`temp` / `tmp`、`cwd`） |
| `onSecondInstance(cb)` | 路线图：第二个实例启动时触发（配合单实例锁） |

### 7.3 `window.whiz.window`

| 方法 | 状态 | 说明 |
|------|------|------|
| `setTitle(text)` | ✅ | 修改窗口标题 |
| `getTitle()` | ✅ | 读取标题 |
| `setSize(w, h)` / `getSize()` | ✅ | 修改 / 读取尺寸 |
| `minimize()` / `maximize()` / `unmaximize()` | ✅ | 窗口状态 |
| `setAlwaysOnTop(bool)` | ✅ | 置顶切换 |
| `focus()` | ✅ | 聚焦窗口 |
| `close()` | ✅ | 关闭窗口 |
| `setFullscreen(bool)` | ⚠️ | 当前映射为“最大化 / 还原”（无原生全屏） |
| `setPosition(x, y)` | ❌ | 暂未实现，调用会 reject |
| `getPosition()` | ❌ | 暂未实现，固定返回 `[-1, -1]` |
| `center()` | ❌ | 暂未实现，调用会 reject |
| `blur()` | ❌ | 暂未实现，调用会 reject |
| `on(event, cb)` / `off(event, cb)` | ❌ | 事件监听暂未实现 |

### 7.4 `window.whiz.dialog`

对齐 Electron 的 `dialog` 模块。

| 方法 | 说明 |
|------|------|
| `showOpenDialog(options): Promise<{ canceled, filePaths }>` | 打开文件 / 文件夹选择器 |
| `showSaveDialog(options): Promise<{ canceled, filePath }>` | 保存文件选择器 |

`options` 常用字段：`title`、`defaultPath`、`buttonLabel`、`filters`（`[{ name, extensions }]`）、`properties`（`openFile`、`openDirectory`、`multiSelections`、`showHiddenFiles`）。

页面侧调用片段：

```js
const res = await window.whiz.dialog.showOpenDialog({
  title: '选择图片',
  filters: [{ name: 'Images', extensions: ['jpg', 'png', 'gif'] }],
  properties: ['openFile', 'multiSelections']
});
if (!res.canceled) console.log(res.filePaths);
```

### 7.5 `window.whiz.tray`（路线图）

计划提供 `create(options)`、`setToolTip(text)`、`setMenu(items)`、`destroy()` 等方法，支持点击事件回传。

### 7.6 `window.whiz.menu`（路线图）

计划提供应用菜单与上下文菜单的构建能力，支持 `build(template)`、`setApplicationMenu(menu)`、`popup(menu, x, y)`。

### 7.7 `window.whiz.clipboard`（路线图）

计划提供 `readText()`、`writeText(text)`、`readImage()`、`writeImage(buffer)` 等能力。

### 7.8 桥接机制

- whiz 在页面创建阶段注入桥接脚本，脚本定义 `window.whiz` 各模块的桩；
- 每个模块的方法通过 saucer 的 `expose` 能力或注入的 `postMessage` 通道，转发到 C++ 侧；
- C++ 侧处理完成后通过 `evaluate` 执行回调，兑现前端的 Promise；
- 请求携带自增 `requestId`，前后端一一配对；
- 所有数据以 JSON 字符串传递，避免平台序列化差异。

---

## 8. 构建系统

### 8.1 依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| saucer | v6.0.0 | 跨平台 WebView 窗口 |
| saucer/desktop | v3.0.0 | 文件对话框等桌面能力（支撑 `window.whiz.dialog`） |
| nlohmann/json | v3.11.3 | 解析 `package.json` 与桥接消息 |
| CMake | ≥ 3.20 | 构建 |
| C++ 标准 | C++20 | — |

通过 CMake 的 `FetchContent` 自动拉取，无需手动安装。

### 8.2 构建命令

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/whiz examples/hello
```

### 8.3 安装

```bash
cmake --install build --prefix /usr/local
whiz ./my-app
```

---

## 9. 跨平台说明

| 平台 | 底层 WebView | 额外依赖 | 备注 |
|------|-------------|----------|------|
| **Linux** | WebKitGTK 6.0 (GTK4) | `libwebkitgtk-6.0-dev`、`libgtk-4-dev`、`libglib2.0-dev` | 开发需 dev 包；运行时需 `libwebkitgtk-6.0.so` |
| **macOS** | WKWebView | 系统自带 | 无额外运行时依赖；本地文件访问需声明 |
| **Windows** | WebView2 | WebView2 Runtime（Win10/11 通常预装） | 首次运行可能需联网安装 Runtime |

**Linux 构建前置**（一条命令即可）：

```bash
sudo apt install -y build-essential cmake git \
  libgtk-4-dev libwebkitgtk-6.0-dev libglib2.0-dev pkg-config
```

### 9.1 关于 `file://` 与 `webSecurity`

whiz 默认通过 `file://` URI 加载页面。此时：

- **`webSecurity: true`（默认）**：`fetch()` / `XHR` / 动态 `import()` 受同源策略限制，静态资源（`<img>`、`<script src>`、`<link>`）正常加载。
- **`webSecurity: false`**：关闭同源策略，`fetch('./data.json')` 等本地读取请求可以正常工作。适用于本地工具类应用。

**推荐组合**：

- 需要 `fetch` 本地 JSON → 设 `webSecurity: false`；
- 完全静态资源 → 保持 `true`（更安全）；
- 未来版本计划内置 `whiz.serve`（本地 HTTP 服务器），可替代直接关闭 webSecurity。

### 9.2 数据存储位置

whiz 将 WebView 的持久化数据（Cookie 等）统一存放到用户数据目录，**不再在项目目录中生成 `.saucer` 文件**：

| 平台 | 存储目录 |
|------|----------|
| Linux / macOS | `$HOME/.local/share/whiz/<name>/` |
| Windows | `%USERPROFILE%\.local\share\whiz\<name>\` |

其中 `<name>` 取自 `package.json` 的 `name` 字段；`--url` 直连模式使用默认名 `whiz-app`。

---

## 10. 使用示例

### 10.1 最小示例

```bash
mkdir hello && cd hello
# 写入 package.json 与 index.html（内容见第 2 章）
whiz .
```

### 10.2 覆盖配置

```bash
whiz ./my-app --title "调试窗口" --width 1280 --height 720
whiz ./my-app --devtools
whiz ./my-app --no-web-security
whiz --url https://example.com
```

### 10.3 前端调用原生对话框（片段）

```js
const result = await window.whiz.dialog.showSaveDialog({
  title: '保存配置',
  defaultPath: 'config.json',
  filters: [{ name: 'JSON', extensions: ['json'] }]
});
if (!result.canceled) {
  console.log('保存到：', result.filePath);
}
```

### 10.4 多页面项目

`pages/home.html` 中直接写相对路径引用资源即可：

```html
<link rel="stylesheet" href="../assets/style.css">
<script src="../scripts/app.js"></script>
```

---

## 11. 与 NW.js / Electron 对照

| 能力 | NW.js | Electron | whiz |
|------|-------|----------|------|
| 启动方式 | `nw <dir>` | `electron <dir>` | `whiz <dir>` |
| 入口配置 | `package.json` 的 `main` | 同上 | ✅ 相同 |
| 窗口配置 | `package.json` 的 `window` | 代码中 `BrowserWindow` | `package.json` 的 `window`（子集） |
| 关闭同源策略 | `chromium-args: --disable-web-security` | `webPreferences.webSecurity: false` | `package.json` 的 `webSecurity: false` |
| 前端 API 命名空间 | `nw.*` | `electron.*` | `window.whiz.*` |
| 内置 Chromium | 是 | 是 | ❌ 使用系统 WebView |
| 内置 Node.js | 是 | 是 | ❌ 无 |
| 分发体积 | 大 | 大 | 小（单文件 + 系统 WebView） |
| 启动速度 | 慢 | 慢 | 快 |
| 文件对话框 | `nw.File` / `dialog` | `dialog` 模块 | `window.whiz.dialog` |
| 托盘 / 菜单 | 支持 | 支持 | 路线图 |

**一句话总结**：whiz 是"NW.js 的最简形态"——只保留"目录 → 窗口"这条最短路径，牺牲 Node 集成和主进程脚本能力，换取极小的体积、极快的启动，以及一个干净统一的 `window.whiz` 原生能力命名空间。

---

## 12. 已知限制与变通

| 限制 | 变通方案 |
|------|----------|
| 无 Node.js 运行时 | 所有逻辑在前端执行；系统能力通过 `window.whiz` 桥接 |
| `main` 只支持 HTML 入口 | 若为 `.js` 报错并提示用户改为 HTML + `<script>` 引入 |
| `file://` 下 `fetch()` 受同源策略限制 | 设置 `webSecurity: false`；或仅使用静态 `<script>` / `<link>` |
| 关闭 webSecurity 带来安全风险 | 仅对可信项目使用；`whiz.expose` 可进一步收窄暴露的模块 |
| 部分 saucer 能力（无边框、透明）依赖平台支持 | 由 `WindowBuilder` 集中处理，未支持时降级为普通窗口并记录日志 |
| WebView2 Runtime 可能未安装（Windows） | 首次运行时提示用户下载；或由安装器捆绑 |
| Linux 上需 GTK4 | 编译时检测，提示安装 dev 包 |
| 不支持打包资源进二进制 | 保持"运行时读目录"的设计；分发时目录随二进制拷贝 |
| 单实例 | 路线图提供 `window.whiz.app` 中的单实例 API |

---

## 13. 开发路线图

1. **MVP**：解析 `package.json`，加载 `index.html`，在 Linux 上弹出窗口。
2. **窗口配置完整**：实现 `window` 全部字段与无闪烁启动。
3. **`webSecurity` 支持**：对齐 Electron 语义，实现关闭同源策略。
4. **`window.whiz` 基础命名空间**：注入桥接脚本，暴露 `app` 与 `window` 控制。
5. **`window.whiz.dialog`**：链接 `saucer::desktop`，实现 `showOpenDialog` / `showSaveDialog`。
6. **跨平台**：Windows（WebView2）、macOS（WKWebView）构建与验证。
7. **CLI 完善**：`--no-web-security`、`--show`、`--verbose` 等。
8. **内嵌 HTTP 服务器**：`whiz.serve` 选项，可作为 `webSecurity: false` 的替代方案。
9. **`window.whiz.tray` / `menu` / `clipboard`**：逐步补齐原生能力。
10. **单实例锁与激活聚焦**。
11. **打包分发**：各平台安装包（`.deb`、`.dmg`、`.msi`）。
12. **文件监听热重载**：`--watch` 下自动刷新。
13. **配置校验**：对 `package.json` 字段类型错误给出精确提示。

---

## 14. 附录 A：`package.json` 字段速查

| 字段 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `name` | string | 必填 | 应用名称（必填，不能为保留名 `whiz-app`） |
| `version` | string | `"0.0.0"` | 应用版本 |
| `main` | string | `"index.html"` | 入口 HTML |
| `webSecurity` | bool | `true` | 是否启用同源策略 |
| `window.title` | string | `name` | 窗口标题 |
| `window.width/height` | number | 1024/768 | 初始尺寸 |
| `window.minWidth/minHeight` | number | 0 | 最小尺寸 |
| `window.maxWidth/maxHeight` | number | 0 | 最大尺寸 |
| `window.x/y` | number | -1 | 初始位置 |
| `window.resizable` | bool | true | 可缩放 |
| `window.frameless` | bool | false | 无边框 |
| `window.transparent` | bool | false | 透明背景 |
| `window.alwaysOnTop` | bool | false | 置顶 |
| `window.fullscreen` | bool | false | 全屏 |
| `window.show` | bool | false | 立即显示 |
| `window.center` | bool | true | 居中 |
| `window.icon` | string | — | 图标路径 |
| `whiz.devTools` | bool | false | 启动 DevTools |
| `whiz.userAgent` | string | — | 覆盖 UA |
| `whiz.backgroundColor` | string | `"#FFFFFF"` | 窗口背景色 |
| `whiz.expose` | string[] | 全量 | `window.whiz` 模块白名单 |

## 15. 附录 B：`window.whiz` 命名空间速查

| 路径 | 状态 | 说明 |
|------|------|------|
| `whiz.version` | ✅ | 运行时版本 |
| `whiz.platform` | ✅ | 平台标识 |
| `whiz.app.*` | ✅ | 应用级控制（quit / getPath） |
| `whiz.window.*` | ⚠️ | 基础窗口控制已实现；`setPosition`/`getPosition`/`center`/`blur`/事件监听暂未实现 |
| `whiz.dialog.*` | ✅ | 文件打开 / 保存对话框 |
| `whiz.tray.*` | 路线图 | 系统托盘 |
| `whiz.menu.*` | 路线图 | 应用菜单 / 右键菜单 |
| `whiz.clipboard.*` | 路线图 | 剪贴板读写 |

---

**文档版本**：1.1
**适用 whiz 版本**：0.1.x（实验性）
**依赖库**：saucer v6.0.0、saucer/desktop v3.0.0、nlohmann/json v3.11.3
**许可证**：The GNU Affero General Public License