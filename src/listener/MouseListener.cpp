#include <InputListener/listener/MouseListener.h>

namespace InputListener {

/**
 * @brief 鼠标按下事件的默认空处理。
 */
void MouseListener::onPress(const MouseEvent &e) {}

/**
 * @brief 鼠标释放事件的默认空处理。
 */
void MouseListener::onRelease(const MouseEvent &e) {}

/**
 * @brief 鼠标拖动事件的默认空处理。
 */
void MouseListener::onDrag(const MouseEvent &e) {}

/**
 * @brief 鼠标移动事件的默认空处理。
 */
void MouseListener::onMove(const MouseEvent &e) {}

/**
 * @brief 鼠标滚动事件的默认空处理。
 */
void MouseListener::onScroll(const MouseEvent &e) {}

/**
 * @brief 鼠标点击完成事件的默认空处理。
 */
void MouseListener::onPressed(const MouseEvent &e) {}

/**
 * @brief 鼠标拖动完成事件的默认空处理。
 */
void MouseListener::onDragged(const MouseEvent &e) {}

} // namespace InputListener
