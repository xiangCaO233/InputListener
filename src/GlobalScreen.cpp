#include <InputListener/GlobalScreen.h>
#include <InputListener/event/KeyEvent.h>
#include <InputListener/event/MouseEvent.h>

/**
 * @file GlobalScreen.cpp
 * @brief GlobalScreen facade 的平台相关实现。
 */

#include "KeyMapping.h"
#include "dispatcher/KeyEventDispatcher.h"
#include "dispatcher/MouseEventDispatcher.h"

#include <iostream>
#include <thread>
#include <vector>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined(__unix__)
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace InputListener {
namespace {

KeyEventDispatcher keyEventDispatcher;     ///< 进程级键盘事件分发器。
MouseEventDispatcher mouseEventDispatcher; ///< 进程级鼠标事件分发器。
std::thread loopThread;                    ///< macOS 事件循环线程。

#ifdef __APPLE__
CFRunLoopSourceRef runLoopSource = nullptr; ///< macOS 事件 tap 的运行循环源。
CFMachPortRef eventTap = nullptr;           ///< macOS 全局事件 tap 句柄。

/**
 * @brief 构造并分发键盘事件。
 *
 * @param rawCode macOS 原始键码。
 * @param key 键码映射得到的文本；允许为 nullptr。
 * @param type InputListener 键盘事件类型。
 * @param modifiers 事件发生时的修饰键状态。
 */
void sendKeyEvent(int rawCode, const char *key, KeyEventType type,
                  const Modifiers &modifiers) {
  KeyEvent event(rawCode, key == nullptr ? "" : key, modifiers, type);
  keyEventDispatcher.dispatchEvent(event);
}

/**
 * @brief 构造并分发鼠标事件。
 *
 * @param button 鼠标按键编号。
 * @param type InputListener 鼠标事件类型。
 * @param modifiers 事件发生时的修饰键状态。
 * @param location macOS 鼠标坐标。
 */
void sendMouseEvent(int button, MouseEventType type, const Modifiers &modifiers,
                    CGPoint location) {
  MouseEvent event(button, modifiers, type, static_cast<int>(location.x),
                   static_cast<int>(location.y), location.x, location.y);
  mouseEventDispatcher.dispatchEvent(event);
}

/**
 * @brief macOS CGEventTap 回调，负责把原生事件转换为库事件。
 *
 * @param proxy macOS 事件 tap 代理对象；当前实现不需要使用。
 * @param type macOS 原生事件类型。
 * @param event macOS 原生事件对象。
 * @param refcon 用户上下文指针；当前实现不需要使用。
 * @return 原始事件对象，允许系统继续处理该事件。
 */
CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type,
                         CGEventRef event, void *refcon) {
  (void)proxy;
  (void)refcon;
  CGKeyCode keyCode = static_cast<CGKeyCode>(
      CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
  CGEventFlags flags = static_cast<CGEventFlags>(CGEventGetFlags(event));
  const char *key =
      Internal::keyTextForCode(keyCode, flags & kCGEventFlagMaskShift);
  CGPoint mouseLocation = CGEventGetLocation(event);

  Modifiers modifiers;
  modifiers.shift = flags & kCGEventFlagMaskShift;
  modifiers.command = flags & kCGEventFlagMaskCommand;
  modifiers.control = flags & kCGEventFlagMaskControl;
  modifiers.option = flags & kCGEventFlagMaskAlternate;

  if (type == kCGEventMouseMoved) {
    sendMouseEvent(0, MouseEventType::MOVE, modifiers, mouseLocation);
  } else if (type == kCGEventLeftMouseDragged) {
    sendMouseEvent(1, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseDragged) {
    sendMouseEvent(2, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseDragged) {
    sendMouseEvent(3, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventKeyDown) {
    auto isRepeat = CGEventGetIntegerValueField(
                        event, kCGKeyboardEventAutorepeat) != 0;
    sendKeyEvent(keyCode, key,
                 isRepeat ? KeyEventType::REPEAT : KeyEventType::PRESS,
                 modifiers);
  } else if (type == kCGEventKeyUp) {
    sendKeyEvent(keyCode, key, KeyEventType::RELEASE, modifiers);
    sendKeyEvent(keyCode, key, KeyEventType::PRESSED, modifiers);
  } else if (type == kCGEventFlagsChanged) {
    if (flags & kCGEventFlagMaskShift) {
      sendKeyEvent(keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(keyCode, key, KeyEventType::PRESSED, modifiers);
    }

    if (flags & kCGEventFlagMaskControl) {
      sendKeyEvent(keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(keyCode, key, KeyEventType::PRESSED, modifiers);
    }

    if (flags & kCGEventFlagMaskAlternate) {
      sendKeyEvent(keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(keyCode, key, KeyEventType::PRESSED, modifiers);
    }

    if (flags & kCGEventFlagMaskCommand) {
      sendKeyEvent(keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(keyCode, key, KeyEventType::PRESSED, modifiers);
    }
  } else if (type == kCGEventLeftMouseDown) {
    sendMouseEvent(1, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventLeftMouseUp) {
    sendMouseEvent(1, MouseEventType::RELEASE, modifiers, mouseLocation);
    sendMouseEvent(1, MouseEventType::PRESSED, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseDown) {
    sendMouseEvent(2, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseUp) {
    sendMouseEvent(2, MouseEventType::RELEASE, modifiers, mouseLocation);
    sendMouseEvent(2, MouseEventType::PRESSED, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseDown) {
    sendMouseEvent(3, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseUp) {
    sendMouseEvent(3, MouseEventType::RELEASE, modifiers, mouseLocation);
    sendMouseEvent(3, MouseEventType::PRESSED, modifiers, mouseLocation);
  }

  return event;
}
#elif defined(__unix__)
/**
 * @brief 读取 Linux 输入设备的人类可读名称。
 *
 * @param devicePath /dev/input/event* 设备路径。
 * @return 设备名称；读取失败时返回“未知设备”。
 */
std::string getDeviceName(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return "未知设备";
  }

  char name[256] = "Unknown";
  if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
    std::cerr << "无法获取设备名称: " << strerror(errno) << std::endl;
    close(fd);
    return "未知设备";
  }

  close(fd);
  return std::string(name);
}

/**
 * @brief 判断 Linux 输入设备是否支持键盘事件。
 *
 * @param devicePath /dev/input/event* 设备路径。
 * @return 支持 EV_KEY 时返回 true。
 */
bool isKeyboard(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return false;
  }

  unsigned long evbit[EV_MAX / 8 + 1];
  memset(evbit, 0, sizeof(evbit));
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit) < 0) {
    std::cerr << "无法获取事件位图: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }

  close(fd);
  return evbit[EV_KEY / 8] & (1 << (EV_KEY % 8));
}

/**
 * @brief 判断 Linux 输入设备是否支持相对移动事件。
 *
 * @param devicePath /dev/input/event* 设备路径。
 * @return 支持 EV_REL 时返回 true。
 */
bool isMouse(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return false;
  }

  unsigned long evbit[EV_MAX / 8 + 1];
  memset(evbit, 0, sizeof(evbit));
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit) < 0) {
    std::cerr << "无法获取事件位图: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }

  close(fd);
  return evbit[EV_REL / 8] & (1 << (EV_REL % 8));
}

/**
 * @brief 根据按键事件更新修饰键状态。
 *
 * @param modifiers 当前设备线程维护的修饰键状态。
 * @param code Linux 输入子系统键码。
 * @param value Linux 按键值：0 表示释放，1 表示按下，2 表示重复。
 */
void updateModifiers(Modifiers &modifiers, unsigned short code, int value) {
  bool pressed = value != 0;
  switch (code) {
  case KEY_LEFTSHIFT:
  case KEY_RIGHTSHIFT:
    modifiers.shift = pressed;
    break;
  case KEY_LEFTCTRL:
  case KEY_RIGHTCTRL:
    modifiers.control = pressed;
    break;
  case KEY_LEFTALT:
  case KEY_RIGHTALT:
    modifiers.option = pressed;
    break;
  case KEY_LEFTMETA:
  case KEY_RIGHTMETA:
    modifiers.command = pressed;
    break;
  default:
    break;
  }
}

/**
 * @brief 把 Linux EV_KEY 事件转换为 KeyEvent 并分发。
 *
 * value 为 0 时分发 RELEASE 和 PRESSED，value 为 1 时分发 PRESS，
 * value 为 2 时分发 REPEAT。
 *
 * @param code Linux 输入子系统键码。
 * @param value Linux 按键值。
 * @param modifiers 当前设备线程维护的修饰键状态。
 */
void dispatchKeyEvent(unsigned short code, int value, Modifiers &modifiers) {
  updateModifiers(modifiers, code, value);

  KeyEventType type = KeyEventType::PRESS;
  if (value == 0) {
    type = KeyEventType::RELEASE;
  } else if (value == 2) {
    type = KeyEventType::REPEAT;
  }

  KeyEvent event(code, Internal::keyTextForCode(code, modifiers.shift),
                 modifiers, type);
  keyEventDispatcher.dispatchEvent(event);

  if (type == KeyEventType::RELEASE) {
    KeyEvent pressedEvent(code, event.getKey(), modifiers,
                          KeyEventType::PRESSED);
    keyEventDispatcher.dispatchEvent(pressedEvent);
  }
}

/**
 * @brief 在当前线程中持续读取并分发指定 Linux 输入设备事件。
 *
 * @param devicePath /dev/input/event* 设备路径。
 */
void listenToDevice(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return;
  }

  Modifiers modifiers;
  struct input_event ev;
  while (true) {
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == static_cast<ssize_t>(sizeof(ev))) {
      if (ev.type == EV_KEY && (ev.value == 0 || ev.value == 1 ||
                                ev.value == 2)) {
        dispatchKeyEvent(ev.code, ev.value, modifiers);
      }
    } else {
      std::cerr << "读取事件失败: " << strerror(errno) << std::endl;
      break;
    }
  }

  close(fd);
}
#endif

} // namespace

/**
 * @brief 启动平台全局输入监听。
 *
 * macOS 分支创建 CGEventTap 并启动 CFRunLoop；Linux 分支枚举
 * /dev/input/event* 设备并为选择的键盘、鼠标设备启动监听线程。
 */
void GlobalScreen::registerScreenHook() {
#ifdef __APPLE__
  eventTap = CGEventTapCreate(
      kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
      CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) |
          CGEventMaskBit(kCGEventFlagsChanged) |
          CGEventMaskBit(kCGEventLeftMouseDown) |
          CGEventMaskBit(kCGEventLeftMouseUp) |
          CGEventMaskBit(kCGEventRightMouseDown) |
          CGEventMaskBit(kCGEventRightMouseUp) |
          CGEventMaskBit(kCGEventOtherMouseDown) |
          CGEventMaskBit(kCGEventOtherMouseUp) |
          CGEventMaskBit(kCGEventLeftMouseDragged) |
          CGEventMaskBit(kCGEventRightMouseDragged) |
          CGEventMaskBit(kCGEventOtherMouseDragged) |
          CGEventMaskBit(kCGEventMouseMoved),
      eventCallback, nullptr);

  if (!eventTap) {
    return;
  }

  runLoopSource =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
                     kCFRunLoopCommonModes);
  CGEventTapEnable(eventTap, true);
  loopThread = std::thread(CFRunLoopRun);
  loopThread.detach();
#elif defined(__unix__)
  const char *inputDir = "/dev/input";
  DIR *dir = opendir(inputDir);
  if (dir == nullptr) {
    std::cerr << "无法打开输入设备目录: " << strerror(errno) << std::endl;
    return;
  }

  std::vector<std::string> mouseDevices;
  std::vector<std::string> keyboardDevices;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    std::string devicePath = std::string(inputDir) + "/" + entry->d_name;
    if (devicePath.find("event") == std::string::npos) {
      continue;
    }

    if (isKeyboard(devicePath)) {
      std::cout << "找到键盘设备: " << devicePath << "->"
                << getDeviceName(devicePath) << std::endl;
      keyboardDevices.push_back(devicePath);
    }
    if (isMouse(devicePath)) {
      std::cout << "找到鼠标设备: " << devicePath << "->"
                << getDeviceName(devicePath) << std::endl;
      mouseDevices.push_back(devicePath);
    }
  }

  std::string prefix = "/dev/input/event";
  printf("请选择监听的鼠标设备:[number]\n");
  std::string mouseNumber;
  std::getline(std::cin, mouseNumber);
  std::string mouse = prefix + mouseNumber;

  if (std::find(mouseDevices.begin(), mouseDevices.end(), mouse) ==
      mouseDevices.end()) {
    std::cerr << "不存在设备:" << mouse << std::endl;
  }

  printf("请选择监听的键盘设备:[number]\n");
  std::string keyboardNumber;
  std::getline(std::cin, keyboardNumber);
  std::string keyboard = prefix + keyboardNumber;

  if (std::find(keyboardDevices.begin(), keyboardDevices.end(), keyboard) ==
      keyboardDevices.end()) {
    std::cerr << "不存在设备:" << keyboard << std::endl;
  }

  std::cout << "开始监听:\nmouse:" << mouse << "\nkeyboard:" << keyboard
            << "\n";
  std::vector<std::string> devices = {keyboard, mouse};

  for (const auto &device : devices) {
    std::thread t(listenToDevice, device);
    t.detach();
  }

  closedir(dir);
#endif
}

/**
 * @brief 释放平台监听资源。
 *
 * macOS 分支释放事件 tap 与 run loop source；Linux 停止语义当前尚未实现。
 */
void GlobalScreen::unRegisterScreenHook() {
#ifdef __APPLE__
  if (runLoopSource != nullptr) {
    CFRelease(runLoopSource);
    runLoopSource = nullptr;
  }
  if (eventTap != nullptr) {
    CFRelease(eventTap);
    eventTap = nullptr;
  }
#endif
}

/**
 * @brief 注册键盘监听器并返回自动解绑连接。
 */
ListenerConnection GlobalScreen::addKeyListener(KeyListener &listener) {
  addKeyListener(&listener);
  auto listenerPtr = &listener;
  return ListenerConnection(
      [listenerPtr] { GlobalScreen::removeKeyListener(listenerPtr); });
}

/**
 * @brief 注册鼠标监听器并返回自动解绑连接。
 */
ListenerConnection GlobalScreen::addMouseListener(MouseListener &listener) {
  addMouseListener(&listener);
  auto listenerPtr = &listener;
  return ListenerConnection(
      [listenerPtr] { GlobalScreen::removeMouseListener(listenerPtr); });
}

/**
 * @brief 通过裸指针注册键盘监听器的兼容入口。
 */
void GlobalScreen::addKeyListener(KeyListener *listener) {
  keyEventDispatcher.addListener(listener);
}

/**
 * @brief 移除通过裸指针注册的键盘监听器。
 */
void GlobalScreen::removeKeyListener(KeyListener *listener) {
  keyEventDispatcher.removeListener(listener);
}

/**
 * @brief 通过裸指针注册鼠标监听器的兼容入口。
 */
void GlobalScreen::addMouseListener(MouseListener *listener) {
  mouseEventDispatcher.addListener(listener);
}

/**
 * @brief 移除通过裸指针注册的鼠标监听器。
 */
void GlobalScreen::removeMouseListener(MouseListener *listener) {
  mouseEventDispatcher.removeListener(listener);
}

} // namespace InputListener
