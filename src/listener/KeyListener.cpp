#include <InputListener/listener/KeyListener.h>

namespace InputListener {

/**
 * @brief 键盘按下事件的默认空处理。
 */
void KeyListener::onPress(const KeyEvent &e) {}

/**
 * @brief 键盘释放事件的默认空处理。
 */
void KeyListener::onRelease(const KeyEvent &e) {}

/**
 * @brief 键盘重复事件的默认空处理。
 */
void KeyListener::onRepeat(const KeyEvent &e) {}

/**
 * @brief 键盘完成事件的默认空处理。
 */
void KeyListener::onPressed(const KeyEvent &e) {}

} // namespace InputListener
