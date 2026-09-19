add_library(revorb STATIC revorb.c)
target_include_directories(revorb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(revorb PRIVATE Ogg::ogg vorbis)
