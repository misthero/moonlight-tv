cmake_minimum_required (VERSION 3.6)

add_library(moonlight-ndlaud-webos5 SHARED
    ndl_platform.c
    ndl_audio_opus.c
    ndl_webos5.c
    base64.c
)

target_link_libraries(moonlight-ndlaud-webos5 PRIVATE moonlight-module-api NDL_directmedia)
target_compile_definitions(moonlight-ndlaud-webos5 PRIVATE WEBOS_TARGET_VERSION_MAJOR=5 PLUGIN_SYMBOL_SUFFIX=ndlaud_webos5 "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-ndlaud-webos5 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})