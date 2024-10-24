#include "../headers/GlobalScreen.h"
#include "event/KeyEvent.h"
#include "event/MouseEvent.h"
#include <iostream>

KeyEventDispatcher GlobalScreen::keyEventDispatcher;
MouseEventDispatcher GlobalScreen::mouseEventDispatcher;

#ifdef __APPLE__
// macos所需
CFRunLoopSourceRef GlobalScreen::runLoopSource;
CFMachPortRef GlobalScreen::eventTap;
std::thread GlobalScreen::loopThread;
#elif defined(__Linux__)
// linux所需

#endif

void GlobalScreen::registerScreenHook() {
#ifdef __APPLE__
  // macos 注册事件api
  eventTap = CGEventTapCreate(
      kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
      CGEventMaskBit(kCGEventKeyDown) |               // 键盘按下
          CGEventMaskBit(kCGEventKeyUp) |             // 键盘释放
          CGEventMaskBit(kCGEventFlagsChanged) |      // 捕获修饰键变化
          CGEventMaskBit(kCGEventLeftMouseDown) |     // 鼠标左键按下
          CGEventMaskBit(kCGEventLeftMouseUp) |       // 鼠标左键释放
          CGEventMaskBit(kCGEventRightMouseDown) |    // 鼠标右键按下
          CGEventMaskBit(kCGEventRightMouseUp) |      // 鼠标右键释放
          CGEventMaskBit(kCGEventOtherMouseDown) |    // 鼠标其他键按下
          CGEventMaskBit(kCGEventOtherMouseUp) |      // 鼠标其他键释放
          CGEventMaskBit(kCGEventLeftMouseDragged) |  // 鼠标左键拖动
          CGEventMaskBit(kCGEventRightMouseDragged) | // 鼠标右键拖动
          CGEventMaskBit(kCGEventOtherMouseDragged) | // 鼠标其他键拖动
          CGEventMaskBit(kCGEventMouseMoved),         // 鼠标移动
      mcallback,                                      // 事件回调
      NULL);

  if (!eventTap) {
    return;
  }

  // 创建循环源
  runLoopSource =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
                     kCFRunLoopCommonModes);
  CGEventTapEnable(eventTap, true);
  // 创建循环线程
  loopThread = std::thread(CFRunLoopRun);
  // 分离线程
  loopThread.detach();
#elif defined(__unix__)
  // linux注册
  const char *inputDir = "/dev/input";
  DIR *dir = opendir(inputDir);
  if (dir == nullptr) {
    std::cerr << "无法打开输入设备目录: " << strerror(errno) << std::endl;
  }

  struct dirent *entry;

  std::vector<std::string> mouse_devices;
  std::vector<std::string> keyboard_devices;
  while ((entry = readdir(dir)) != nullptr) {
    // 过滤掉 . 和 .. 目录
    if (entry->d_name[0] == '.')
      continue;

    std::string devicePath = std::string(inputDir) + "/" + entry->d_name;
    if (devicePath.find("event") != std::string::npos) {
      if (isKeyboard(devicePath)) {
        std::cout << "找到键盘设备: " << devicePath << "->"
                  << getDeviceName(devicePath) << std::endl;
        keyboard_devices.push_back(std::string(devicePath));
      }
      if (isMouse(devicePath)) {
        std::cout << "找到鼠标设备: " << devicePath << "->"
                  << getDeviceName(devicePath) << std::endl;
        mouse_devices.push_back(std::string(devicePath));
      }
    }
  }
  std::string prefix = "/dev/input/event";
  printf("请选择监听的鼠标设备:[number]\n");
  std::string mousenum;
  std::getline(std::cin, mousenum);
  std::string mouse = prefix + mousenum;

  auto mit = std::find(mouse_devices.begin(), mouse_devices.end(), mouse);
  if (mit == mouse_devices.end())
    std::cerr << "不存在设备:" << mouse << std::endl;

  printf("请选择监听的键盘设备:[number]\n");
  std::string keyboardnum;
  std::getline(std::cin, keyboardnum);
  std::string keyboard = prefix + keyboardnum;
  auto kit =
      std::find(keyboard_devices.begin(), keyboard_devices.end(), keyboard);
  if (kit == keyboard_devices.end())
    std::cerr << "不存在设备:" << keyboard << std::endl;

  std::cout << "开始监听:\n"
            << "mouse:" << mouse << "\n"
            << std::endl;
  std::cout << "keyboard:" << keyboard << "\n" << std::endl;
  std::vector<std::string> devices = {keyboard, mouse};

  // 创建线程来监听每个设备
  for (const auto &device : devices) {
    std::thread t(listenToDevice, device);
    t.detach(); // 将线程设置为分离状态
  }
  closedir(dir);
#endif
  // 公共代码
}

void GlobalScreen::unRegisterScreenHook() {
#ifdef __APPLE__
  // macos
  CFRelease(runLoopSource);
  CFRelease(eventTap);
#elif defined(__unix__)
  // linux

#endif
}

#ifdef __APPLE__
// macos
CGEventRef GlobalScreen::mcallback(CGEventTapProxy proxy, CGEventType type,
                                   CGEventRef event, void *refcon) {
  // 获取按键码
  CGKeyCode keyCode =
      (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  // std::cout << keyCode << std::endl;
  // 获取原生修饰符
  CGEventFlags flags = (CGEventFlags)CGEventGetFlags(event);
  // 获取字符
  auto ks = mGetCharacterFromKeyCode(keyCode, flags & kCGEventFlagMaskShift);
  // auto ks = "test";
  //  获取鼠标位置
  CGPoint mouseLocation = CGEventGetLocation(event);
  // 初始化修饰符
  Modifiers modifiers;
  modifiers.shift = flags & kCGEventFlagMaskShift;
  modifiers.command = flags & kCGEventFlagMaskCommand;
  modifiers.control = flags & kCGEventFlagMaskControl;
  modifiers.option = flags & kCGEventFlagMaskAlternate;
  if (type == kCGEventMouseMoved) {
    // 鼠标运动
    msendMouseEvent(0, MouseEventType::MOVE, modifiers, mouseLocation);
  } else if (type == kCGEventLeftMouseDragged) {
    msendMouseEvent(1, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseDragged) {
    msendMouseEvent(2, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseDragged) {
    msendMouseEvent(3, MouseEventType::DRAG, modifiers, mouseLocation);
  }
  // 处理键盘事件
  else if (type == kCGEventKeyDown) {
    // 按键按下
    // std::cout << "按键按下:" << keyCode << std::endl;
    msendKeyEvent(keyCode, ks, KeyEventType::PRESS, modifiers);
  } else if (type == kCGEventKeyUp) {
    // 按键释放
    msendKeyEvent(keyCode, ks, KeyEventType::RELEASE, modifiers);
    msendKeyEvent(keyCode, ks, KeyEventType::PRESSED, modifiers);
  } else if (type == kCGEventFlagsChanged) {
    //    std::cout << "修饰符变化" << std::endl;

    // 检查 Shift 键
    if (flags & kCGEventFlagMaskShift) {
      msendKeyEvent(keyCode, ks, KeyEventType::PRESS, modifiers);
    } else {
      msendKeyEvent(keyCode, ks, KeyEventType::RELEASE, modifiers);
      msendKeyEvent(keyCode, ks, KeyEventType::PRESSED, modifiers);
    }

    // 检查 Control 键
    if (flags & kCGEventFlagMaskControl) {
      msendKeyEvent(keyCode, ks, KeyEventType::PRESS, modifiers);
    } else {
      msendKeyEvent(keyCode, ks, KeyEventType::RELEASE, modifiers);
      msendKeyEvent(keyCode, ks, KeyEventType::PRESSED, modifiers);
    }

    // 检查 Option 键
    if (flags & kCGEventFlagMaskAlternate) {
      msendKeyEvent(keyCode, ks, KeyEventType::PRESS, modifiers);
    } else {
      msendKeyEvent(keyCode, ks, KeyEventType::RELEASE, modifiers);
      msendKeyEvent(keyCode, ks, KeyEventType::PRESSED, modifiers);
    }

    // 检查 Command 键
    if (flags & kCGEventFlagMaskCommand) {
      msendKeyEvent(keyCode, ks, KeyEventType::PRESS, modifiers);
    } else {
      msendKeyEvent(keyCode, ks, KeyEventType::RELEASE, modifiers);
      msendKeyEvent(keyCode, ks, KeyEventType::PRESSED, modifiers);
    }
  }
  // 处理鼠标事件
  else if (type == kCGEventLeftMouseDown) {
    msendMouseEvent(1, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventLeftMouseUp) {
    msendMouseEvent(1, MouseEventType::RELEASE, modifiers, mouseLocation);
    msendMouseEvent(1, MouseEventType::PRESSED, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseDown) {
    msendMouseEvent(2, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseUp) {
    msendMouseEvent(1, MouseEventType::RELEASE, modifiers, mouseLocation);
    msendMouseEvent(1, MouseEventType::PRESSED, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseDown) {
    msendMouseEvent(3, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseUp) {
    msendMouseEvent(3, MouseEventType::RELEASE, modifiers, mouseLocation);
    msendMouseEvent(3, MouseEventType::PRESSED, modifiers, mouseLocation);
  }
  // 返回事件，允许事件继续传播
  return event;
}

const char *GlobalScreen::mGetCharacterFromKeyCode(CGKeyCode keycode,
                                                   bool shiftPressed) {
  if (shiftPressed) {
    return Mapping::shiftKeyMap.at(keycode);
  } else {
    return Mapping::normalKeyMap.at(keycode);
  }
}

void GlobalScreen::msendKeyEvent(CGKeyCode rawCode, const char *key,
                                 KeyEventType type, Modifiers &modifiers) {

  auto e = new KeyEvent(rawCode, key, modifiers, type);
  keyEventDispatcher.dispachEvent(*e);
}

void GlobalScreen::msendMouseEvent(int button, MouseEventType type,
                                   Modifiers &modifiers, CGPoint &location) {
  auto e = new MouseEvent(button, modifiers, type, (int)location.x,
                          (int)location.y, location.x, location.y);
  mouseEventDispatcher.dispatchEvent(*e);
}
#elif defined(__unix__)
// linux
// 获取设备名称
std::string GlobalScreen::getDeviceName(const std::string &devicePath) {
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
};
// 检查是否是键盘
bool GlobalScreen::isKeyboard(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return false;
  }

  // 获取设备支持的事件类型
  unsigned long evbit[EV_MAX / 8 + 1];
  memset(evbit, 0, sizeof(evbit));
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit) < 0) {
    std::cerr << "无法获取事件位图: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }

  close(fd);

  // 检查是否是键盘（是否支持 EV_KEY 事件）
  bool hasKeyEvents = evbit[EV_KEY / 8] & (1 << (EV_KEY % 8));
  return hasKeyEvents;
};
// 检查是否是鼠标
bool GlobalScreen::isMouse(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return false;
  }

  // 获取设备支持的事件类型
  unsigned long evbit[EV_MAX / 8 + 1];
  memset(evbit, 0, sizeof(evbit));
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit) < 0) {
    std::cerr << "无法获取事件位图: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }

  close(fd);

  // 检查是否是鼠标（是否支持 EV_REL 事件，用于相对运动检测）
  bool hasRelativeMovement = evbit[EV_REL / 8] & (1 << (EV_REL % 8));
  return hasRelativeMovement;
};
// 开始监听指定设备
void GlobalScreen::listenToDevice(const std::string &devicePath) {
  int fd = open(devicePath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "无法打开设备 " << devicePath << ": " << strerror(errno)
              << std::endl;
    return;
  }
  struct input_event ev;
  while (true) {
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == (ssize_t)sizeof(ev)) {
      std::cout << "设备: " << devicePath << " 时间戳: " << ev.time.tv_sec
                << "." << ev.time.tv_usec << " 类型: " << ev.type
                << " 代码: " << ev.code << " 值: " << ev.value << std::endl;
    } else {
      std::cerr << "读取事件失败: " << strerror(errno) << std::endl;
      break;
    }
  }

  close(fd);
};
#endif

// 公共
void GlobalScreen::addKeyListener(KeyListener *listener) {
  keyEventDispatcher.addListener(listener);
}

void GlobalScreen::removeKeyListener(KeyListener *listener) {
  keyEventDispatcher.removeListener(listener);
}

void GlobalScreen::addMouseListener(MouseListener *listener) {
  mouseEventDispatcher.addListener(listener);
}

void GlobalScreen::removeMouseListener(MouseListener *listener) {
  mouseEventDispatcher.removeListener(listener);
}
