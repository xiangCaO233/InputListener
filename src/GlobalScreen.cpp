#include <InputListener/GlobalScreen.h>

/**
 * @file GlobalScreen.cpp
 * @brief GlobalScreen facade 的核心分发实现。
 */

#include "dispatcher/KeyEventDispatcher.h"
#include "dispatcher/MouseEventDispatcher.h"
#include "platform/PlatformInputListener.h"

namespace InputListener
{
namespace
{

KeyEventDispatcher   keyEventDispatcher;    ///< 进程级键盘事件分发器。
MouseEventDispatcher mouseEventDispatcher;  ///< 进程级鼠标事件分发器。

/**
 * @brief 平台事件到内部 dispatcher 的桥接器。
 *
 * 平台监听层只依赖 InputEventSink，不直接依赖具体 dispatcher。GlobalScreen
 * 通过该桥接器把平台层转换出的跨平台事件送入对应分发器。
 */
class DispatcherEventSink : public Internal::InputEventSink
{
public:
    /**
     * @brief 将键盘事件送入键盘分发器。
     *
     * @param event 平台层转换出的键盘事件。
     */
    void dispatchKeyEvent(const KeyEvent& event) override
    {
        keyEventDispatcher.dispatchEvent(event);
    }

    /**
     * @brief 将鼠标事件送入鼠标分发器。
     *
     * @param event 平台层转换出的鼠标事件。
     */
    void dispatchMouseEvent(const MouseEvent& event) override
    {
        mouseEventDispatcher.dispatchEvent(event);
    }
};

DispatcherEventSink eventSink;  ///< 供平台监听层回传事件的全局 sink。

}  // namespace

/**
 * @brief 启动当前平台的全局输入监听。
 */
void GlobalScreen::registerScreenHook()
{
    Internal::startPlatformInputListener(eventSink);
}

/**
 * @brief 停止当前平台的全局输入监听。
 */
void GlobalScreen::unRegisterScreenHook()
{
    Internal::stopPlatformInputListener();
}

/**
 * @brief 注册键盘监听器并返回自动解绑连接。
 */
ListenerConnection GlobalScreen::addKeyListener(KeyListener& listener)
{
    addKeyListener(&listener);
    auto listenerPtr = &listener;
    return ListenerConnection(
        [listenerPtr] { GlobalScreen::removeKeyListener(listenerPtr); });
}

/**
 * @brief 注册鼠标监听器并返回自动解绑连接。
 */
ListenerConnection GlobalScreen::addMouseListener(MouseListener& listener)
{
    addMouseListener(&listener);
    auto listenerPtr = &listener;
    return ListenerConnection(
        [listenerPtr] { GlobalScreen::removeMouseListener(listenerPtr); });
}

/**
 * @brief 通过裸指针注册键盘监听器的兼容入口。
 */
void GlobalScreen::addKeyListener(KeyListener* listener)
{
    keyEventDispatcher.addListener(listener);
}

/**
 * @brief 移除通过裸指针注册的键盘监听器。
 */
void GlobalScreen::removeKeyListener(KeyListener* listener)
{
    keyEventDispatcher.removeListener(listener);
}

/**
 * @brief 通过裸指针注册鼠标监听器的兼容入口。
 */
void GlobalScreen::addMouseListener(MouseListener* listener)
{
    mouseEventDispatcher.addListener(listener);
}

/**
 * @brief 移除通过裸指针注册的鼠标监听器。
 */
void GlobalScreen::removeMouseListener(MouseListener* listener)
{
    mouseEventDispatcher.removeListener(listener);
}

}  // namespace InputListener
