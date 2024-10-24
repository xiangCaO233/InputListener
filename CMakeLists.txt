cmake_minimum_required(VERSION 3.28)

project(InputListener)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(APPLE)
  # 查找 ApplicationServices, CoreFoundation ,Carbon框架
  find_library(APPLICATION_SERVICES_LIBRARY ApplicationServices)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(CARBON_LIBRARY Carbon)
endif(APPLE)
if(UNIX)
  # 查找库
endif(UNIX)

find_package(
  Qt6
  COMPONENTS Core Gui Widgets
  REQUIRED)

set(INCLUDES src/headers)

set(SOURCES
    src/cpp/GlobalScreen.cpp
    src/cpp/dispatcher/MouseEventDispatcher.cpp
    src/cpp/dispatcher/KeyEventDispatcher.cpp
    src/cpp/event/KeyEvent.cpp
    src/cpp/event/MouseEvent.cpp
    src/cpp/listener/KeyListener.cpp
    src/cpp/listener/MouseListener.cpp)

add_library(InputLsnr-static STATIC ${SOURCES})
add_library(InputLsnr-dynamic SHARED ${SOURCES})

add_executable(InputListener ${SOURCES} src/cpp/test.cpp)

target_include_directories(InputListener PUBLIC ${INCLUDES})
target_include_directories(InputLsnr-static PUBLIC ${INCLUDES})
target_include_directories(InputLsnr-dynamic PUBLIC ${INCLUDES})

if(APPLE)
  target_link_libraries(
    InputLsnr-static ${CARBON_LIBRARY} ${COREFOUNDATION_LIBRARY}
    ${APPLICATION_SERVICES_LIBRARY})
  target_link_libraries(
    InputLsnr-dynamic ${CARBON_LIBRARY} ${COREFOUNDATION_LIBRARY}
    ${APPLICATION_SERVICES_LIBRARY})

  # 链接 Carbon 和 CoreFoundation 库
  target_link_libraries(
    InputListener
    ${CARBON_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    ${APPLICATION_SERVICES_LIBRARY}
    Qt::Core
    Qt::Gui
    Qt::Widgets)
elseif(UNIX)
  target_link_libraries(InputLsnr-static)
  target_link_libraries(InputLsnr-dynamic)
  target_link_libraries(InputLsnr Qt::Core Qt::Gui Qt::Widgets)
elseif(WIN32)
  target_link_libraries(InputLsnr-static)
  target_link_libraries(InputLsnr-dynamic)
  target_link_libraries(InputLsnr Qt::Core Qt::Gui Qt::Widgets)
endif()
