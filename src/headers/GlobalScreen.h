#ifndef GLOBALSCREEN_GLOBALSCREEN_H
#define GLOBALSCREEN_GLOBALSCREEN_H

#ifdef __APPLE__
// macos所需头文件
#include "dispatcher/KeyEventDispatcher.h"
#include "dispatcher/MouseEventDispatcher.h"
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#if TARGET_OS_MAC
// macos所需声明函数
#endif // TARGET_OS_MAC

#elif defined(__linux__)
// Linux所需头文件及声明

// 其他 Linux 特定实现
#endif // defined(__linux__)

// 公共头
#include <thread>

class GlobalScreen {
  // 事件派遣器
  static KeyEventDispatcher keyEventDispatcher;
  static MouseEventDispatcher mouseEventDispatcher;
  // 循环线程
  static std::thread loopThread;
#ifdef __APPLE__
  // macos
  // api事件触
  static CFMachPortRef eventTap;
  // api循环源
  static CFRunLoopSourceRef runLoopSource;
#elif defined(__Linux__)
  // linux
#endif
public:
  // 公共
  // 注册全局钩子
  static void registerScreenHook();
  // 添加按键监听器
  static void addKeyListener(KeyListener *listener);
  // 移除按键监听器
  static void removeKeyListener(KeyListener *listener);
  // 添加鼠标监听器
  static void addMouseListener(MouseListener *listener);
  // 移除鼠标监听器
  static void removeMouseListener(MouseListener *listener);
  // 停止捕获
  static void unRegisterScreenHook();
#ifdef __APPLE__
  // macos
  // api事件回调
  static CGEventRef mcallback(CGEventTapProxy proxy, CGEventType type,
                              CGEventRef event, void *refcon);
  // 获取按下按键的字符
  static const char *mGetCharacterFromKeyCode(CGKeyCode keyCode,
                                              bool shiftPressed);
  // 发送按键事件到派遣器
  static void msendKeyEvent(CGKeyCode rawCode, const char *key,
                            KeyEventType type, Modifiers &modifiers);
  // 发送鼠标事件到派遣器
  static void msendMouseEvent(int button, MouseEventType type,
                              Modifiers &modifiers, CGPoint &location);
#elif defined(__Linux__)
  // linux
#endif
};

#endif // GLOBALSCREEN_GLOBALSCREEN_H
