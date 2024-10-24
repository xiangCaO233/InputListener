#include "../../headers/dispatcher/MouseEventDispatcher.h"

MouseEventDispatcher::MouseEventDispatcher(){};

MouseEventDispatcher::~MouseEventDispatcher(){};
// 添加监听器
void MouseEventDispatcher::addListener(MouseListener *listener) {
  listeners.push_back(listener);
}
// 移除监听器
void MouseEventDispatcher::removeListener(MouseListener *listener) {
  //
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
void MouseEventDispatcher::dispatchEvent(const MouseEvent &e) {
  for (auto l : listeners) {
    switch (e.getType()) {
    case MouseEventType::PRESS: {
      l->onPress(e);
      break;
    }
    case MouseEventType::RELEASE: {
      l->onRelease(e);
      break;
    }
    case MouseEventType::MOVE: {
      l->onMove(e);
      break;
    }
    case MouseEventType::DRAG: {
      l->onDrag(e);
      break;
    }
    case MouseEventType::PRESSED: {
      l->onPressed(e);
      break;
    }
    case MouseEventType::DRAGGED: {
      l->onDragged(e);
      break;
    }
    }
  }
}
