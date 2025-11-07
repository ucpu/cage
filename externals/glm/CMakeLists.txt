message(STATUS "----------------------------------------------------------------")
message(STATUS "glm")
message(STATUS "----------------------------------------------------------------")

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm")
target_compile_definitions(glm INTERFACE "GLM_FORCE_COLUMN_MAJOR=1")
target_compile_definitions(glm INTERFACE "GLM_FORCE_RIGHT_HANDED=1")
target_compile_definitions(glm INTERFACE "GLM_FORCE_DEPTH_ZERO_TO_ONE=1")

cage_gen_module(glm $<TARGET_LINKER_FILE:glm> "")
