cmake_minimum_required (VERSION 3.6)

add_library(moonlight-ndl SHARED 
    ndl_platform.c
    ndl_audio_pcm.c
    ndl_video.c
)
target_link_libraries(moonlight-ndl PRIVATE moonlight-module-api opus NDL_directmedia)
target_compile_definitions(moonlight-ndl PRIVATE WEBOS_TARGET_VERSION_MAJOR=4 PLUGIN_SYMBOL_SUFFIX=ndl "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-ndl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})