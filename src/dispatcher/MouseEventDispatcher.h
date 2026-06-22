#pragma once

/**
 * @file MouseEventDispatcher.h
 * @brief 内部鼠标事件分发器。
 *
 * @warning 该头文件属于 src 内部实现，不是安装给用户的公开 API。
 */

#include <InputListener/listener/MouseListener.h>
#include <mutex>
#include <vector>

namespace InputListener
{

/**
 * @brief 保存鼠标监听器列表并把 MouseEvent 分发给对应回调。
 *
 * 分发器不拥有监听器对象，只保存裸指针引用。公开层通过 ListenerConnection
 * 管理注册生命周期。
 */
class MouseEventDispatcher
{
    std::vector<MouseListener*> listeners;  ///< 当前注册的鼠标监听器指针列表。
    std::mutex                  listenersMutex;  ///< 保护 listeners 的互斥锁。

public:
    /**
     * @brief 构造空分发器。
     */
    MouseEventDispatcher();

    /**
     * @brief 销毁分发器；不会销毁已注册监听器。
     */
    ~MouseEventDispatcher();

    /**
     * @brief 添加鼠标监听器。
     *
     * @param listener 要添加的监听器；nullptr 会被忽略。
     */
    void addListener(MouseListener*);

    /**
     * @brief 移除鼠标监听器。
     *
     * @param listener 要移除的监听器指针。
     */
    void removeListener(MouseListener*);

    /**
     * @brief 根据事件类型调用所有监听器的对应回调。
     *
     * @param e 要分发的鼠标事件。
     */
    void dispatchEvent(const MouseEvent& e);
};

}  // namespace InputListener
