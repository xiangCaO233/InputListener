#include "PlatformInputListener.h"

#include "KeyMapping.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <thread>

namespace InputListener {
namespace Internal {
namespace {

CFRunLoopSourceRef runLoopSource = nullptr; ///< macOS 事件 tap 的运行循环源。
CFMachPortRef eventTap = nullptr;           ///< macOS 全局事件 tap 句柄。
std::thread loopThread;                     ///< macOS 事件循环线程。

/**
 * @brief 构造并分发键盘事件。
 *
 * @param sink 核心事件接收接口。
 * @param rawCode macOS 原始键码。
 * @param key 键码映射得到的文本；允许为 nullptr。
 * @param type InputListener 键盘事件类型。
 * @param modifiers 事件发生时的修饰键状态。
 */
void sendKeyEvent(InputEventSink &sink, int rawCode, const char *key,
                  KeyEventType type, const Modifiers &modifiers) {
  KeyEvent event(rawCode, key == nullptr ? "" : key, modifiers, type);
  sink.dispatchKeyEvent(event);
}

/**
 * @brief 构造并分发鼠标事件。
 *
 * @param sink 核心事件接收接口。
 * @param button 鼠标按键编号。
 * @param type InputListener 鼠标事件类型。
 * @param modifiers 事件发生时的修饰键状态。
 * @param location macOS 鼠标坐标。
 */
void sendMouseEvent(InputEventSink &sink, int button, MouseEventType type,
                    const Modifiers &modifiers, CGPoint location,
                    double scrollX = 0.0, double scrollY = 0.0) {
  MouseEvent event(button, modifiers, type, static_cast<int>(location.x),
                   static_cast<int>(location.y), location.x, location.y,
                   scrollX, scrollY);
  sink.dispatchMouseEvent(event);
}

/**
 * @brief macOS CGEventTap 回调，负责把原生事件转换为库事件。
 *
 * @param proxy macOS 事件 tap 代理对象；当前实现不需要使用。
 * @param type macOS 原生事件类型。
 * @param event macOS 原生事件对象。
 * @param refcon 指向 InputEventSink 的用户上下文指针。
 * @return 原始事件对象，允许系统继续处理该事件。
 */
CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type,
                         CGEventRef event, void *refcon) {
  (void)proxy;
  auto *sink = static_cast<InputEventSink *>(refcon);
  if (sink == nullptr) {
    return event;
  }

  CGKeyCode keyCode = static_cast<CGKeyCode>(
      CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
  CGEventFlags flags = static_cast<CGEventFlags>(CGEventGetFlags(event));
  const char *key =
      keyTextForCode(keyCode, flags & kCGEventFlagMaskShift);
  CGPoint mouseLocation = CGEventGetLocation(event);

  Modifiers modifiers;
  modifiers.shift = flags & kCGEventFlagMaskShift;
  modifiers.command = flags & kCGEventFlagMaskCommand;
  modifiers.control = flags & kCGEventFlagMaskControl;
  modifiers.option = flags & kCGEventFlagMaskAlternate;

  if (type == kCGEventMouseMoved) {
    sendMouseEvent(*sink, 0, MouseEventType::MOVE, modifiers, mouseLocation);
  } else if (type == kCGEventLeftMouseDragged) {
    sendMouseEvent(*sink, 1, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseDragged) {
    sendMouseEvent(*sink, 2, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseDragged) {
    sendMouseEvent(*sink, 3, MouseEventType::DRAG, modifiers, mouseLocation);
  } else if (type == kCGEventScrollWheel) {
    auto vertical =
        CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);
    auto horizontal =
        CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2);
    sendMouseEvent(*sink, 0, MouseEventType::SCROLL, modifiers, mouseLocation,
                   static_cast<double>(horizontal),
                   static_cast<double>(vertical));
  } else if (type == kCGEventKeyDown) {
    auto isRepeat = CGEventGetIntegerValueField(
                        event, kCGKeyboardEventAutorepeat) != 0;
    sendKeyEvent(*sink, keyCode, key,
                 isRepeat ? KeyEventType::REPEAT : KeyEventType::PRESS,
                 modifiers);
  } else if (type == kCGEventKeyUp) {
    sendKeyEvent(*sink, keyCode, key, KeyEventType::RELEASE, modifiers);
    sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESSED, modifiers);
  } else if (type == kCGEventFlagsChanged) {
    if (flags & kCGEventFlagMaskShift) {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESSED, modifiers);
    }

    if (flags & kCGEventFlagMaskControl) {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESSED, modifiers);
    }

    if (flags & kCGEventFlagMaskAlternate) {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESSED, modifiers);
    }

    if (flags & kCGEventFlagMaskCommand) {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESS, modifiers);
    } else {
      sendKeyEvent(*sink, keyCode, key, KeyEventType::RELEASE, modifiers);
      sendKeyEvent(*sink, keyCode, key, KeyEventType::PRESSED, modifiers);
    }
  } else if (type == kCGEventLeftMouseDown) {
    sendMouseEvent(*sink, 1, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventLeftMouseUp) {
    sendMouseEvent(*sink, 1, MouseEventType::RELEASE, modifiers,
                   mouseLocation);
    sendMouseEvent(*sink, 1, MouseEventType::PRESSED, modifiers,
                   mouseLocation);
  } else if (type == kCGEventRightMouseDown) {
    sendMouseEvent(*sink, 2, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventRightMouseUp) {
    sendMouseEvent(*sink, 2, MouseEventType::RELEASE, modifiers,
                   mouseLocation);
    sendMouseEvent(*sink, 2, MouseEventType::PRESSED, modifiers,
                   mouseLocation);
  } else if (type == kCGEventOtherMouseDown) {
    sendMouseEvent(*sink, 3, MouseEventType::PRESS, modifiers, mouseLocation);
  } else if (type == kCGEventOtherMouseUp) {
    sendMouseEvent(*sink, 3, MouseEventType::RELEASE, modifiers,
                   mouseLocation);
    sendMouseEvent(*sink, 3, MouseEventType::PRESSED, modifiers,
                   mouseLocation);
  }

  return event;
}

} // namespace

void startPlatformInputListener(InputEventSink &sink) {
  if (eventTap != nullptr) {
    return;
  }

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
          CGEventMaskBit(kCGEventMouseMoved) |
          CGEventMaskBit(kCGEventScrollWheel),
      eventCallback, &sink);

  if (eventTap == nullptr) {
    return;
  }

  runLoopSource =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
                     kCFRunLoopCommonModes);
  CGEventTapEnable(eventTap, true);
  loopThread = std::thread(CFRunLoopRun);
  loopThread.detach();
}

void stopPlatformInputListener() {
  if (runLoopSource != nullptr) {
    CFRelease(runLoopSource);
    runLoopSource = nullptr;
  }
  if (eventTap != nullptr) {
    CFRelease(eventTap);
    eventTap = nullptr;
  }
}

} // namespace Internal
} // namespace InputListener
