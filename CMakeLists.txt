# Project Config
cmake_minimum_required(VERSION 3.16)


#Include Paths
include_directories(
					"D:/Documents/Ref/Platforms/Espressif/Lbr/esp32-camera/driver/include"
					"D:/Documents/Ref/Platforms/Espressif/Lbr/esp32-camera/conversions/include"
					"D:/Documents/Ref/Platforms/Espressif/Lbr/esp32-arduino-lib-builder/components/fb_gfx/include"
				   )

#
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

#
project(freenove_camera)
