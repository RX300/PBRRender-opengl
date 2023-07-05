# 设置cmake最小版本
cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE "D:/utils/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_TARGET_TRIPLET "x64-windows")

# 设置项目名称
project(Advanced—intancing)

# set(PYTHON_EXECUTABLE "D:/Anaconda/envs/AiStudy/python.exe")

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 设置源文件列表
list(APPEND SOURCE_FILES
        main.cpp
)
set(exename advanced_GLSL)

# 因为pybind11_add_module是pybind11提供的，所以我们在使用它之前需要先使用find_package命令来找到pybind11库。

# 添加可执行文件
add_executable(${exename} ${SOURCE_FILES})

# 添加头文件目录
target_include_directories(${exename} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加头文件目录
target_include_directories(${exename} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/shader)

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