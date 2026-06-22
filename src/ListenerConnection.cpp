#include <InputListener/ListenerConnection.h>

#include <utility>

namespace InputListener
{

/**
 * @brief 保存断开连接时需要执行的回调。
 */
ListenerConnection::ListenerConnection(std::function<void()> disconnectHandler)
    : disconnectHandler(std::move(disconnectHandler))
{
}

/**
 * @brief 析构时自动断开仍然有效的监听器连接。
 */
ListenerConnection::~ListenerConnection()
{
    disconnect();
}

/**
 * @brief 转移连接所有权，并清空源句柄。
 */
ListenerConnection::ListenerConnection(ListenerConnection&& other) noexcept
    : disconnectHandler(std::move(other.disconnectHandler))
{
    other.disconnectHandler = {};
}

/**
 * @brief 断开当前连接后接管 other 的连接所有权。
 */
ListenerConnection& ListenerConnection::operator=(
    ListenerConnection&& other) noexcept
{
    if ( this != &other ) {
        disconnect();
        disconnectHandler       = std::move(other.disconnectHandler);
        other.disconnectHandler = {};
    }
    return *this;
}

/**
 * @brief 执行解绑回调并把当前句柄置为空连接。
 */
void ListenerConnection::disconnect()
{
    if ( disconnectHandler ) {
        auto handler      = std::move(disconnectHandler);
        disconnectHandler = {};
        handler();
    }
}

/**
 * @brief 检查当前句柄是否仍持有解绑回调。
 */
bool ListenerConnection::connected() const
{
    return static_cast<bool>(disconnectHandler);
}

}  // namespace InputListener
