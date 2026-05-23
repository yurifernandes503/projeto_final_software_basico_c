/*
 * ui.c — janela Win32 e loop principal Nuklear.
 *
 * Responsabilidade única: criar a janela, inicializar o Nuklear e
 * rodar o loop de mensagens. O desenho de cada tela fica no módulo
 * que owna aquela funcionalidade: login.c desenha login/cadastro,
 * net.c desenha o chat e faz o poll dos workers.
 *
 * NK_IMPLEMENTATION e NK_GDI_IMPLEMENTATION são definidos AQUI e apenas aqui.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "../libs/nuklear.h"
#include "../libs/nuklear_gdi.h"

#include "ui.h"
#include "login.h"
#include "net.h"

static struct nk_context *ctx;
static GdiFont            *font;


static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (nk_gdi_handle_event(hwnd, msg, wp, lp)) return 0;
    return DefWindowProcW(hwnd, msg, wp, lp);
}


int run_ui(HINSTANCE hInst,
           HANDLE h_read,  HANDLE h_write,
           HANDLE h_net_read, HANDLE h_net_write) {

    net_ui_init(h_read, h_write, h_net_read, h_net_write);

    WNDCLASSW wc = {0};
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"POCChat";
    RegisterClassW(&wc);

    RECT r = {0, 0, WIN_W, WIN_H};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowExW(
        0, L"POCChat", L"Projeto Final \x2014 Win32 + Nuklear GDI",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);

    HDC dc = GetDC(hwnd);
    font = nk_gdifont_create("Consolas", 14);
    ctx  = nk_gdi_init(font, dc, WIN_W, WIN_H);

    MSG win_msg;
    while (1) {
        nk_input_begin(ctx);
        while (PeekMessageW(&win_msg, NULL, 0, 0, PM_REMOVE)) {
            if (win_msg.message == WM_QUIT) goto shutdown;
            TranslateMessage(&win_msg);
            DispatchMessageW(&win_msg);
        }
        nk_input_end(ctx);

        if (app_state == STATE_CHAT) {
            poll_backend();
            poll_net();
            desenhar_chat(ctx);
        } else {
            desenhar_login(ctx);
        }

        nk_gdi_render(nk_rgb(25, 25, 30));
        Sleep(16);
    }

shutdown:
    nk_gdifont_del(font);
    nk_gdi_shutdown();
    ReleaseDC(hwnd, dc);
    return 0;
}
