#包含文件
set(POD_INC_DIR ./ ./OGLES)
include_directories(${POD_INC_DIR})

aux_source_directory(. DIR_POD_TOOLS_SRCS)
add_library(Pod_tools ${DIR_POD_TOOLS_SRCS})
