set(CAR_INC_DIR usr/include/)

include_directories(${CAR_INC_DIR})
#包含文件
aux_source_directory(. DIR_S1_CAR_DRAW_SRCS)
add_library(s1_car_draw ${DIR_S1_CAR_DRAW_SRCS})

#set(OPENGL_LDFLAGS /usr/local/lib /usr/lib64)
#link_directories(${OPENGL_LDFLAGS})

