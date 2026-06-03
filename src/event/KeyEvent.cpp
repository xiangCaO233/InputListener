#include <InputListener/event/KeyEvent.h>

#include <utility>

namespace InputListener {

/**
 * @brief 保存一次键盘事件的原始键码、文本、修饰键和事件类型。
 */
KeyEvent::KeyEvent(int rawCode, std::string key, const Modifiers &modifiers,
                   KeyEventType type)
    : rawCode(rawCode), key(std::move(key)), modifiers(modifiers), type(type) {}

/**
 * @brief 默认析构；KeyEvent 自持有 std::string，无外部资源需要释放。
 */
KeyEvent::~KeyEvent() = default;

/**
 * @brief 返回平台原始键码。
 */
int KeyEvent::getRawCode() const { return rawCode; }

/**
 * @brief 返回键码映射得到的按键文本。
 */
const std::string &KeyEvent::getKey() const { return key; }

/**
 * @brief 返回事件发生时的修饰键状态。
 */
Modifiers KeyEvent::getModifiers() const { return modifiers; }

/**
 * @brief 返回键盘事件类型。
 */
KeyEventType KeyEvent::getType() const { return type; }

} // namespace InputListener
