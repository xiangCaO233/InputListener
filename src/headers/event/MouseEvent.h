#ifndef MOUSEEVENT_MOUSEEVENT_H
#define MOUSEEVENT_MOUSEEVENT_H

#include "KeyEvent.h"

enum class MouseEventType { PRESS, RELEASE, MOVE, DRAG, PRESSED, DRAGGED };

class MouseEvent {
  // 鼠标按键code
  int button;
  // 修饰符
  Modifiers modifiers;
  // 事件类型
  MouseEventType type;
  // 时间戳
  long long time;
  // 鼠标位置x
  int xv;
  // 鼠标位置x2d
  double xd;
  // 鼠标位置y
  int yv;
  // 鼠标位置y2d
  double yd;

public:
  MouseEvent(int b, Modifiers &m, MouseEventType t, int x, int y, double x2d,
             double y2d);
  ~MouseEvent();

  int getButton() const;

  Modifiers getModifiers() const;

  MouseEventType getType() const;

  int X() const;

  double X2D() const;

  int Y() const;

  double Y2D() const;
};

#endif // MOUSEEVENT_MOUSEEVENT_H
