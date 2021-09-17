#include <util/user_event.h>
#include <ui/root.h>
#include <lvgl/font/symbols_material_icon.h>
#include <lvgl/util/lv_app_utils.h>
#include <errors.h>
#include "app.h"
#include "launcher.controller.h"
#include "apps.controller.h"

#include "util/logging.h"
#include "add.dialog.h"
#include "pair.dialog.h"

static void launcher_controller(struct lv_obj_controller_t *self, void *args);

static void launcher_view_init(lv_obj_controller_t *self, lv_obj_t *view);

static void launcher_view_destroy(lv_obj_controller_t *self, lv_obj_t *view);

static bool launcher_event_cb(lv_obj_controller_t *self, int which, void *data1, void *data2);

static void on_pc_added(const pcmanager_resp_t *resp, void *userdata);

static void on_pc_updated(const pcmanager_resp_t *resp, void *userdata);

static void on_pc_removed(const pcmanager_resp_t *resp, void *userdata);

static void update_pclist(launcher_controller_t *controller);

static void cb_pc_selected(lv_event_t *event);

static void cb_nav_focused(lv_event_t *event);

static void cb_detail_focused(lv_event_t *event);

static void open_pair(launcher_controller_t *controller, PSERVER_LIST node);

static void open_manual_add(lv_event_t *event);

static void select_pc(launcher_controller_t *controller, PSERVER_LIST selected);

static void set_detail_opened(launcher_controller_t *controller, bool opened);

static lv_obj_t *pclist_item_create(launcher_controller_t *controller, PSERVER_LIST cur);

static const char *server_item_icon(const SERVER_LIST *node);

const lv_obj_controller_class_t launcher_controller_class = {
        .constructor_cb = launcher_controller,
        .create_obj_cb = launcher_win_create,
        .obj_created_cb = launcher_view_init,
        .obj_deleted_cb = launcher_view_destroy,
        .event_cb = launcher_event_cb,
        .instance_size = sizeof(launcher_controller_t),
};

static const pcmanager_listener_t pcmanager_callbacks = {
        .added = on_pc_added,
        .updated = on_pc_updated,
        .removed = on_pc_removed,
};

static void launcher_controller(struct lv_obj_controller_t *self, void *args) {
    (void) args;
    launcher_controller_t *controller = (launcher_controller_t *) self;
    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (cur->selected) {
            controller->selected_server = cur;
            break;
        }
    }
    static const lv_style_prop_t props[] = {
            LV_STYLE_OPA, LV_STYLE_BG_OPA, LV_STYLE_TRANSLATE_X, LV_STYLE_TRANSLATE_Y, 0
    };
    lv_style_transition_dsc_init(&controller->tr_detail, props, lv_anim_path_ease_out, 300, 0, NULL);
    lv_style_transition_dsc_init(&controller->tr_nav, props, lv_anim_path_ease_out, 350, 0, NULL);
}


static void launcher_view_init(lv_obj_controller_t *self, lv_obj_t *view) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    pcmanager_register_listener(pcmanager, &pcmanager_callbacks, controller);
    controller->pane_manager = lv_controller_manager_create(controller->detail);
    lv_obj_add_event_cb(controller->nav, cb_nav_focused, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(controller->detail, cb_detail_focused, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(controller->pclist, cb_pc_selected, LV_EVENT_CLICKED, controller);
    lv_obj_add_event_cb(controller->add_btn, open_manual_add, LV_EVENT_CLICKED, controller);
    update_pclist(controller);

    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (cur->selected) {
            select_pc(controller, cur);
            set_detail_opened(controller, true);
            continue;
        }
        pcmanager_request_update(pcmanager, cur->server, NULL, NULL);
    }
    pcmanager_auto_discovery_start(pcmanager);
}

static void launcher_view_destroy(lv_obj_controller_t *self, lv_obj_t *view) {
    pcmanager_auto_discovery_stop(pcmanager);

    launcher_controller_t *controller = (launcher_controller_t *) self;
    lv_controller_manager_del(controller->pane_manager);
    controller->pane_manager = NULL;
    pcmanager_unregister_listener(pcmanager, &pcmanager_callbacks);
}

static bool launcher_event_cb(lv_obj_controller_t *self, int which, void *data1, void *data2) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    switch (which) {
        case USER_SIZE_CHANGED: {
            lv_obj_set_size(self->obj, ui_display_width, ui_display_height);
            break;
        }
    }
    return lv_controller_manager_dispatch_event(controller->pane_manager, which, data1, data2);
}

void on_pc_added(const pcmanager_resp_t *resp, void *userdata) {
    launcher_controller_t *controller = userdata;
    if (!resp->server) return;
    PSERVER_LIST cur = NULL;
    for (cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        const SERVER_DATA *server = cur->server;
        if (server == resp->server) {
            break;
        }
    }
    if (!cur) return;
    pclist_item_create(controller, cur);
}

void on_pc_updated(const pcmanager_resp_t *resp, void *userdata) {
    launcher_controller_t *controller = userdata;
    if (!resp->server) return;
    for (uint16_t i = 0, j = lv_obj_get_child_cnt(controller->pclist); i < j; i++) {
        lv_obj_t *child = lv_obj_get_child(controller->pclist, i);
        SERVER_LIST *cur = lv_obj_get_user_data(child);
        if (resp->server == cur->server) {
            const char *icon = server_item_icon(cur);
            lv_btn_set_icon(child, icon);
            break;
        }
    }
}

void on_pc_removed(const pcmanager_resp_t *resp, void *userdata) {
    launcher_controller_t *controller = userdata;
    if (!resp->server) return;
    for (uint16_t i = 0, j = lv_obj_get_child_cnt(controller->pclist); i < j; i++) {
        lv_obj_t *child = lv_obj_get_child(controller->pclist, i);
        SERVER_LIST *cur = lv_obj_get_user_data(child);
        if (resp->server == cur->server) {
            lv_obj_del(child);
            break;
        }
    }
}

static void cb_pc_selected(lv_event_t *event) {
    struct _lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) return;
    launcher_controller_t *controller = lv_event_get_user_data(event);
    PSERVER_LIST selected = lv_obj_get_user_data(target);
    if (selected->state.code == SERVER_STATE_ONLINE && !selected->server->paired) {
        open_pair(controller, selected);
        return;
    }
    set_detail_opened(controller, true);
    if (selected->selected) return;
    select_pc(controller, selected);
}

static void select_pc(launcher_controller_t *controller, PSERVER_LIST selected) {
    lv_controller_manager_replace(controller->pane_manager, &apps_controller_class, selected);
    uint32_t pclen = lv_obj_get_child_cnt(controller->pclist);
    for (int i = 0; i < pclen; i++) {
        lv_obj_t *pcitem = lv_obj_get_child(controller->pclist, i);
        PSERVER_LIST cur = (PSERVER_LIST) lv_obj_get_user_data(pcitem);
        cur->selected = cur == selected;
        if (!cur->selected) {
            lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        }
    }
}

static void update_pclist(launcher_controller_t *controller) {
    lv_obj_clean(controller->pclist);
    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        lv_obj_t *pcitem = pclist_item_create(controller, cur);

        if (!cur->selected) {
            lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        }
    }
}

static lv_obj_t *pclist_item_create(launcher_controller_t *controller, PSERVER_LIST cur) {
    const SERVER_DATA *server = cur->server;
    const char *icon = server_item_icon(cur);
    lv_obj_t *pcitem = lv_list_add_btn(controller->pclist, icon, server->hostname);
    lv_obj_add_flag(pcitem, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_icon_font(pcitem, LV_ICON_FONT_DEFAULT);
    lv_obj_set_style_bg_color(pcitem, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
    lv_obj_set_user_data(pcitem, cur);
    return pcitem;
}

static const char *server_item_icon(const SERVER_LIST *node) {
    switch (node->state.code) {
        case SERVER_STATE_NONE:
        case SERVER_STATE_QUERYING:
            return MAT_SYMBOL_TV;
        case SERVER_STATE_ONLINE:
            if (!node->server->paired) {
                return MAT_SYMBOL_LOCK;
            } else if (node->server->currentGame) {
                return MAT_SYMBOL_ONDEMAND_VIDEO;
            } else {
                return MAT_SYMBOL_TV;
            }
        case SERVER_STATE_ERROR:
        case SERVER_STATE_OFFLINE:
            return MAT_SYMBOL_WARNING;
        default:
            return MAT_SYMBOL_TV;
    }
}

static void cb_detail_focused(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    if (lv_obj_get_parent(event->target) != controller->detail) return;
    set_detail_opened(controller, true);
}

static void cb_nav_focused(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = event->target;
    while (target && target != controller->nav) {
        target = lv_obj_get_parent(target);
    }
    if (!target) return;
    set_detail_opened(controller, false);
}

static void set_detail_opened(launcher_controller_t *controller, bool opened) {
    if (opened) {
        lv_obj_add_state(controller->detail, LV_STATE_USER_1);
    } else {
        lv_obj_clear_state(controller->detail, LV_STATE_USER_1);
    }
}

/** Pairing functions */

static void open_pair(launcher_controller_t *controller, PSERVER_LIST node) {
    lv_controller_manager_show(controller->pane_manager, &pair_dialog_class, node);
}


static void open_manual_add(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    lv_controller_manager_show(controller->pane_manager, &add_dialog_class, NULL);
}
