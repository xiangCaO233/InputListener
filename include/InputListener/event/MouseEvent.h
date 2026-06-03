#pragma once

/**
 * @file MouseEvent.h
 * @brief 定义鼠标事件数据结构。
 */

#include <InputListener/event/KeyEvent.h>

namespace InputListener {

/**
 * @brief 鼠标事件类型。
 */
enum class MouseEventType {
  PRESS,   ///< 鼠标按键被按下。
  RELEASE, ///< 鼠标按键被释放。
  MOVE,    ///< 鼠标移动且没有拖拽。
  DRAG,    ///< 鼠标按键按下时移动。
  SCROLL,  ///< 鼠标滚轮或触控板滚动。
  PRESSED, ///< 一次完整点击动作结束后的完成事件。
  DRAGGED  ///< 一次完整拖拽动作结束后的完成事件。
};

/**
 * @brief 平台原生鼠标事件转换后的跨平台鼠标事件。
 */
class MouseEvent {
  int button;             ///< 鼠标按键编号；移动事件通常为 0。
  Modifiers modifiers;    ///< 事件发生时的修饰键状态。
  MouseEventType type;    ///< 当前鼠标事件类型。
  int xv;                 ///< 鼠标位置的整数 x 坐标。
  double xd;              ///< 鼠标位置的浮点 x 坐标。
  int yv;                 ///< 鼠标位置的整数 y 坐标。
  double yd;              ///< 鼠标位置的浮点 y 坐标。
  double scrollDeltaX;    ///< 水平滚动增量。
  double scrollDeltaY;    ///< 垂直滚动增量。

public:
  /**
   * @brief 构造鼠标事件。
   *
   * @param b 鼠标按键编号。
   * @param m 事件发生时的修饰键状态。
   * @param t 鼠标事件类型。
   * @param x 整数 x 坐标。
   * @param y 整数 y 坐标。
   * @param x2d 浮点 x 坐标。
   * @param y2d 浮点 y 坐标。
   * @param sx 水平滚动增量。
   * @param sy 垂直滚动增量。
   */
  MouseEvent(int b, const Modifiers &m, MouseEventType t, int x, int y,
             double x2d, double y2d, double sx = 0.0, double sy = 0.0);

  /**
   * @brief 销毁鼠标事件。
   */
  ~MouseEvent();

  /**
   * @brief 获取鼠标按键编号。
   *
   * @return 鼠标按键编号。
   */
  int getButton() const;

  /**
   * @brief 获取事件发生时的修饰键状态。
   *
   * @return 修饰键状态值。
   */
  Modifiers getModifiers() const;

  /**
   * @brief 获取鼠标事件类型。
   *
   * @return 鼠标事件类型。
   */
  MouseEventType getType() const;

  /**
   * @brief 获取整数 x 坐标。
   *
   * @return 整数 x 坐标。
   */
  int X() const;

  /**
   * @brief 获取浮点 x 坐标。
   *
   * @return 浮点 x 坐标。
   */
  double X2D() const;

  /**
   * @brief 获取整数 y 坐标。
   *
   * @return 整数 y 坐标。
   */
  int Y() const;

  /**
   * @brief 获取浮点 y 坐标。
   *
   * @return 浮点 y 坐标。
   */
  double Y2D() const;

  /**
   * @brief 获取水平滚动增量。
   *
   * @return 水平滚动增量；非滚动事件通常为 0。
   */
  double getScrollDeltaX() const;

  /**
   * @brief 获取垂直滚动增量。
   *
   * @return 垂直滚动增量；非滚动事件通常为 0。
   */
  double getScrollDeltaY() const;
};

} // namespace InputListener
