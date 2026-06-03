#pragma once

/**
 * @file PlatformInputListener.h
 * @brief 平台输入监听层的内部抽象接口。
 *
 * @warning 该头文件属于 src 内部实现，不是安装给用户的公开 API。
 */

#include <InputListener/event/KeyEvent.h>
#include <InputListener/event/MouseEvent.h>

namespace InputListener {
namespace Internal {

/**
 * @brief 平台监听层向核心分发层发送事件的接收接口。
 *
 * 平台实现只依赖该接口，不直接接触 KeyEventDispatcher 或
 * MouseEventDispatcher。这样新增 Windows 等平台时，只需要实现平台采集层。
 */
class InputEventSink {
public:
  /**
   * @brief 默认虚析构函数。
   */
  virtual ~InputEventSink() = default;

  /**
   * @brief 分发平台层转换出的键盘事件。
   *
   * @param event 跨平台键盘事件。
   */
  virtual void dispatchKeyEvent(const KeyEvent &event) = 0;

  /**
   * @brief 分发平台层转换出的鼠标事件。
   *
   * @param event 跨平台鼠标事件。
   */
  virtual void dispatchMouseEvent(const MouseEvent &event) = 0;
};

/**
 * @brief 启动当前平台的原生输入监听。
 *
 * @param sink 平台层转换出事件后发送到的核心分发接口。
 */
void startPlatformInputListener(InputEventSink &sink);

/**
 * @brief 停止当前平台的原生输入监听并释放平台资源。
 */
void stopPlatformInputListener();

} // namespace Internal
} // namespace InputListener

