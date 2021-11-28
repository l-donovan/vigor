project(vigor)

include_directories(${VIGOR_SOURCE_DIR}/glad/include)
link_directories(${VIGOR_SOURCE_DIR}/glad/src)

include_directories(${VIGOR_SOURCE_DIR}/include)

add_executable(vigor
    engine.cpp
    main.cpp
    shader.cpp
    example_layer.cpp
    text_layer.cpp
    window.cpp
   "text_buffer.cpp")

target_link_libraries(vigor PRIVATE glad)
