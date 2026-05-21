/*
 * main.c — ponto de entrada e orquestração de processos.
 *
 * Detecta o modo de execução pelo argv[1] e delega:
 *   (sem args)  → abre banco, exibe UI (login → chat)
 *   --backend   → loop de echo simulado
 *   --net       → servidor/cliente TCP Winsock2
 *
 * Todo o código de interface gráfica está em ui.c.
 * Toda a lógica de login/cadastro está em login.c.
 * Todo o acesso ao banco está em database/db.c.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

#include "orquestrador.h"
#include "net.h"
#include "ui.h"
#include "../database/db.h"


/*
    Loop do processo --backend. Lê mensagens da UI e devolve uma resposta simulada.
*/
static void run_backend(HANDLE h_read, HANDLE h_write) {
    char buf[512];
    char resp[600];
    DWORD avail, bytes_read, bytes_written;

    while (1) {
        Sleep(100);

        avail = 0;
        if (!PeekNamedPipe(h_read, NULL, 0, NULL, &avail, NULL)) break;
        if (avail == 0) continue;

        if (ReadFile(h_read, buf, sizeof(buf) - 1, &bytes_read, NULL) && bytes_read > 0) {
            buf[bytes_read] = '\0';
            _snprintf(resp, sizeof(resp), "[Sistema] Backend roteando: %s\n", buf);
            WriteFile(h_write, resp, (DWORD)strlen(resp), &bytes_written, NULL);
        }
    }

    CloseHandle(h_read);
    CloseHandle(h_write);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;

    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    /* Modo --backend */
    if (argc >= 4 && lstrcmpW(argv[1], L"--backend") == 0) {
        HANDLE h_read  = (HANDLE)(UINT_PTR)wcstoull(argv[2], NULL, 10);
        HANDLE h_write = (HANDLE)(UINT_PTR)wcstoull(argv[3], NULL, 10);
        LocalFree(argv);
        run_backend(h_read, h_write);
        return 0;
    }

    /* Modo --net */
    if (argc >= 4 && lstrcmpW(argv[1], L"--net") == 0) {
        HANDLE h_read  = (HANDLE)(UINT_PTR)wcstoull(argv[2], NULL, 10);
        HANDLE h_write = (HANDLE)(UINT_PTR)wcstoull(argv[3], NULL, 10);
        LocalFree(argv);
        run_net(h_read, h_write);
        return 0;
    }

    LocalFree(argv);

    /* Modo UI: abre o banco antes de qualquer outra coisa */
    if (db_init() != DB_OK) {
        MessageBoxW(NULL, L"Falha ao inicializar o banco de dados.",
                    L"Erro", MB_ICONERROR);
        return 1;
    }

    /* Lança os workers de backend e rede */
    int backend_slot = orq_launch(L"--backend");
    if (backend_slot == -1) { db_fechar(); return 1; }

    int net_slot = orq_launch(L"--net");
    if (net_slot == -1) { orq_shutdown_all(); db_fechar(); return 1; }

    /* Abre a janela — bloqueia até o usuário fechar */
    int result = run_ui(hInst,
                        workers[backend_slot].pipe_read,
                        workers[backend_slot].pipe_write,
                        workers[net_slot].pipe_read,
                        workers[net_slot].pipe_write);

    orq_shutdown_all();
    db_fechar();
    return result;
}
