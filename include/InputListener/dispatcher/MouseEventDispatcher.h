#ifndef MOUSEEVENTDISPATCHER_H
#define MOUSEEVENTDISPATCHER_H

#include <InputListener/listener/MouseListener.h>
#include <vector>

namespace InputListener {

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

} // namespace InputListener

#endif // MOUSEEVENTDISPATCHER_H
