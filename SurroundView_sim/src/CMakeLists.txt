#添加目录
add_subdirectory(common)
add_subdirectory(Pod_tools)
add_subdirectory(s1_car_draw)
add_subdirectory(s2_srv_draw)
add_subdirectory(shader)

#包含文件
set(INC_DIR ./common/
            ./shader/ 
            ./s1_car_draw/
            ./s2_srv_draw/
            ./Pod_tools/ 
            ./Pod_tools/OGLES
            )

include_directories(${INC_DIR})

#所有的cpp文件包含到source list当中
set(SRC_LIST main.cpp 
             #通过搜索的方法获取 common文件夹下的所有c/cpp源文件,见common下的cmakelists
             ${DIR_COMMON_SRCS}
             shader/glad.c
             shader/renderutils.cpp
             s1_car_draw/car.cpp
	     s2_srv_draw/srv.cpp
             Pod_tools/PVRTVertex.cpp
             Pod_tools/PVRTVector.cpp 
             Pod_tools/PVRTUnicode.cpp 
             Pod_tools/PVRTTrans.cpp 
             Pod_tools/PVRTTexture.cpp
             Pod_tools/PVRTStringHash.cpp
             Pod_tools/PVRTString.cpp
             Pod_tools/PVRTResourceFile.cpp 
             Pod_tools/PVRTDecompress.cpp
             Pod_tools/PVRTError.cpp
             Pod_tools/PVRTMatrixF.cpp
             Pod_tools/PVRTModelPOD.cpp
             Pod_tools/PVRTQuaternionF.cpp    
             )

add_executable(Surroundview_sim ${SRC_LIST})

#MESSAGE(STATUS "the project_name is : " ${PROJECT_NAME})

# opengl 链接库名称保存在 OPENGL_LIBS 变量
set(OPENGL_LIBS GL glfw3 X11 Xxf86vm Xrandr pthread Xi dl Xinerama Xcursor m)

target_link_libraries(Surroundview_sim ${OPENGL_LIBS} common s1_car_draw s2_srv_draw Pod_tools)


