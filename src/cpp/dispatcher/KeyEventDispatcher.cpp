#include "../headers/dispatcher/KeyEventDispatcher.h"
#include <iostream>

KeyEventDispatcher::KeyEventDispatcher() {}

KeyEventDispatcher::~KeyEventDispatcher() {}

// 添加监听器
void KeyEventDispatcher::addListener(KeyListener *listener) {
  listeners.push_back(listener);
}
// 移除监听器
void KeyEventDispatcher::removeListener(KeyListener *listener) {
  // listeners.erase(listener);
  int i = 0;
  for (; i < listeners.size(); i++) {
    if (listener == listeners[i])
      break;
  }
  if (i < listeners.size()) {
    listeners.erase(listeners.begin() + i);
    for (int j = i + 1; j < listeners.size(); j++) {
      listeners[j - 1] = listeners[j];
    }
  }
}
// 分派事件
void KeyEventDispatcher::dispachEvent(const KeyEvent &event) {
  // std::cout << "监听器列表长度：" << listeners.size() << std::endl;
  for (auto l : listeners) {
    auto t = event.getType();
    switch (t) {
    case KeyEventType::PRESS: {
      l->onPress(event);
      break;
    };
    case KeyEventType::RELEASE: {
      l->onRelease(event);
      break;
    }
    case KeyEventType::PRESSED: {
      l->onPressed(event);
      break;
    }
    }
  }
}
