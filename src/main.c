/* nuklear - 1.40.8 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_WL_EGL_GLES2_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_wayland_egl.h"
#include <wayland-webos-shell-client-protocol.h>

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define DTIME 16

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define UNUSED(a) (void)a
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a) / sizeof(a)[0])

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the include
 * and the corresponding function. */
/*#include "../style.c"*/
/*#include "../calculator.c"*/
/*#include "../overview.c"*/
/*#include "../node_editor.c"*/

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/

struct wl_surface *g_pstSurface = NULL;
struct wl_shell *g_pstShell = NULL;
struct wl_shell_surface *g_pstShellSurface = NULL;
struct wl_webos_shell *g_pstWebOSShell = NULL;
struct wl_webos_shell_surface *g_pstWebosShellSurface = NULL;
struct wl_egl_window *g_pstEglWindow = NULL;

EGLDisplay g_pstEglDisplay = NULL;
EGLConfig g_pstEglConfig = NULL;
EGLSurface g_pstEglSurface = NULL;
EGLContext g_pstEglContext = NULL;

static const char APPID[] = "org.webosbrew.sample.ndl-directmedia";

static int exit_requested_;

static void finalize();

//WAYLAND POINTER INTERFACE (mouse/touchpad)
static void nk_wayland_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

static void nk_wayland_pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
}

static void nk_wayland_pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;
    win->mouse_pointer_x = wl_fixed_to_int(x);
    win->mouse_pointer_y = wl_fixed_to_int(y);

    // fprintf(stderr, "pointer motion: %d,%d \n", win->mouse_pointer_x, win->mouse_pointer_y);

    nk_input_motion(&(win->ctx), win->mouse_pointer_x, win->mouse_pointer_y);
}

static void nk_wayland_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;
    fprintf(stderr, "pointer button: %d, pressed: %d \n", button, state);

    if (button == 272)
    { //left mouse button
        if (state == WL_POINTER_BUTTON_STATE_PRESSED)
        {
            // printf("nk_input_button x=%d, y=%d press: 1 \n", win->mouse_pointer_x, win->mouse_pointer_y);
            nk_input_button(&(win->ctx), NK_BUTTON_LEFT, win->mouse_pointer_x, win->mouse_pointer_y, 1);
        }
        else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
        {
            nk_input_button(&(win->ctx), NK_BUTTON_LEFT, win->mouse_pointer_x, win->mouse_pointer_y, 0);
        }
    }
}

static void nk_wayland_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static struct wl_pointer_listener nk_wayland_pointer_listener =
    {
        &nk_wayland_pointer_enter,
        &nk_wayland_pointer_leave,
        &nk_wayland_pointer_motion,
        &nk_wayland_pointer_button,
        &nk_wayland_pointer_axis};
//-------------------------------------------------------------------- endof WAYLAND POINTER INTERFACE

//WAYLAND KEYBOARD INTERFACE
static void nk_wayland_keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size)
{
}

static void nk_wayland_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
}

static void nk_wayland_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
}

static void nk_wayland_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    fprintf(stderr, "key: %d \n", key);
}

static void nk_wayland_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static struct wl_keyboard_listener nk_wayland_keyboard_listener =
    {
        &nk_wayland_keyboard_keymap,
        &nk_wayland_keyboard_enter,
        &nk_wayland_keyboard_leave,
        &nk_wayland_keyboard_key,
        &nk_wayland_keyboard_modifiers};
//-------------------------------------------------------------------- endof WAYLAND KEYBOARD INTERFACE

//WAYLAND SEAT INTERFACE
static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        struct wl_pointer *pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &nk_wayland_pointer_listener, win);
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(keyboard, &nk_wayland_keyboard_listener, win);
    }
}

static struct wl_seat_listener seat_listener =
    {
        &seat_capabilities};
//-------------------------------------------------------------------- endof WAYLAND SEAT INTERFACE

static void registryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;
    if (strcmp(interface, "wl_compositor") == 0)
    {
        win->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        g_pstShell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
    }
    else if (strcmp(interface, "wl_webos_shell") == 0)
    {
        g_pstWebOSShell = wl_registry_bind(registry, id, &wl_webos_shell_interface, 1);
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        win->seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(win->seat, &seat_listener, win);
    }
}

static void registryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
}

static const struct wl_registry_listener s_stRegistryListener = {
    registryHandler,
    registryRemover};

static void webosShellHandleState(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, uint32_t state)
{
    switch (state)
    {
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
        break;
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
        break;
    }
}

static void webosShellHandlePosition(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, int32_t x, int32_t y)
{
}

static void webosShellHandleClose(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface)
{
}

static void webosShellHandleExpose(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, struct wl_array *rectangles)
{
}

static void webosShellHandleStateAboutToChange(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, uint32_t state)
{
}

static const struct wl_webos_shell_surface_listener s_pstWebosShellListener = {
    webosShellHandleState,
    webosShellHandlePosition,
    webosShellHandleClose,
    webosShellHandleExpose,
    webosShellHandleStateAboutToChange};

static void getWaylandServer()
{
    struct wl_registry *pstRegistry = NULL;
    struct nk_wl_egl *win = &wl_egl;

    win->display = wl_display_connect(NULL);
    if (win->display == NULL)
    {
        fprintf(stderr, "ERROR, cannot connect!\n");
        exit(1);
    }

    pstRegistry = wl_display_get_registry(win->display);
    wl_registry_add_listener(pstRegistry, &s_stRegistryListener, &wl_egl);

    wl_display_dispatch(win->display);
    // wait for a synchronous response
    wl_display_roundtrip(win->display);

    if ((&wl_egl)->compositor == NULL || g_pstShell == NULL || g_pstWebOSShell == NULL)
    {
        fprintf(stderr, "ERROR, cannot find compositor / shell\n");
        exit(1);
    }

    g_pstSurface = wl_compositor_create_surface((&wl_egl)->compositor);
    if (g_pstSurface == NULL)
    {
        fprintf(stderr, "ERROR, cannot create surface \n");
        exit(1);
    }

    g_pstShellSurface = wl_shell_get_shell_surface(g_pstShell, g_pstSurface);
    if (g_pstShellSurface == NULL)
    {
        fprintf(stderr, "Can't create shell surface\n");
        exit(1);
    }
    wl_shell_surface_set_toplevel(g_pstShellSurface);

    // Please see wayland-webos-shell-client-protocol.h file for webOS specific wayland protocol
    g_pstWebosShellSurface = wl_webos_shell_get_shell_surface(g_pstWebOSShell, g_pstSurface);
    if (g_pstWebosShellSurface == NULL)
    {
        fprintf(stderr, "Can't create webos shell surface\n");
        exit(1);
    }
    wl_webos_shell_surface_add_listener(g_pstWebosShellSurface, &s_pstWebosShellListener, win->display);
    wl_webos_shell_surface_set_property(g_pstWebosShellSurface, "appId", (getenv("APPID") ? getenv("APPID") : APPID));
    // for secondary display, set the last parameter as 1
    wl_webos_shell_surface_set_property(g_pstWebosShellSurface, "displayAffinity", (getenv("DISPLAY_ID") ? getenv("DISPLAY_ID") : "0"));
    wl_webos_shell_surface_set_property(g_pstWebosShellSurface, "_WEBOS_ACCESS_POLICY_KEYS_BACK", "true");
}

static void createWindow()
{
    struct nk_wl_egl *win = &wl_egl;
    win->width = WINDOW_WIDTH;
    win->height = WINDOW_HEIGHT;

    // webOS only supports full screen size
    g_pstEglWindow = wl_egl_window_create(g_pstSurface, win->width, win->height);

    if (g_pstEglWindow == EGL_NO_SURFACE)
    {
        fprintf(stderr, "ERROR, cannot create wayland egl window\n");
        exit(1);
    }

    g_pstEglSurface = eglCreateWindowSurface(g_pstEglDisplay, g_pstEglConfig, g_pstEglWindow, NULL);

    if (!eglMakeCurrent(g_pstEglDisplay, g_pstEglSurface, g_pstEglSurface, g_pstEglContext))
    {
        fprintf(stderr, "ERROR, cannot make current\n");
    }

    eglSwapInterval(g_pstEglDisplay, 1);
}

static void initEgl()
{
    struct nk_wl_egl *win = &wl_egl;
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    int i;

    EGLint configAttributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0x18,
        EGL_STENCIL_SIZE, 0,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_NONE};

    static const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE};

    g_pstEglDisplay = eglGetDisplay((EGLNativeDisplayType)win->display);
    if (g_pstEglDisplay == EGL_NO_DISPLAY)
    {
        fprintf(stderr, "ERROR, cannot create create egl g_pstDisplay\n");
        exit(1);
    }

    if (eglInitialize(g_pstEglDisplay, &major, &minor) != EGL_TRUE)
    {
        fprintf(stderr, "ERROR, cannot initialize egl g_pstDisplay\n");
        exit(1);
    }
    eglGetConfigs(g_pstEglDisplay, NULL, 0, &count);
    configs = (EGLConfig *)calloc(count, sizeof(EGLConfig));
    eglChooseConfig(g_pstEglDisplay, configAttributes, configs, count, &n);
    // simply choose the first config
    g_pstEglConfig = configs[0];
    g_pstEglContext = eglCreateContext(g_pstEglDisplay, g_pstEglConfig, EGL_NO_CONTEXT, contextAttributes);
}

static void finalize()
{
    struct nk_wl_egl *win = &wl_egl;
    eglDestroyContext(g_pstEglDisplay, g_pstEglContext);
    eglDestroySurface(g_pstEglDisplay, g_pstEglSurface);
    eglTerminate(g_pstEglDisplay);
    wl_display_disconnect(win->display);
}

static long timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000);
}

static void sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t / 1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while (-1 == nanosleep(&req, &req))
        ;
}

static void test_window(struct nk_context *ctx)
{
    /* GUI */
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 200, 200),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                     NK_WINDOW_CLOSABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200)))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200)))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);

        enum
        {
            EASY,
            HARD
        };
        static int op = EASY;
        static int property = 20;
        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            fprintf(stdout, "button pressed\n");
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY))
            op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD))
            op = HARD;
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    }
    nk_end(ctx);
}

static void
MainLoop(void *loopArg)
{
    struct nk_context *ctx = (struct nk_context *)loopArg;
    struct nk_wl_egl *win = &wl_egl;
    test_window(ctx);

    /* -------------- EXAMPLES ---------------- */
    /*calculator(ctx);*/
    /*overview(ctx);*/
    /*node_editor(ctx);*/
    /* ----------------------------------------- */

    /* Draw */
    {
        float bg[4];
        nk_color_fv(bg, nk_rgb(28, 48, 62));
        glViewport(0, 0, win->width, win->height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg[0], bg[1], bg[2], bg[3]);
        /* IMPORTANT: `nk_wl_egl_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
        nk_wl_egl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        eglSwapBuffers(g_pstEglDisplay, g_pstEglSurface);
    }

    //handle wayland stuff (send display to FB & get inputs)
    nk_input_begin(ctx);
    wl_display_dispatch(win->display);
    nk_input_end(ctx);
}

int main(int argc, char *argv[])
{
    long dt;
    long started;
    int running = 1;

    /* GUI */
    struct nk_context *ctx;
    struct nk_wl_egl *win = &wl_egl;

    getWaylandServer();
    initEgl();
    createWindow();

    /* OpenGL setup */
    glViewport(0, 0, win->width, win->height);

    ctx = nk_wl_egl_init(g_pstEglWindow);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        nk_wl_egl_font_stash_begin(&atlas);
        /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
        /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
        /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
        /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
        /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
        /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
        nk_wl_egl_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        /*nk_style_set_font(ctx, &roboto->handle)*/;
    }

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/

    while (running)
    {
        started = timestamp();

        MainLoop((void *)ctx);

        // Timing
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }
    nk_wl_egl_shutdown();
    finalize();
    return 0;
}
