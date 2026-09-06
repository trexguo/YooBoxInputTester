cmake_minimum_required(VERSION 3.10)
project(YooBoxInputTester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CROSSCOMPILING)
    set(TOOLCHAIN_SYSROOT ${CMAKE_SYSROOT})
    set(SDL2_INCLUDE_DIR ${TOOLCHAIN_SYSROOT}/usr/include/SDL2)
    set(SDL2_LIB_DIR ${TOOLCHAIN_SYSROOT}/usr/lib)

    add_executable(YooBoxInputTester
        src/main.cpp
        src/app.cpp
        src/ui.cpp
        src/font.cpp
    )

    target_include_directories(YooBoxInputTester PRIVATE
        ${SDL2_INCLUDE_DIR}
        ${TOOLCHAIN_SYSROOT}/usr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_directories(YooBoxInputTester PRIVATE ${SDL2_LIB_DIR})

    target_link_libraries(YooBoxInputTester
        SDL2
        SDL2_ttf
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

    add_executable(YooBoxInputTester
        src/main.cpp
        src/app.cpp
        src/ui.cpp
        src/font.cpp
    )

    target_include_directories(YooBoxInputTester PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_directories(YooBoxInputTester PRIVATE
        ${SDL2_LIBRARY_DIRS}
        ${SDL2_TTF_LIBRARY_DIRS}
    )

    # macOS/Homebrew 的 sdl2-compat 在 .pc 里声明 -lSDL2main 却不提供该库;
    # 本程序使用普通 int main(),不需要 SDL2main,链接时直接剔除
    set(_SDL2_LIBS
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
    )
    list(FILTER _SDL2_LIBS EXCLUDE REGEX "SDL2main")

    target_link_libraries(YooBoxInputTester
        ${_SDL2_LIBS}
    )

    target_compile_options(YooBoxInputTester PRIVATE
        ${SDL2_CFLAGS_OTHER}
    )
endif()
