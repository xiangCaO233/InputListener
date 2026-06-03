#ifndef MOUSELISTENER_MOUSELISTENER_H
#define MOUSELISTENER_MOUSELISTENER_H

#include <InputListener/event/MouseEvent.h>

namespace InputListener {

class MouseListener {
public:
  virtual ~MouseListener() = default;

  // 鼠标按下
  virtual void onPress(const MouseEvent &e);
  // 鼠标释放
  virtual void onRelease(const MouseEvent &e);
  // 鼠标移动
  virtual void onMove(const MouseEvent &e);
  // 鼠标拖动
  virtual void onDrag(const MouseEvent &e);
  // 鼠标按完
  virtual void onPressed(const MouseEvent &e);
  // 鼠标拖动完成
  virtual void onDragged(const MouseEvent &e);
};

} // namespace InputListener

#endif // MOUSELISTENER_MOUSELISTENER_H
