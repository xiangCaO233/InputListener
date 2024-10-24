#ifndef KEYLISTENER_KEYLISTENER_H
#define KEYLISTENER_KEYLISTENER_H
#include "../event/KeyEvent.h"
class KeyListener {
public:
  // 按下事件
  virtual void onPress(const KeyEvent &e);
  // 释放事件
  virtual void onRelease(const KeyEvent &e);
  // 按完事件
  virtual void onPressed(const KeyEvent &e);
};

#endif // KEYLISTENER_KEYLISTENER_H
