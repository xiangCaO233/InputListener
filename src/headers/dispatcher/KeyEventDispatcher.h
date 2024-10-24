#ifndef KEYTIPWIDGET_KEYTIPWIDGET_H
#define KEYTIPWIDGET_KEYTIPWIDGET_H

#include "../event/KeyEvent.h"
#include "../listener/KeyListener.h"
#include <vector>

class KeyEventDispatcher {
  std::vector<KeyListener *> listeners;

public:
  // 构造与析构
  KeyEventDispatcher();
  ~KeyEventDispatcher();
  // 添加监听器
  void addListener(KeyListener *);
  // 移除按键监听器
  void removeListener(KeyListener *);
  // 派遣事件
  void dispachEvent(const KeyEvent &event);
};

#endif // KEYTIPWIDGET_KEYTIPWIDGET_H
