#include "PlatformInputListener.h"

#include <iostream>

namespace InputListener
{
namespace Internal
{

void startPlatformInputListener(InputEventSink&)
{
    std::cerr << "当前平台尚未实现全局输入监听" << std::endl;
}

void stopPlatformInputListener() {}

}  // namespace Internal
}  // namespace InputListener
