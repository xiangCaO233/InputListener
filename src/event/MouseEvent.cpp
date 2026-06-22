#include <InputListener/event/MouseEvent.h>

namespace InputListener
{

/**
 * @brief 保存一次鼠标事件的按键、修饰键、类型和位置坐标。
 */
MouseEvent::MouseEvent(int b, const Modifiers& m, MouseEventType t, int x,
                       int y, double x2d, double y2d, double sx, double sy)
    : button(b)
    , modifiers(m)
    , type(t)
    , xv(x)
    , yv(y)
    , xd(x2d)
    , yd(y2d)
    , scrollDeltaX(sx)
    , scrollDeltaY(sy)
{
}

/**
 * @brief 默认析构；MouseEvent 不持有外部资源。
 */
MouseEvent::~MouseEvent() {}

/**
 * @brief 返回鼠标按键编号。
 */
int MouseEvent::getButton() const
{
    return button;
}

/**
 * @brief 返回事件发生时的修饰键状态。
 */
Modifiers MouseEvent::getModifiers() const
{
    return modifiers;
}

/**
 * @brief 返回鼠标事件类型。
 */
MouseEventType MouseEvent::getType() const
{
    return type;
}

/**
 * @brief 返回整数 x 坐标。
 */
int MouseEvent::X() const
{
    return xv;
}

/**
 * @brief 返回整数 y 坐标。
 */
int MouseEvent::Y() const
{
    return yv;
}

/**
 * @brief 返回浮点 x 坐标。
 */
double MouseEvent::X2D() const
{
    return xd;
}

/**
 * @brief 返回浮点 y 坐标。
 */
double MouseEvent::Y2D() const
{
    return yd;
}

/**
 * @brief 返回水平滚动增量。
 */
double MouseEvent::getScrollDeltaX() const
{
    return scrollDeltaX;
}

/**
 * @brief 返回垂直滚动增量。
 */
double MouseEvent::getScrollDeltaY() const
{
    return scrollDeltaY;
}

}  // namespace InputListener
