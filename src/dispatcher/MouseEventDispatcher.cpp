#include "MouseEventDispatcher.h"

#include <algorithm>

namespace InputListener
{

/**
 * @brief 创建空的鼠标事件分发器。
 */
MouseEventDispatcher::MouseEventDispatcher() {};

/**
 * @brief 销毁分发器；监听器对象由调用方管理。
 */
MouseEventDispatcher::~MouseEventDispatcher() {};

/**
 * @brief 将非空监听器加入分发列表。
 */
void MouseEventDispatcher::addListener(MouseListener* listener)
{
    if ( listener == nullptr ) {
        return;
    }
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners.push_back(listener);
}

/**
 * @brief 从分发列表中移除指定监听器。
 */
void MouseEventDispatcher::removeListener(MouseListener* listener)
{
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener),
                    listeners.end());
}

/**
 * @brief 根据 MouseEventType 把事件派发给所有鼠标监听器。
 *
 * 先在锁内复制监听器快照，再在锁外执行回调，避免回调中注册或注销监听器时
 * 与分发器互斥锁形成重入死锁。
 */
void MouseEventDispatcher::dispatchEvent(const MouseEvent& e)
{
    std::vector<MouseListener*> listenerSnapshot;
    {
        std::lock_guard<std::mutex> lock(listenersMutex);
        listenerSnapshot = listeners;
    }

    for ( auto l : listenerSnapshot ) {
        switch ( e.getType() ) {
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
        case MouseEventType::SCROLL: {
            l->onScroll(e);
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

}  // namespace InputListener
