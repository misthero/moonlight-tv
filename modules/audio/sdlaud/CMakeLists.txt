cmake_minimum_required(VERSION 3.6)

add_library(moonlight-sdlaud STATIC
        ringbuf.c
        sdl_audio.c
        )
target_link_libraries(moonlight-sdlaud PRIVATE moonlight-module-api ${SDL2_LIBRARIES})
target_include_directories(moonlight-sdlaud PRIVATE SYSTEM ${SDL2_INCLUDE_DIRS})
target_compile_definitions(moonlight-sdlaud PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
target_compile_definitions(moonlight-sdlaud PRIVATE MODULE_LOGVPRINTF=sdlaud_logvprintf MODULE_EMBEDDED=1)

if (OPUS_LIBRARIES)
    target_include_directories(moonlight-sdlaud SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight-sdlaud PRIVATE ${OPUS_LIBRARIES})
else ()
    target_link_libraries(moonlight-sdlaud PRIVATE opus)
endif ()