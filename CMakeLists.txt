# https://www.bilibili.com/read/cv18494541/
# 设置cmake最小版本
cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE "D:/utils/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_TARGET_TRIPLET "x64-windows")

# 设置项目名称
project(openglRender)

set(PYTHON_EXECUTABLE "D:/Anaconda/envs/AiStudy/python.exe")

# 打印python 版本和解释器路径
message(STATUS "python version: ${PYTHON_VERSION}")
message(STATUS "python path: ${PYTHON_EXECUTABLE}")

# 打印python头文件路径和库文件路径
message(STATUS "python include path: ${PYTHON_INCLUDE_DIRS}")
message(STATUS "python lib path: ${PYTHON_LIBRARIES}")

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 设置源文件列表
list(APPEND SOURCE_FILES
        main.cpp
)
set(exename openglRender)

# 因为pybind11_add_module是pybind11提供的，所以我们在使用它之前需要先使用find_package命令来找到pybind11库。

# Gather header files in different directories
file(GLOB INCLUDE_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB COMMAND_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/custom_command/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Test/*.h"
)

# Gather all source files
set(SOURCE_FILES
        main.cpp
        ${INCLUDE_HEADER_FILES} # Add include header files to the source files list
        ${COMMAND_HEADER_FILES} # Add command header files to the source files list
)

# Create a source group for header files in the include directory and assign it to the INCLUDE_HEADER_FILES variable
# source_group创建的文件夹名字是取决于相对路径，会显示这些头文件的上一级目录
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${INCLUDE_HEADER_FILES})

# Create a source group for header files in the shader directory and assign it to the COMMAND_HEADER_FILES variable
# source_group("CustomFolder" FILES ${COMMAND_HEADER_FILES})
source_group("CustomFolder" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${COMMAND_HEADER_FILES})

# 添加可执行文件
add_executable(${exename} ${SOURCE_FILES})

# 添加python头文件和库文件
target_include_directories(${exename} PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(${exename} PRIVATE ${PYTHON_LIBRARIES})

# 添加头文件目录
target_include_directories(${exename} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加头文件目录
target_include_directories(${exename} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/custom_command)

# 添加头文件目录
target_include_directories(${exename} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Test)

# 链接第三方库
find_package(glad CONFIG REQUIRED)
target_link_libraries(${exename} PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${exename} PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${exename} PRIVATE glm::glm)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${exename} PRIVATE assimp::assimp)
configure_file(root_directory.h.in ${CMAKE_SOURCE_DIR}/include/root_directory.h)

if(MSVC)
        # 设置 Cpp 语言编译 flags,  输入代码编码格式为 utf-8
        set(CMAKE_CXX_FLAGS /source-charset:utf-8)
        set_target_properties(${exename} PROPERTIES COMPILE_FLAGS "/EHsc")
endif()

# add_subdirectory("D:/utils/pybind11/pybind11" pybindbuild)

# # 因为pybind11_add_module是pybind11提供的，所以我们在使用它之前需要先使用find_package命令来找到pybind11库。
# find_package(pybind11 CONFIG REQUIRED)
# find_package(PythonInterp REQUIRED)

# # pybind11_add_module是pybind11提供的一个CMake函数，用于简化pybind11项目的生成Python拓展模块的流程。
# # 设置生成py接口的源文件列表
# list(APPEND pySOURCE_FILES
# pyApi.cpp
# )
# set(pyModuleName renderPyApi)
# pybind11_add_module(${pyModuleName} ${pySOURCE_FILES})

# # 添加头文件目录
# target_include_directories(${pyModuleName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# # 添加头文件目录
# target_include_directories(${pyModuleName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/shader)

# # 链接第三方库
# find_package(glad CONFIG REQUIRED)
# target_link_libraries(${pyModuleName} PRIVATE glad::glad)

# find_package(glfw3 CONFIG REQUIRED)
# target_link_libraries(${pyModuleName} PRIVATE glfw)

# find_package(glm CONFIG REQUIRED)
# target_link_libraries(${pyModuleName} PRIVATE glm::glm)

# find_package(assimp CONFIG REQUIRED)
# target_link_libraries(${pyModuleName} PRIVATE assimp::assimp)