#include "applications_window.h"
#include "backend/application_manager.h"
#include "backend/streaming_session.h"

#include "sdl/webos_keys.h"

#define LINKEDLIST_TYPE PAPP_LIST
#include "util/linked_list.h"

void applications_window_init(struct nk_context *ctx)
{
}

bool applications_window(struct nk_context *ctx, PSERVER_LIST node)
{
    int content_height_remaining;
    if (nk_begin(ctx, "Applications", nk_rect(100, 100, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE))
    {
        PAPP_LIST apps = node->apps;
        if (apps != NULL)
        {
            content_height_remaining = (int)nk_window_get_content_region_size(ctx).y;
            content_height_remaining -= ctx->style.window.padding.y * 2;
            struct nk_list_view list_view;
            nk_layout_row_dynamic(ctx, content_height_remaining, 1);
            int app_len = linkedlist_len(apps);
            if (nk_list_view_begin(ctx, &list_view, "apps_list", NK_WINDOW_BORDER, 25, app_len))
            {
                nk_layout_row_dynamic(ctx, 25, 1);
                PAPP_LIST cur = linkedlist_nth(apps, list_view.begin);

                for (int i = 0; i < list_view.count; ++i, cur = cur->next)
                {
                    if (nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
                    {
                        streaming_begin(node->server, cur->id);
                    }
                    nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "%d. %s", list_view.begin + i + 1, cur->name);
                }
                nk_list_view_end(&list_view);
            }
        }
    }
    nk_end(ctx);

    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, "Applications"))
    {
        nk_window_close(ctx, "Applications");
        return false;
    }
    return true;
}

bool applications_window_dispatch_userevent(struct nk_context *ctx, SDL_Event ev)
{
    return false;
}

bool applications_window_dispatch_inputevent(struct nk_context *ctx, SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_KEYUP:
        if (ev.key.keysym.sym == SDLK_WEBOS_BACK)
        {
            fprintf(stderr, "Back pressed\n");
            if (nk_window_is_active(ctx, "Applications"))
            {
                nk_window_close(ctx, "Applications");
                return true;
            }
        }
        break;
    }
    return false;
}