#include "PlatformInputListener.h"

/**
 * @file WindowsInputListener.cpp
 * @brief 基于 Windows 低级全局 Hook 的输入监听实现。
 *
 * 该实现把 Win32 细节限制在平台层内部，并将 WH_KEYBOARD_LL /
 * WH_MOUSE_LL 回调转换为库内跨平台的 KeyEvent 和 MouseEvent 对象。
 */

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace InputListener
{
namespace Internal
{
namespace
{

constexpr int buttonNone     = 0;            ///< 无活动鼠标按键。
constexpr int buttonLeft     = 1;            ///< 鼠标左键编号。
constexpr int buttonRight    = 2;            ///< 鼠标右键编号。
constexpr int buttonMiddle   = 3;            ///< 鼠标中键编号。
constexpr int buttonSide     = 4;            ///< 第一个扩展鼠标按键编号。
constexpr int buttonExtra    = 5;            ///< 第二个扩展鼠标按键编号。
constexpr int maxMouseButton = buttonExtra;  ///< 当前跟踪的最大鼠标按键编号。

std::atomic_bool listening{ false };  ///< Hook 消息循环是否应继续运行。
std::atomic<InputEventSink*> eventSink{ nullptr };  ///< 当前事件接收器。

std::thread hookThread;      ///< 持有 Windows Hook 消息循环的线程。
std::mutex  lifecycleMutex;  ///< 保护 Hook 生命周期状态的互斥锁。
std::condition_variable
      hooksReadyCondition;     ///< 通知 Hook 启动结果的条件变量。
bool  hooksReady     = false;  ///< Hook 启动流程是否已经结束。
bool  hooksInstalled = false;  ///< 键盘和鼠标低级 Hook 是否均安装成功。
DWORD hookThreadId   = 0;      ///< 用于投递 WM_QUIT 的 Windows 线程 ID。

HHOOK keyboardHook = nullptr;  ///< 已安装的 WH_KEYBOARD_LL Hook 句柄。
HHOOK mouseHook    = nullptr;  ///< 已安装的 WH_MOUSE_LL Hook 句柄。

std::array<bool, 256> keyDown{};  ///< 按虚拟键码索引的按下状态缓存。

/**
 * @brief Hook 线程维护的鼠标可变状态。
 */
struct MouseState {
    std::array<bool, maxMouseButton + 1> down{};  ///< 当前处于按下状态的按键。
    std::array<bool, maxMouseButton + 1> dragged{};  ///< 已发生拖拽的按键。
    int activeButton = buttonNone;  ///< 当前拖拽事件使用的鼠标按键。
};

MouseState mouseState;  ///< 当前鼠标按键和拖拽状态。

LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

/**
 * @brief 判断虚拟键码是否可用于索引本地按键状态表。
 *
 * @param virtualKey Windows 虚拟键码。
 * @return 键码位于当前跟踪的 0-255 范围内时返回 true。
 */
bool isValidVirtualKey(UINT virtualKey)
{
    return virtualKey < keyDown.size();
}

/**
 * @brief 判断库内鼠标按键编号是否被当前实现跟踪。
 *
 * @param button 库内鼠标按键编号。
 * @return 该按键属于受支持鼠标按键时返回 true。
 */
bool isValidMouseButton(int button)
{
    return button >= buttonLeft && button <= maxMouseButton;
}

/**
 * @brief 读取指定虚拟键码的缓存按下状态。
 *
 * @param virtualKey Windows 虚拟键码。
 * @return 当前认为该键处于按下状态时返回 true。
 */
bool isKeyDown(UINT virtualKey)
{
    return isValidVirtualKey(virtualKey) && keyDown[virtualKey];
}

/**
 * @brief 向 Windows 查询当前异步按键状态。
 *
 * @param virtualKey Windows 虚拟键码。
 * @return Windows 报告该键处于物理按下状态时返回 true。
 */
bool isAsyncKeyDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

/**
 * @brief 根据当前修饰键状态初始化本地按键缓存。
 *
 * 低级 Hook 只报告安装之后发生的状态变化，因此启动时需要主动采样修饰键，
 * 避免监听器在修饰键已按下时启动导致事件快照不准确。
 */
void initializeModifierState()
{
    keyDown.fill(false);
    keyDown[VK_LSHIFT]   = isAsyncKeyDown(VK_LSHIFT);
    keyDown[VK_RSHIFT]   = isAsyncKeyDown(VK_RSHIFT);
    keyDown[VK_SHIFT]    = keyDown[VK_LSHIFT] || keyDown[VK_RSHIFT];
    keyDown[VK_LCONTROL] = isAsyncKeyDown(VK_LCONTROL);
    keyDown[VK_RCONTROL] = isAsyncKeyDown(VK_RCONTROL);
    keyDown[VK_CONTROL]  = keyDown[VK_LCONTROL] || keyDown[VK_RCONTROL];
    keyDown[VK_LMENU]    = isAsyncKeyDown(VK_LMENU);
    keyDown[VK_RMENU]    = isAsyncKeyDown(VK_RMENU);
    keyDown[VK_MENU]     = keyDown[VK_LMENU] || keyDown[VK_RMENU];
    keyDown[VK_LWIN]     = isAsyncKeyDown(VK_LWIN);
    keyDown[VK_RWIN]     = isAsyncKeyDown(VK_RWIN);
}

/**
 * @brief 更新单个按键及聚合修饰键的缓存状态。
 *
 * @param virtualKey Windows 虚拟键码。
 * @param pressed 按键按下时为 true，释放时为 false。
 */
void setKeyState(UINT virtualKey, bool pressed)
{
    if ( !isValidVirtualKey(virtualKey) ) {
        return;
    }

    keyDown[virtualKey] = pressed;
    switch ( virtualKey ) {
    case VK_LSHIFT:
    case VK_RSHIFT:
        keyDown[VK_SHIFT] = keyDown[VK_LSHIFT] || keyDown[VK_RSHIFT];
        break;
    case VK_LCONTROL:
    case VK_RCONTROL:
        keyDown[VK_CONTROL] = keyDown[VK_LCONTROL] || keyDown[VK_RCONTROL];
        break;
    case VK_LMENU:
    case VK_RMENU:
        keyDown[VK_MENU] = keyDown[VK_LMENU] || keyDown[VK_RMENU];
        break;
    default: break;
    }
}

/**
 * @brief 根据缓存按键状态构造跨平台修饰键快照。
 *
 * @return 附加到事件上的修饰键状态。
 */
Modifiers snapshotModifiers()
{
    Modifiers modifiers;
    modifiers.shift =
        isKeyDown(VK_SHIFT) || isKeyDown(VK_LSHIFT) || isKeyDown(VK_RSHIFT);
    modifiers.control = isKeyDown(VK_CONTROL) || isKeyDown(VK_LCONTROL) ||
                        isKeyDown(VK_RCONTROL);
    modifiers.option =
        isKeyDown(VK_MENU) || isKeyDown(VK_LMENU) || isKeyDown(VK_RMENU);
    modifiers.command = isKeyDown(VK_LWIN) || isKeyDown(VK_RWIN);
    return modifiers;
}

/**
 * @brief 将 Windows UTF-16 文本片段转换为 UTF-8。
 *
 * @param text UTF-16 缓冲区指针。
 * @param length 需要转换的 UTF-16 code unit 数量。
 * @return UTF-8 编码文本；转换失败时返回空字符串。
 */
std::string wideToUtf8(const wchar_t* text, int length)
{
    if ( text == nullptr || length <= 0 ) {
        return {};
    }

    int size = WideCharToMultiByte(
        CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
    if ( size <= 0 ) {
        return {};
    }

    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, text, length, result.data(), size, nullptr, nullptr);
    return result;
}

/**
 * @brief 返回不可打印键或具名 Windows 按键的显示文本。
 *
 * @param virtualKey Windows 虚拟键码。
 * @return 显示名称；按键可打印或未知时返回空字符串。
 */
std::string specialKeyText(UINT virtualKey)
{
    switch ( virtualKey ) {
    case VK_ESCAPE: return "Esc";
    case VK_BACK: return "Backspace";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_SPACE: return " ";
    case VK_CAPITAL: return "CapsLock";
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT: return "Shift";
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL: return "Ctrl";
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU: return "Alt";
    case VK_LWIN:
    case VK_RWIN: return "Meta";
    case VK_APPS: return "Menu";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "PageUp";
    case VK_NEXT: return "PageDown";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_SNAPSHOT: return "PrintScreen";
    case VK_SCROLL: return "ScrollLock";
    case VK_PAUSE: return "Pause";
    case VK_NUMLOCK: return "NumLock";
    default: break;
    }

    if ( virtualKey >= VK_F1 && virtualKey <= VK_F24 ) {
        return "F" + std::to_string(virtualKey - VK_F1 + 1);
    }

    return {};
}

/**
 * @brief 简单 ASCII 字母和数字的兜底文本映射。
 *
 * @param virtualKey Windows 虚拟键码。
 * @param shiftPressed 当前事件中 Shift 是否处于按下状态。
 * @return 可打印 ASCII 文本；不支持时返回空字符串。
 */
std::string fallbackPrintableText(UINT virtualKey, bool shiftPressed)
{
    if ( virtualKey >= 'A' && virtualKey <= 'Z' ) {
        char ch = static_cast<char>(shiftPressed ? virtualKey
                                                 : virtualKey - 'A' + 'a');
        return std::string(1, ch);
    }

    if ( virtualKey >= '0' && virtualKey <= '9' ) {
        return std::string(1, static_cast<char>(virtualKey));
    }

    return {};
}

/**
 * @brief 将 Windows 按键事件转换为库内使用的显示文本。
 *
 * 具名按键会优先处理。可打印按键使用当前键盘布局通过 ToUnicodeEx 转换，
 * 转换失败后再使用简单 ASCII 兜底映射。
 *
 * @param virtualKey Windows 虚拟键码。
 * @param scanCode Hook 回调提供的硬件扫描码。
 * @param modifiers 事件派发时的修饰键状态。
 * @return KeyEvent::getKey() 使用的显示文本。
 */
std::string keyTextForWindowsKey(UINT virtualKey, UINT scanCode,
                                 const Modifiers& modifiers)
{
    std::string special = specialKeyText(virtualKey);
    if ( !special.empty() ) {
        return special;
    }

    BYTE keyboardState[256] = {};
    if ( modifiers.shift ) {
        keyboardState[VK_SHIFT] = 0x80;
        if ( isKeyDown(VK_LSHIFT) ) {
            keyboardState[VK_LSHIFT] = 0x80;
        }
        if ( isKeyDown(VK_RSHIFT) ) {
            keyboardState[VK_RSHIFT] = 0x80;
        }
    }
    if ( (GetKeyState(VK_CAPITAL) & 1) != 0 ) {
        keyboardState[VK_CAPITAL] = 1;
    }

    wchar_t buffer[8] = {};
    int     count     = ToUnicodeEx(virtualKey,
                                    scanCode,
                                    keyboardState,
                                    buffer,
                                    static_cast<int>(std::size(buffer)),
                                    0,
                                    GetKeyboardLayout(0));
    if ( count > 0 ) {
        return wideToUtf8(buffer, count);
    }

    return fallbackPrintableText(virtualKey, modifiers.shift);
}

/**
 * @brief 将通用修饰键虚拟键码解析为左右侧变体。
 *
 * WH_KEYBOARD_LL 会把部分修饰键报告为通用的 VK_SHIFT/VK_CONTROL/VK_MENU。
 * 本实现内部跟踪左右侧状态，以便在按下和释放转换之间保持聚合修饰键准确。
 *
 * @param info 低级键盘 Hook 的原始事件数据。
 * @return 归一化后的 Windows 虚拟键码。
 */
UINT normalizedVirtualKey(const KBDLLHOOKSTRUCT& info)
{
    UINT virtualKey = static_cast<UINT>(info.vkCode);
    bool extended   = (info.flags & LLKHF_EXTENDED) != 0;

    switch ( virtualKey ) {
    case VK_SHIFT: {
        UINT mapped = MapVirtualKeyW(info.scanCode, MAPVK_VSC_TO_VK_EX);
        return mapped == 0 ? virtualKey : mapped;
    }
    case VK_CONTROL: return extended ? VK_RCONTROL : VK_LCONTROL;
    case VK_MENU: return extended ? VK_RMENU : VK_LMENU;
    default: return virtualKey;
    }
}

/**
 * @brief 构造 KeyEvent 并派发到当前事件接收器。
 *
 * 释放事件会额外派发库内的完成态 PRESSED 事件，以保持与其他平台实现一致。
 *
 * @param virtualKey 作为 KeyEvent rawCode 的 Windows 虚拟键码。
 * @param scanCode Hook 回调提供的硬件扫描码。
 * @param type 库内键盘事件类型。
 */
void dispatchKeyEvent(UINT virtualKey, UINT scanCode, KeyEventType type)
{
    InputEventSink* sink = eventSink.load();
    if ( sink == nullptr ) {
        return;
    }

    Modifiers   modifiers = snapshotModifiers();
    std::string key = keyTextForWindowsKey(virtualKey, scanCode, modifiers);
    KeyEvent    event(static_cast<int>(virtualKey), key, modifiers, type);
    sink->dispatchKeyEvent(event);

    if ( type == KeyEventType::RELEASE ) {
        KeyEvent pressedEvent(static_cast<int>(virtualKey),
                              key,
                              modifiers,
                              KeyEventType::PRESSED);
        sink->dispatchKeyEvent(pressedEvent);
    }
}

/**
 * @brief 查找当前第一个处于按下状态的鼠标按键。
 *
 * @return 鼠标按键编号；没有被跟踪的按键按下时返回 buttonNone。
 */
int firstDownMouseButton()
{
    for ( int button = buttonLeft; button <= maxMouseButton; ++button ) {
        if ( mouseState.down[button] ) {
            return button;
        }
    }
    return buttonNone;
}

/**
 * @brief 构造 MouseEvent 并派发到当前事件接收器。
 *
 * @param button 库内鼠标按键编号。
 * @param type 库内鼠标事件类型。
 * @param point Windows 提供的屏幕坐标。
 * @param scrollDeltaX 以滚轮单位表示的水平滚动量。
 * @param scrollDeltaY 以滚轮单位表示的垂直滚动量。
 */
void dispatchMouseEvent(int button, MouseEventType type, const POINT& point,
                        double scrollDeltaX = 0.0, double scrollDeltaY = 0.0)
{
    InputEventSink* sink = eventSink.load();
    if ( sink == nullptr ) {
        return;
    }

    Modifiers  modifiers = snapshotModifiers();
    MouseEvent event(button,
                     modifiers,
                     type,
                     static_cast<int>(point.x),
                     static_cast<int>(point.y),
                     static_cast<double>(point.x),
                     static_cast<double>(point.y),
                     scrollDeltaX,
                     scrollDeltaY);
    sink->dispatchMouseEvent(event);
}

/**
 * @brief 更新鼠标按键状态并派发按下或释放事件。
 *
 * 释放事件会额外派发 PRESSED 或 DRAGGED 完成态事件。
 *
 * @param button 库内鼠标按键编号。
 * @param pressed 按键按下时为 true，释放时为 false。
 * @param point Windows 提供的屏幕坐标。
 */
void dispatchMouseButtonEvent(int button, bool pressed, const POINT& point)
{
    if ( !isValidMouseButton(button) ) {
        return;
    }

    if ( pressed ) {
        mouseState.down[button]    = true;
        mouseState.dragged[button] = false;
        mouseState.activeButton    = button;
        dispatchMouseEvent(button, MouseEventType::PRESS, point);
        return;
    }

    bool dragged               = mouseState.dragged[button];
    mouseState.down[button]    = false;
    mouseState.dragged[button] = false;

    dispatchMouseEvent(button, MouseEventType::RELEASE, point);
    dispatchMouseEvent(
        button,
        dragged ? MouseEventType::DRAGGED : MouseEventType::PRESSED,
        point);

    if ( mouseState.activeButton == button ) {
        mouseState.activeButton = firstDownMouseButton();
    }
}

/**
 * @brief 根据鼠标按键状态派发移动或拖拽事件。
 *
 * @param point Windows 提供的屏幕坐标。
 */
void dispatchMouseMoveEvent(const POINT& point)
{
    int button = mouseState.activeButton;
    if ( button == buttonNone ) {
        button                  = firstDownMouseButton();
        mouseState.activeButton = button;
    }

    if ( button == buttonNone ) {
        dispatchMouseEvent(buttonNone, MouseEventType::MOVE, point);
        return;
    }

    mouseState.dragged[button] = true;
    dispatchMouseEvent(button, MouseEventType::DRAG, point);
}

/**
 * @brief 将 Windows XBUTTON mouseData 映射为库内鼠标按键编号。
 *
 * @param mouseData MSLLHOOKSTRUCT 中的 mouseData 字段。
 * @return buttonSide、buttonExtra，未知时返回 buttonNone。
 */
int xButtonForMouseData(DWORD mouseData)
{
    WORD xButton = HIWORD(mouseData);
    if ( xButton == XBUTTON1 ) {
        return buttonSide;
    }
    if ( xButton == XBUTTON2 ) {
        return buttonExtra;
    }
    return buttonNone;
}

/**
 * @brief 获取包含 Hook 回调代码的模块句柄。
 *
 * @return 传递给 SetWindowsHookExW 的模块句柄。
 */
HMODULE moduleForHookProcs()
{
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&keyboardHookProc),
                       &module);
    return module;
}

/**
 * @brief 卸载当前已安装的低级 Hook。
 */
void clearHooks()
{
    if ( keyboardHook != nullptr ) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
    }
    if ( mouseHook != nullptr ) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = nullptr;
    }
}

/**
 * @brief Hook 持有线程的执行过程。
 *
 * 该线程会创建消息队列、安装键盘和鼠标低级 Hook、向启动方报告安装结果，
 * 然后运行 Win32 消息循环直到收到停止请求。
 */
void hookLoop()
{
    {
        std::lock_guard<std::mutex> lock(lifecycleMutex);
        hookThreadId = GetCurrentThreadId();
    }

    MSG queueMessage;
    PeekMessageW(&queueMessage, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    initializeModifierState();
    mouseState = {};

    HMODULE module = moduleForHookProcs();
    keyboardHook =
        SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHookProc, module, 0);
    mouseHook      = SetWindowsHookExW(WH_MOUSE_LL, mouseHookProc, module, 0);
    bool installed = keyboardHook != nullptr && mouseHook != nullptr;

    {
        std::lock_guard<std::mutex> lock(lifecycleMutex);
        hooksInstalled = installed;
        hooksReady     = true;
    }
    hooksReadyCondition.notify_one();

    if ( !installed ) {
        clearHooks();
        listening.store(false);
        std::lock_guard<std::mutex> lock(lifecycleMutex);
        hookThreadId = 0;
        return;
    }

    MSG message;
    while ( listening.load() && GetMessageW(&message, nullptr, 0, 0) > 0 ) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    clearHooks();
    listening.store(false);

    std::lock_guard<std::mutex> lock(lifecycleMutex);
    hookThreadId = 0;
}

/**
 * @brief 转换键盘状态变化的 WH_KEYBOARD_LL 回调。
 *
 * @param nCode Windows 提供的 Hook 处理码。
 * @param wParam 键盘消息编号。
 * @param lParam 指向 KBDLLHOOKSTRUCT 的指针。
 * @return CallNextHookEx 的返回值。
 */
LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if ( nCode == HC_ACTION && listening.load() ) {
        const auto* info = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        UINT        virtualKey = normalizedVirtualKey(*info);
        UINT        scanCode   = static_cast<UINT>(info->scanCode);

        if ( wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN ) {
            bool repeat = isKeyDown(virtualKey);
            setKeyState(virtualKey, true);
            dispatchKeyEvent(
                virtualKey,
                scanCode,
                repeat ? KeyEventType::REPEAT : KeyEventType::PRESS);
        } else if ( wParam == WM_KEYUP || wParam == WM_SYSKEYUP ) {
            setKeyState(virtualKey, false);
            dispatchKeyEvent(virtualKey, scanCode, KeyEventType::RELEASE);
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

/**
 * @brief 转换鼠标状态变化的 WH_MOUSE_LL 回调。
 *
 * @param nCode Windows 提供的 Hook 处理码。
 * @param wParam 鼠标消息编号。
 * @param lParam 指向 MSLLHOOKSTRUCT 的指针。
 * @return CallNextHookEx 的返回值。
 */
LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if ( nCode == HC_ACTION && listening.load() ) {
        const auto* info = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        switch ( wParam ) {
        case WM_MOUSEMOVE: dispatchMouseMoveEvent(info->pt); break;
        case WM_LBUTTONDOWN:
            dispatchMouseButtonEvent(buttonLeft, true, info->pt);
            break;
        case WM_LBUTTONUP:
            dispatchMouseButtonEvent(buttonLeft, false, info->pt);
            break;
        case WM_RBUTTONDOWN:
            dispatchMouseButtonEvent(buttonRight, true, info->pt);
            break;
        case WM_RBUTTONUP:
            dispatchMouseButtonEvent(buttonRight, false, info->pt);
            break;
        case WM_MBUTTONDOWN:
            dispatchMouseButtonEvent(buttonMiddle, true, info->pt);
            break;
        case WM_MBUTTONUP:
            dispatchMouseButtonEvent(buttonMiddle, false, info->pt);
            break;
        case WM_XBUTTONDOWN:
            dispatchMouseButtonEvent(
                xButtonForMouseData(info->mouseData), true, info->pt);
            break;
        case WM_XBUTTONUP:
            dispatchMouseButtonEvent(
                xButtonForMouseData(info->mouseData), false, info->pt);
            break;
        case WM_MOUSEWHEEL: {
            double delta = static_cast<SHORT>(HIWORD(info->mouseData)) /
                           static_cast<double>(WHEEL_DELTA);
            dispatchMouseEvent(
                buttonNone, MouseEventType::SCROLL, info->pt, 0.0, delta);
            break;
        }
        case WM_MOUSEHWHEEL: {
            double delta = static_cast<SHORT>(HIWORD(info->mouseData)) /
                           static_cast<double>(WHEEL_DELTA);
            dispatchMouseEvent(
                buttonNone, MouseEventType::SCROLL, info->pt, delta, 0.0);
            break;
        }
        default: break;
        }
    }

    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

}  // namespace

/**
 * @brief 启动 Windows 全局输入监听。
 *
 * 该函数会创建 Hook 持有线程，等待 Hook 安装成功或失败，并在监听期间把传入的
 * sink 作为事件派发目标。
 *
 * @param sink 转换后输入事件的接收器。
 */
void startPlatformInputListener(InputEventSink& sink)
{
    if ( listening.exchange(true) ) {
        return;
    }

    eventSink.store(&sink);

    {
        std::lock_guard<std::mutex> lock(lifecycleMutex);
        hooksReady     = false;
        hooksInstalled = false;
    }

    hookThread = std::thread(hookLoop);

    bool installed = false;
    {
        std::unique_lock<std::mutex> lock(lifecycleMutex);
        hooksReadyCondition.wait(lock, [] { return hooksReady; });
        installed = hooksInstalled;
    }

    if ( !installed ) {
        if ( hookThread.joinable() ) {
            hookThread.join();
        }
        eventSink.store(nullptr);
    }
}

/**
 * @brief 停止 Windows 全局输入监听并释放 Hook 资源。
 */
void stopPlatformInputListener()
{
    if ( !listening.exchange(false) ) {
        return;
    }

    std::thread threadToJoin;
    DWORD       threadId = 0;
    {
        std::lock_guard<std::mutex> lock(lifecycleMutex);
        threadId = hookThreadId;
        if ( threadId != 0 ) {
            PostThreadMessageW(threadId, WM_QUIT, 0, 0);
        }

        if ( hookThread.joinable() ) {
            if ( threadId == GetCurrentThreadId() ) {
                hookThread.detach();
            } else {
                threadToJoin = std::move(hookThread);
            }
        }
    }

    if ( threadToJoin.joinable() ) {
        threadToJoin.join();
    }

    eventSink.store(nullptr);
}

}  // namespace Internal
}  // namespace InputListener
