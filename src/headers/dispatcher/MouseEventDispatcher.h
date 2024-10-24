#ifndef MOUSEEVENTDISPATCHER_H
#define MOUSEEVENTDISPATCHER_H

#include "../listener/MouseListener.h"
#include <vector>

class MouseEventDispatcher {
  std::vector<MouseListener *> listeners;

public:
  MouseEventDispatcher();
  ~MouseEventDispatcher();

  // 添加鼠标监听器
  void addListener(MouseListener *);

  // 移除鼠标监听器
  void removeListener(MouseListener *);

  // 分派事件
  void dispatchEvent(const MouseEvent &e);
};

#endif // MOUSEEVENTDISPATCHER_H
