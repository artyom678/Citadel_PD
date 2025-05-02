cmake_minimum_required(VERSION 3.28.3)

project(SystemMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux") 
	message(FATAL_ERROR "This project is supported only on Linux") 
endif()

add_executable(system_monitor 
	${CMAKE_SOURCE_DIR}/src/main.cpp 
	${CMAKE_SOURCE_DIR}/src/system_monitor.cpp
	)

target_include_directories(system_monitor PRIVATE ${CMAKE_SOURCE_DIR}/include)