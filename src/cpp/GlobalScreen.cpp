#include "../headers/GlobalScreen.h"
#include "event/KeyEvent.h"
#include "event/MouseEvent.h"
#include <iostream>

KeyEventDispatcher GlobalScreen::keyEventDispatcher;
MouseEventDispatcher GlobalScreen::mouseEventDispatcher;

std::thread GlobalScreen::loopThread;

#ifdef __APPLE__
// macos所需
CFRunLoopSourceRef GlobalScreen::runLoopSource;
CFMachPortRef GlobalScreen::eventTap;
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
#elif defined(__Linux__)
  // linux注册

#endif

  // 公共代码

  // 分离线程
  loopThread.detach();
}

void GlobalScreen::unRegisterScreenHook() {
#ifdef __APPLE__
  // macos
  CFRelease(runLoopSource);
  CFRelease(eventTap);
#elif defined(__Linux__)
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
#elif defined(__Linux__)
// linux

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
