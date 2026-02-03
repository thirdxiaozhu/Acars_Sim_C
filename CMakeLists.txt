cmake_minimum_required(VERSION 3.19)
project(acarstrans)

AUX_SOURCE_DIRECTORY(. SRC_LIST) #把当前目录(.)下所有源代码文件和头文件加入变量SRC_LIST
add_executable(acarstrans_exe ${SRC_LIST})
add_library(acarstrans SHARED ${SRC_LIST})

set_target_properties(acarstrans PROPERTIES output_name "acarstrans")

TARGET_LINK_LIBRARIES(acarstrans_exe LINK_PUBLIC hackrf m uhd)
TARGET_LINK_LIBRARIES(acarstrans LINK_PUBLIC hackrf m uhd)
