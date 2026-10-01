// whiz 桥接脚本：定义 window.whiz 命名空间。
// 内部通过 saucer 的 exposed 机制调用 C++ 侧原生函数。
// 由 whiz 在页面创建阶段注入（DOM ready 前）。

(function () {
    "use strict";

    if (window.whiz) {
        return;
    }

    // 调用 C++ 侧 exposed 函数。返回 Promise。
    function call(name, ...args) {
        if (typeof window.saucer === "undefined" || typeof window.saucer.exposed === "undefined") {
            return Promise.reject(new Error("saucer bridge 不可用"));
        }
        return window.saucer.exposed[name](...args);
    }

    const whiz = {
        version: "__WHIZ_VERSION__",
        platform: "__WHIZ_PLATFORM__",

        app: {
            get version() {
                return whiz.version;
            },
            get platform() {
                return whiz.platform;
            },
            quit() {
                return call("whiz_app_quit");
            },
            getPath(name) {
                return call("whiz_app_get_path", name);
            },
        },

        window: {
            setTitle(text) {
                return call("whiz_window_set_title", text);
            },
            getTitle() {
                return call("whiz_window_get_title");
            },
            setSize(w, h) {
                return call("whiz_window_set_size", w, h);
            },
            getSize() {
                return call("whiz_window_get_size");
            },
            setPosition(x, y) {
                return call("whiz_window_set_position", x, y);
            },
            getPosition() {
                return call("whiz_window_get_position");
            },
            center() {
                return call("whiz_window_center");
            },
            minimize() {
                return call("whiz_window_minimize");
            },
            maximize() {
                return call("whiz_window_maximize");
            },
            unmaximize() {
                return call("whiz_window_unmaximize");
            },
            setFullscreen(v) {
                return call("whiz_window_set_fullscreen", v);
            },
            setAlwaysOnTop(v) {
                return call("whiz_window_set_always_on_top", v);
            },
            focus() {
                return call("whiz_window_focus");
            },
            blur() {
                return call("whiz_window_blur");
            },
            close() {
                return call("whiz_window_close");
            },
        },

        dialog: {
            showOpenDialog(options) {
                return call("whiz_dialog_open", JSON.stringify(options || {})).then(JSON.parse);
            },
            showSaveDialog(options) {
                return call("whiz_dialog_save", JSON.stringify(options || {})).then(JSON.parse);
            },
        },
    };

    Object.defineProperty(window, "whiz", {
        value: whiz,
        writable: false,
        configurable: false,
        enumerable: true,
    });
})();
