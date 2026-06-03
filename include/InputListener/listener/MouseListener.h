#pragma once

/**
 * @file MouseListener.h
 * @brief 定义鼠标事件监听器接口。
 */

#include <InputListener/event/MouseEvent.h>

namespace InputListener {

/**
 * @brief 接收鼠标事件的可继承监听器接口。
 *
 * 所有回调默认空实现，使用方只需要覆写自己关心的事件。
 */
class MouseListener {
public:
  /**
   * @brief 虚析构函数，保证通过基类指针销毁派生类对象时行为正确。
   */
  virtual ~MouseListener() = default;

  /**
   * @brief 鼠标按键按下时触发。
   *
   * @param e 鼠标按下事件。
   */
  virtual void onPress(const MouseEvent &e);

  /**
   * @brief 鼠标按键释放时触发。
   *
   * @param e 鼠标释放事件。
   */
  virtual void onRelease(const MouseEvent &e);

  /**
   * @brief 鼠标移动时触发。
   *
   * @param e 鼠标移动事件。
   */
  virtual void onMove(const MouseEvent &e);

  /**
   * @brief 鼠标按键按下并移动时触发。
   *
   * @param e 鼠标拖动事件。
   */
  virtual void onDrag(const MouseEvent &e);

  /**
   * @brief 鼠标滚轮或触控板滚动时触发。
   *
   * @param e 鼠标滚动事件。
   */
  virtual void onScroll(const MouseEvent &e);

  /**
   * @brief 一次鼠标点击动作完成后触发。
   *
   * @param e 鼠标点击完成事件。
   */
  virtual void onPressed(const MouseEvent &e);

  /**
   * @brief 一次鼠标拖动动作完成后触发。
   *
   * @param e 鼠标拖动完成事件。
   */
  virtual void onDragged(const MouseEvent &e);
};

} // namespace InputListener
