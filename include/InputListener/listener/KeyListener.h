#pragma once

/**
 * @file KeyListener.h
 * @brief 定义键盘事件监听器接口。
 */

#include <InputListener/event/KeyEvent.h>

namespace InputListener
{

/**
 * @brief 接收键盘事件的可继承监听器接口。
 *
 * 所有回调默认空实现，使用方只需要覆写自己关心的事件。
 */
class KeyListener
{
public:
    /**
     * @brief 虚析构函数，保证通过基类指针销毁派生类对象时行为正确。
     */
    virtual ~KeyListener() = default;

    /**
     * @brief 按键按下时触发。
     *
     * @param e 键盘按下事件。
     */
    virtual void onPress(const KeyEvent& e);

    /**
     * @brief 按键释放时触发。
     *
     * @param e 键盘释放事件。
     */
    virtual void onRelease(const KeyEvent& e);

    /**
     * @brief 按键保持按下并由系统产生重复输入时触发。
     *
     * @param e 键盘重复事件。
     */
    virtual void onRepeat(const KeyEvent& e);

    /**
     * @brief 一次按下释放动作完成后触发。
     *
     * @param e 键盘完成事件。
     */
    virtual void onPressed(const KeyEvent& e);
};

}  // namespace InputListener
