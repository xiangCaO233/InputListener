#pragma once

/**
 * @file KeyEventDispatcher.h
 * @brief 内部键盘事件分发器。
 *
 * @warning 该头文件属于 src 内部实现，不是安装给用户的公开 API。
 */

#include <InputListener/event/KeyEvent.h>
#include <InputListener/listener/KeyListener.h>
#include <mutex>
#include <vector>

namespace InputListener
{

/**
 * @brief 保存键盘监听器列表并把 KeyEvent 分发给对应回调。
 *
 * 分发器不拥有监听器对象，只保存裸指针引用。公开层通过 ListenerConnection
 * 管理注册生命周期。
 */
class KeyEventDispatcher
{
    std::vector<KeyListener*> listeners;  ///< 当前注册的键盘监听器指针列表。
    std::mutex                listenersMutex;  ///< 保护 listeners 的互斥锁。

public:
    /**
     * @brief 构造空分发器。
     */
    KeyEventDispatcher();

    /**
     * @brief 销毁分发器；不会销毁已注册监听器。
     */
    ~KeyEventDispatcher();

    /**
     * @brief 添加键盘监听器。
     *
     * @param listener 要添加的监听器；nullptr 会被忽略。
     */
    void addListener(KeyListener*);

    /**
     * @brief 移除键盘监听器。
     *
     * @param listener 要移除的监听器指针。
     */
    void removeListener(KeyListener*);

    /**
     * @brief 根据事件类型调用所有监听器的对应回调。
     *
     * @param event 要分发的键盘事件。
     */
    void dispatchEvent(const KeyEvent& event);
};

}  // namespace InputListener
