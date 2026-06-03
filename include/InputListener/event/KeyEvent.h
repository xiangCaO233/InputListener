#ifndef INPUTLISTENER_EVENT_KEYEVENT_H
#define INPUTLISTENER_EVENT_KEYEVENT_H

/**
 * @file KeyEvent.h
 * @brief 定义键盘事件数据结构。
 */

#include <string>

namespace InputListener {

/**
 * @brief 输入事件触发时处于按下状态的修饰键集合。
 */
struct Modifiers {
  bool shift = false;   ///< Shift 键是否处于按下状态。
  bool control = false; ///< Control 键是否处于按下状态。
  bool option = false;  ///< Option/Alt 键是否处于按下状态。
  bool command = false; ///< Command/Meta 键是否处于按下状态。
};

/**
 * @brief 键盘事件类型。
 */
enum class KeyEventType {
  PRESS,   ///< 按键被按下。
  RELEASE, ///< 按键被释放。
  REPEAT,  ///< 按键保持按下并产生系统重复事件。
  PRESSED  ///< 一次完整按下释放动作结束后的完成事件。
};

/**
 * @brief 平台原生键盘事件转换后的跨平台键盘事件。
 */
class KeyEvent {
  int rawCode;           ///< 平台提供的原始键码。
  std::string key;       ///< 根据当前键盘映射得到的可显示按键文本。
  Modifiers modifiers;   ///< 事件发生时的修饰键状态。
  KeyEventType type;     ///< 当前键盘事件类型。

public:
  /**
   * @brief 构造键盘事件。
   *
   * @param rawCode 平台原始键码。
   * @param key 可显示按键文本；不可识别时可为空字符串。
   * @param modifiers 事件发生时的修饰键状态。
   * @param type 键盘事件类型。
   */
  KeyEvent(int rawCode, std::string key, const Modifiers &modifiers,
           KeyEventType type);

  /**
   * @brief 销毁键盘事件。
   */
  ~KeyEvent();

  /**
   * @brief 获取平台原始键码。
   *
   * @return 平台原始键码。
   */
  int getRawCode() const;

  /**
   * @brief 获取可显示按键文本。
   *
   * @return 按键文本引用。
   */
  const std::string &getKey() const;

  /**
   * @brief 获取事件发生时的修饰键状态。
   *
   * @return 修饰键状态值。
   */
  Modifiers getModifiers() const;

  /**
   * @brief 获取键盘事件类型。
   *
   * @return 键盘事件类型。
   */
  KeyEventType getType() const;
};

} // namespace InputListener

#endif // INPUTLISTENER_EVENT_KEYEVENT_H
