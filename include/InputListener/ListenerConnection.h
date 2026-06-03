#pragma once

/**
 * @file ListenerConnection.h
 * @brief 定义监听器自动解绑句柄。
 */

#include <functional>

namespace InputListener {

/**
 * @brief 管理一次监听器注册关系的 RAII 句柄。
 *
 * ListenerConnection 不拥有监听器对象本身，只保存解绑动作。句柄析构、移动赋值
 * 覆盖旧句柄，或显式调用 disconnect() 时，会执行解绑动作。
 */
class ListenerConnection {
  std::function<void()> disconnectHandler; ///< 当前连接的解绑回调；为空表示未连接。

public:
  /**
   * @brief 创建一个空连接句柄。
   */
  ListenerConnection() = default;

  /**
   * @brief 使用指定解绑回调创建连接句柄。
   *
   * @param disconnectHandler 连接断开时调用的回调。
   */
  explicit ListenerConnection(std::function<void()> disconnectHandler);

  /**
   * @brief 析构时自动断开连接。
   */
  ~ListenerConnection();

  /**
   * @brief 禁止复制，避免同一注册关系被多个句柄重复解绑。
   */
  ListenerConnection(const ListenerConnection &) = delete;

  /**
   * @brief 禁止复制赋值，避免同一注册关系被多个句柄重复解绑。
   */
  ListenerConnection &operator=(const ListenerConnection &) = delete;

  /**
   * @brief 移动构造，转移解绑责任。
   *
   * @param other 被移动的连接句柄；移动后变为空连接。
   */
  ListenerConnection(ListenerConnection &&other) noexcept;

  /**
   * @brief 移动赋值，先断开当前连接，再接管 other 的解绑责任。
   *
   * @param other 被移动的连接句柄；移动后变为空连接。
   * @return 当前连接句柄。
   */
  ListenerConnection &operator=(ListenerConnection &&other) noexcept;

  /**
   * @brief 显式断开连接。
   *
   * 多次调用是安全的；第一次调用后句柄会变为空连接。
   */
  void disconnect();

  /**
   * @brief 判断句柄当前是否仍持有连接。
   *
   * @return 持有解绑回调时返回 true，否则返回 false。
   */
  bool connected() const;
};

} // namespace InputListener

