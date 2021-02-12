#include "window.h"

#include <stddef.h>
#include <stdio.h>

#define HAS_WEBOS_SETTINGS OS_WEBOS || DEBUG

void _settings_pane_advanced(struct nk_context *ctx)
{
#if HAS_WEBOS_SETTINGS
    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_label(ctx, "Video Decoder", NK_TEXT_LEFT);
    static const char *platforms[] = {"auto", "legacy"};
    if (nk_combo_begin_label(ctx, app_configuration->platform, nk_vec2(nk_widget_width(ctx), 200 * NK_UI_SCALE)))
    {
        nk_layout_row_dynamic_s(ctx, 25, 1);
        for (int i = 0; i < NK_LEN(platforms); i++)
        {
            if (nk_combo_item_label(ctx, platforms[i], NK_TEXT_LEFT))
            {
                app_configuration->platform = (char *)platforms[i];
            }
        }
        nk_combo_end(ctx);
    }
    nk_layout_row_dynamic_s(ctx, 25, 1);
    nk_bool sdlaud = app_configuration->audio_device && strcmp(app_configuration->audio_device, "sdl") == 0;
    nk_checkbox_label(ctx, "Use SDL to play audio", &sdlaud);
    app_configuration->audio_device = sdlaud ? "sdl" : NULL;
    nk_spacing(ctx, 1);
#endif
}