cmake_minimum_required(VERSION 3.19)
project(Acars_Sim_C)

#set(CMAKE_C_STANDARD 99)
AUX_SOURCE_DIRECTORY(. SRC_LIST) #把当前目录(.)下所有源代码文件和头文件加入变量SRC_LIST
add_executable(Acars_Sim_C ${SRC_LIST} )
add_library(acarstrans SHARED ${SRC_LIST})

set_target_properties(acarstrans PROPERTIES output_name "acarstrans")

TARGET_LINK_LIBRARIES(Acars_Sim_C LINK_PUBLIC hackrf)
TARGET_LINK_LIBRARIES(Acars_Sim_C LINK_PUBLIC m)
TARGET_LINK_LIBRARIES(acarstrans LINK_PUBLIC hackrf)
TARGET_LINK_LIBRARIES(acarstrans LINK_PUBLIC m)
