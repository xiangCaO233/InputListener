#include "KeyEventDispatcher.h"

#include <algorithm>

namespace InputListener {

/**
 * @brief 创建空的键盘事件分发器。
 */
KeyEventDispatcher::KeyEventDispatcher() {}

/**
 * @brief 销毁分发器；监听器对象由调用方管理。
 */
KeyEventDispatcher::~KeyEventDispatcher() {}

/**
 * @brief 将非空监听器加入分发列表。
 */
void KeyEventDispatcher::addListener(KeyListener *listener) {
  if (listener == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(listenersMutex);
  listeners.push_back(listener);
}

/**
 * @brief 从分发列表中移除指定监听器。
 */
void KeyEventDispatcher::removeListener(KeyListener *listener) {
  std::lock_guard<std::mutex> lock(listenersMutex);
  listeners.erase(std::remove(listeners.begin(), listeners.end(), listener),
                  listeners.end());
}

/**
 * @brief 根据 KeyEventType 把事件派发给所有键盘监听器。
 *
 * 先在锁内复制监听器快照，再在锁外执行回调，避免回调中注册或注销监听器时
 * 与分发器互斥锁形成重入死锁。
 */
void KeyEventDispatcher::dispatchEvent(const KeyEvent &event) {
  std::vector<KeyListener *> listenerSnapshot;
  {
    std::lock_guard<std::mutex> lock(listenersMutex);
    listenerSnapshot = listeners;
  }

  for (auto l : listenerSnapshot) {
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
    case KeyEventType::REPEAT: {
      l->onRepeat(event);
      break;
    }
    case KeyEventType::PRESSED: {
      l->onPressed(event);
      break;
    }
    }
  }
}

} // namespace InputListener
