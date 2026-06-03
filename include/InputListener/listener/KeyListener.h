#ifndef KEYLISTENER_KEYLISTENER_H
#define KEYLISTENER_KEYLISTENER_H

#include <InputListener/event/KeyEvent.h>

namespace InputListener {

class KeyListener {
public:
  virtual ~KeyListener() = default;

  // 按下事件
  virtual void onPress(const KeyEvent &e);
  // 释放事件
  virtual void onRelease(const KeyEvent &e);
  // 按完事件
  virtual void onPressed(const KeyEvent &e);
};

} // namespace InputListener

#endif // KEYLISTENER_KEYLISTENER_H
