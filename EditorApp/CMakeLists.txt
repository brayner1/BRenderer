find_package(Vulkan)

set(Editor_SRC_FILES
	"EditorApp.cpp"
)

add_executable (EditorApp ${Editor_SRC_FILES})

set_property(TARGET EditorApp PROPERTY CXX_STANDARD 20)
set_property(TARGET EditorApp PROPERTY CXX_STANDARD_REQUIRED ON)

#target_include_directories(EditorApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries(EditorApp PUBLIC BRenderer)

message("Shader files: ${BRR_SHADER_FILES}")

add_custom_command(TARGET EditorApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/Resources $<TARGET_FILE_DIR:EditorApp>/Resources
)

add_custom_command(TARGET EditorApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:BRenderer>/Engine $<TARGET_FILE_DIR:EditorApp>/Engine
)