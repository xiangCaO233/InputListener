#ifndef INPUTLISTENER_GLOBALSCREEN_H
#define INPUTLISTENER_GLOBALSCREEN_H

/**
 * @file GlobalScreen.h
 * @brief 提供跨平台全局键盘、鼠标事件监听入口。
 */

#include <InputListener/ListenerConnection.h>
#include <InputListener/listener/KeyListener.h>
#include <InputListener/listener/MouseListener.h>

namespace InputListener {

/**
 * @brief 全局输入事件监听 facade。
 *
 * GlobalScreen 负责启动或停止系统级输入监听，并把平台原生输入事件转换为
 * InputListener 的键盘、鼠标事件对象。公开接口保持跨平台，平台回调和设备枚举
 * 细节保留在库内部实现中。
 *
 * @note 当前实现维护一个进程级全局监听状态，因此所有成员函数都是静态函数。
 */
class GlobalScreen {
public:
  /**
   * @brief 注册并启动全局输入事件监听。
   *
   * macOS 上会创建事件 tap 和运行循环线程；Linux 上会枚举输入设备并启动设备
   * 监听线程。
   */
  static void registerScreenHook();

  /**
   * @brief 停止全局输入事件监听并释放平台资源。
   *
   * @note Linux 当前设备监听线程仍是分离线程，停止语义后续可继续完善。
   */
  static void unRegisterScreenHook();

  /**
   * @brief 添加键盘监听器并返回自动解绑句柄。
   *
   * @param listener 生命周期必须长于返回的 ListenerConnection。
   * @return ListenerConnection 析构或调用 disconnect() 时会自动解绑 listener。
   */
  [[nodiscard]] static ListenerConnection addKeyListener(KeyListener &listener);

  /**
   * @brief 添加鼠标监听器并返回自动解绑句柄。
   *
   * @param listener 生命周期必须长于返回的 ListenerConnection。
   * @return ListenerConnection 析构或调用 disconnect() 时会自动解绑 listener。
   */
  [[nodiscard]] static ListenerConnection
  addMouseListener(MouseListener &listener);

  /**
   * @brief 添加键盘监听器的裸指针兼容接口。
   *
   * @param listener 要注册的监听器；传入 nullptr 时内部会忽略。
   * @warning 调用方需要保证 listener 在 removeKeyListener() 之前保持有效。
   */
  static void addKeyListener(KeyListener *listener);

  /**
   * @brief 移除通过裸指针接口添加的键盘监听器。
   *
   * @param listener 要移除的监听器指针。
   */
  static void removeKeyListener(KeyListener *listener);

  /**
   * @brief 添加鼠标监听器的裸指针兼容接口。
   *
   * @param listener 要注册的监听器；传入 nullptr 时内部会忽略。
   * @warning 调用方需要保证 listener 在 removeMouseListener() 之前保持有效。
   */
  static void addMouseListener(MouseListener *listener);

  /**
   * @brief 移除通过裸指针接口添加的鼠标监听器。
   *
   * @param listener 要移除的监听器指针。
   */
  static void removeMouseListener(MouseListener *listener);
};

} // namespace InputListener

#endif // INPUTLISTENER_GLOBALSCREEN_H
