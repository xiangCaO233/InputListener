#include "PlatformInputListener.h"

#include "KeyMapping.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <linux/input.h>
#include <mutex>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace InputListener
{
namespace Internal
{
namespace
{

constexpr const char* inputDirectory = "/dev/input";  ///< Linux 输入设备目录。
constexpr int         buttonNone     = 0;             ///< 无鼠标按键。
constexpr int         buttonLeft     = 1;             ///< 鼠标左键编号。
constexpr int         buttonRight    = 2;             ///< 鼠标右键编号。
constexpr int         buttonMiddle   = 3;             ///< 鼠标中键编号。
constexpr int         buttonSide     = 4;             ///< 鼠标侧键编号。
constexpr int         buttonExtra    = 5;             ///< 鼠标扩展键编号。

std::atomic_bool listening{ false };       ///< 平台监听线程是否应该继续运行。
std::vector<std::thread> listenerThreads;  ///< 当前 Linux 设备监听线程列表。
std::mutex listenerThreadsMutex;           ///< 保护 listenerThreads 的互斥锁。
std::mutex modifiersMutex;                 ///< 保护 globalModifiers 的互斥锁。
Modifiers  globalModifiers;  ///< 所有 Linux 设备共享的修饰键状态。

/**
 * @brief Linux 输入设备能力集合。
 */
struct DeviceInfo {
    std::string path;      ///< /dev/input/event* 设备路径。
    std::string name;      ///< 设备名称。
    bool        keyboard;  ///< 是否具备键盘能力。
    bool        mouse;     ///< 是否具备鼠标能力。
};

/**
 * @brief 单个鼠标设备线程维护的相对移动状态。
 */
struct MouseState {
    int    x              = 0;    ///< 累计相对 x 坐标。
    int    y              = 0;    ///< 累计相对 y 坐标。
    int    pendingDx      = 0;    ///< 当前 SYN_REPORT 前累计的 x 增量。
    int    pendingDy      = 0;    ///< 当前 SYN_REPORT 前累计的 y 增量。
    double pendingScrollX = 0.0;  ///< 当前 SYN_REPORT 前累计的水平滚动增量。
    double pendingScrollY = 0.0;  ///< 当前 SYN_REPORT 前累计的垂直滚动增量。
    int    activeButton   = buttonNone;  ///< 当前按下的鼠标按键。
    bool   buttonDown     = false;       ///< 是否有鼠标按键处于按下状态。
    bool   dragged        = false;       ///< 当前按键周期内是否发生过拖动。
};

/**
 * @brief 判断位图中指定 bit 是否被设置。
 *
 * @param bits Linux ioctl 返回的位图数组。
 * @param bit 要检查的 bit 编号。
 * @return bit 已设置时返回 true。
 */
bool testBit(const unsigned long* bits, int bit)
{
    constexpr int bitsPerLong = static_cast<int>(sizeof(unsigned long) * 8);
    return bits[bit / bitsPerLong] & (1UL << (bit % bitsPerLong));
}

/**
 * @brief 将 Linux 鼠标按键码映射为库内部鼠标按键编号。
 *
 * @param code Linux 输入子系统按键码。
 * @return 鼠标按键编号；不支持的按键返回 0。
 */
int mouseButtonForCode(unsigned short code)
{
    switch ( code ) {
    case BTN_LEFT: return buttonLeft;
    case BTN_RIGHT: return buttonRight;
    case BTN_MIDDLE: return buttonMiddle;
    case BTN_SIDE: return buttonSide;
    case BTN_EXTRA: return buttonExtra;
    default: return buttonNone;
    }
}

/**
 * @brief 判断 Linux 按键码是否属于键盘按键范围。
 *
 * @param code Linux 输入子系统按键码。
 * @return 属于键盘键码时返回 true。
 */
bool isKeyboardKey(unsigned short code)
{
    return code < BTN_MISC;
}

/**
 * @brief 获取当前全局修饰键状态快照。
 *
 * @return 修饰键状态副本。
 */
Modifiers snapshotModifiers()
{
    std::lock_guard<std::mutex> lock(modifiersMutex);
    return globalModifiers;
}

/**
 * @brief 根据 Linux 按键事件更新全局修饰键状态。
 *
 * @param code Linux 输入子系统按键码。
 * @param value Linux 按键值：0 表示释放，1 表示按下，2 表示重复。
 */
void updateModifiers(unsigned short code, int value)
{
    std::lock_guard<std::mutex> lock(modifiersMutex);
    bool                        pressed = value != 0;
    switch ( code ) {
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT: globalModifiers.shift = pressed; break;
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL: globalModifiers.control = pressed; break;
    case KEY_LEFTALT:
    case KEY_RIGHTALT: globalModifiers.option = pressed; break;
    case KEY_LEFTMETA:
    case KEY_RIGHTMETA: globalModifiers.command = pressed; break;
    default: break;
    }
}

/**
 * @brief 查询单个 Linux event 设备的名称和输入能力。
 *
 * @param devicePath /dev/input/event* 设备路径。
 * @return 设备能力描述。
 */
DeviceInfo inspectDevice(const std::string& devicePath)
{
    DeviceInfo info{ devicePath, "未知设备", false, false };
    int        fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if ( fd < 0 ) {
        std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
                  << std::endl;
        return info;
    }

    char name[256] = "Unknown";
    if ( ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 ) {
        info.name = name;
    }

    constexpr int bitsPerLong = static_cast<int>(sizeof(unsigned long) * 8);
    std::array<unsigned long, EV_MAX / bitsPerLong + 1>  evbit{};
    std::array<unsigned long, KEY_MAX / bitsPerLong + 1> keybit{};
    std::array<unsigned long, REL_MAX / bitsPerLong + 1> relbit{};

    bool hasEvents = ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit.data()) >= 0;
    bool hasKeys =
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit.data()) >= 0;
    bool hasRel =
        ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit.data()) >= 0;

    if ( hasEvents && hasKeys && testBit(evbit.data(), EV_KEY) ) {
        info.keyboard = testBit(keybit.data(), KEY_A) ||
                        testBit(keybit.data(), KEY_SPACE) ||
                        testBit(keybit.data(), KEY_ENTER);
    }

    if ( hasEvents && hasKeys && hasRel && testBit(evbit.data(), EV_REL) ) {
        bool hasPointerAxis =
            testBit(relbit.data(), REL_X) || testBit(relbit.data(), REL_Y);
        bool hasScrollAxis = testBit(relbit.data(), REL_WHEEL) ||
                             testBit(relbit.data(), REL_HWHEEL);
#ifdef REL_WHEEL_HI_RES
        hasScrollAxis =
            hasScrollAxis || testBit(relbit.data(), REL_WHEEL_HI_RES);
#endif
#ifdef REL_HWHEEL_HI_RES
        hasScrollAxis =
            hasScrollAxis || testBit(relbit.data(), REL_HWHEEL_HI_RES);
#endif
        bool hasPointerButton = testBit(keybit.data(), BTN_LEFT) ||
                                testBit(keybit.data(), BTN_RIGHT);
        info.mouse = (hasPointerAxis && hasPointerButton) || hasScrollAxis;
    }

    close(fd);
    return info;
}

/**
 * @brief 枚举当前系统中所有可监听的键盘或鼠标 event 设备。
 *
 * @return 设备能力描述列表。
 */
std::vector<DeviceInfo> enumerateInputDevices()
{
    std::vector<DeviceInfo> devices;
    DIR*                    dir = opendir(inputDirectory);
    if ( dir == nullptr ) {
        std::cerr << "无法打开输入设备目录: " << strerror(errno) << std::endl;
        return devices;
    }

    struct dirent* entry;
    while ( (entry = readdir(dir)) != nullptr ) {
        if ( entry->d_name[0] == '.' ) {
            continue;
        }

        std::string devicePath =
            std::string(inputDirectory) + "/" + entry->d_name;
        if ( devicePath.find("event") == std::string::npos ) {
            continue;
        }

        auto info = inspectDevice(devicePath);
        if ( info.keyboard || info.mouse ) {
            devices.push_back(std::move(info));
        }
    }

    closedir(dir);
    std::sort(devices.begin(),
              devices.end(),
              [](const DeviceInfo& left, const DeviceInfo& right) {
                  return left.path < right.path;
              });
    return devices;
}

/**
 * @brief 把 Linux EV_KEY 键盘事件转换为 KeyEvent 并分发。
 *
 * @param sink 核心事件接收接口。
 * @param code Linux 输入子系统键码。
 * @param value Linux 按键值。
 */
void dispatchKeyEvent(InputEventSink& sink, unsigned short code, int value)
{
    updateModifiers(code, value);

    auto         modifiers = snapshotModifiers();
    KeyEventType type      = KeyEventType::PRESS;
    if ( value == 0 ) {
        type = KeyEventType::RELEASE;
    } else if ( value == 2 ) {
        type = KeyEventType::REPEAT;
    }

    KeyEvent event(
        code, keyTextForCode(code, modifiers.shift), modifiers, type);
    sink.dispatchKeyEvent(event);

    if ( type == KeyEventType::RELEASE ) {
        KeyEvent pressedEvent(
            code, event.getKey(), modifiers, KeyEventType::PRESSED);
        sink.dispatchKeyEvent(pressedEvent);
    }
}

/**
 * @brief 把 Linux 鼠标按键事件转换为 MouseEvent 并分发。
 *
 * @param sink 核心事件接收接口。
 * @param state 当前鼠标设备的状态。
 * @param code Linux 输入子系统按键码。
 * @param value Linux 按键值。
 */
void dispatchMouseButtonEvent(InputEventSink& sink, MouseState& state,
                              unsigned short code, int value)
{
    int button = mouseButtonForCode(code);
    if ( button == buttonNone ) {
        return;
    }

    auto modifiers = snapshotModifiers();
    if ( value != 0 ) {
        state.activeButton = button;
        state.buttonDown   = true;
        state.dragged      = false;
        MouseEvent event(button,
                         modifiers,
                         MouseEventType::PRESS,
                         state.x,
                         state.y,
                         state.x,
                         state.y);
        sink.dispatchMouseEvent(event);
        return;
    }

    MouseEvent releaseEvent(button,
                            modifiers,
                            MouseEventType::RELEASE,
                            state.x,
                            state.y,
                            state.x,
                            state.y);
    sink.dispatchMouseEvent(releaseEvent);

    MouseEventType completeType =
        state.dragged ? MouseEventType::DRAGGED : MouseEventType::PRESSED;
    MouseEvent completeEvent(
        button, modifiers, completeType, state.x, state.y, state.x, state.y);
    sink.dispatchMouseEvent(completeEvent);

    state.activeButton = buttonNone;
    state.buttonDown   = false;
    state.dragged      = false;
}

/**
 * @brief 累计 Linux 鼠标相对移动增量。
 *
 * @param state 当前鼠标设备的状态。
 * @param code Linux 相对轴编号。
 * @param value 当前事件携带的轴增量。
 */
void accumulateMouseMotion(MouseState& state, unsigned short code, int value)
{
    if ( code == REL_X ) {
        state.pendingDx += value;
    } else if ( code == REL_Y ) {
        state.pendingDy += value;
    } else if ( code == REL_HWHEEL ) {
        state.pendingScrollX += value;
    } else if ( code == REL_WHEEL ) {
        state.pendingScrollY += value;
    }
#ifdef REL_HWHEEL_HI_RES
    else if ( code == REL_HWHEEL_HI_RES ) {
        state.pendingScrollX += static_cast<double>(value) / 120.0;
    }
#endif
#ifdef REL_WHEEL_HI_RES
    else if ( code == REL_WHEEL_HI_RES ) {
        state.pendingScrollY += static_cast<double>(value) / 120.0;
    }
#endif
}

/**
 * @brief 在 SYN_REPORT 到来时把累计移动转换为 MOVE 或 DRAG 事件。
 *
 * @param sink 核心事件接收接口。
 * @param state 当前鼠标设备的状态。
 */
void flushMouseMotion(InputEventSink& sink, MouseState& state)
{
    if ( state.pendingDx == 0 && state.pendingDy == 0 ) {
        return;
    }

    state.x += state.pendingDx;
    state.y += state.pendingDy;
    state.pendingDx = 0;
    state.pendingDy = 0;

    auto modifiers = snapshotModifiers();
    auto type = state.buttonDown ? MouseEventType::DRAG : MouseEventType::MOVE;
    if ( type == MouseEventType::DRAG ) {
        state.dragged = true;
    }

    MouseEvent event(state.activeButton,
                     modifiers,
                     type,
                     state.x,
                     state.y,
                     state.x,
                     state.y);
    sink.dispatchMouseEvent(event);
}

/**
 * @brief 在 SYN_REPORT 到来时把累计滚动转换为 SCROLL 事件。
 *
 * @param sink 核心事件接收接口。
 * @param state 当前鼠标设备的状态。
 */
void flushMouseScroll(InputEventSink& sink, MouseState& state)
{
    if ( state.pendingScrollX == 0.0 && state.pendingScrollY == 0.0 ) {
        return;
    }

    auto       modifiers = snapshotModifiers();
    MouseEvent event(buttonNone,
                     modifiers,
                     MouseEventType::SCROLL,
                     state.x,
                     state.y,
                     state.x,
                     state.y,
                     state.pendingScrollX,
                     state.pendingScrollY);
    state.pendingScrollX = 0.0;
    state.pendingScrollY = 0.0;
    sink.dispatchMouseEvent(event);
}

/**
 * @brief 在当前线程中持续读取并分发指定 Linux 输入设备事件。
 *
 * @param device 设备能力描述。
 * @param sink 核心事件接收接口。
 */
void listenToDevice(DeviceInfo device, InputEventSink& sink)
{
    int fd = open(device.path.c_str(), O_RDONLY | O_NONBLOCK);
    if ( fd < 0 ) {
        std::cerr << "无法打开设备 " << device.path << ": " << strerror(errno)
                  << std::endl;
        return;
    }

    MouseState         mouseState;
    struct input_event ev;
    while ( listening.load() ) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if ( n == static_cast<ssize_t>(sizeof(ev)) ) {
            if ( ev.type == EV_KEY &&
                 (ev.value == 0 || ev.value == 1 || ev.value == 2) ) {
                if ( device.keyboard && isKeyboardKey(ev.code) ) {
                    dispatchKeyEvent(sink, ev.code, ev.value);
                } else if ( device.mouse ) {
                    dispatchMouseButtonEvent(
                        sink, mouseState, ev.code, ev.value);
                }
            } else if ( device.mouse && ev.type == EV_REL ) {
                accumulateMouseMotion(mouseState, ev.code, ev.value);
            } else if ( device.mouse && ev.type == EV_SYN &&
                        ev.code == SYN_REPORT ) {
                flushMouseMotion(sink, mouseState);
                flushMouseScroll(sink, mouseState);
            }
            continue;
        }

        if ( n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if ( n < 0 ) {
            std::cerr << "读取事件失败: " << strerror(errno) << std::endl;
        }
        break;
    }

    close(fd);
}

}  // namespace

void startPlatformInputListener(InputEventSink& sink)
{
    if ( listening.exchange(true) ) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(modifiersMutex);
        globalModifiers = {};
    }

    auto devices = enumerateInputDevices();
    if ( devices.empty() ) {
        std::cerr << "未找到可监听的输入设备" << std::endl;
        listening.store(false);
        return;
    }

    std::lock_guard<std::mutex> lock(listenerThreadsMutex);
    listenerThreads.clear();
    listenerThreads.reserve(devices.size());
    for ( const auto& device : devices ) {
        listenerThreads.emplace_back(listenToDevice, device, std::ref(sink));
    }
}

void stopPlatformInputListener()
{
    if ( !listening.exchange(false) ) {
        return;
    }

    std::vector<std::thread> threads;
    {
        std::lock_guard<std::mutex> lock(listenerThreadsMutex);
        threads.swap(listenerThreads);
    }

    for ( auto& thread : threads ) {
        if ( thread.joinable() ) {
            thread.join();
        }
    }
}

}  // namespace Internal
}  // namespace InputListener
