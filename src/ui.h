#ifndef UI_H
#define UI_H

#include <windows.h>

/* Dimensões da janela — usadas por login.c e net.c para posicionar painéis */
#define WIN_W 640
#define WIN_H 480

/*
    Cria a janela Win32, inicializa o Nuklear e executa o loop principal.
    Bloqueia até o usuário fechar a janela.

    Args:
        hInst:       handle da instância do executável.
        h_read:      pipe de leitura do worker --backend.
        h_write:     pipe de escrita para o worker --backend.
        h_net_read:  pipe de leitura do worker --net.
        h_net_write: pipe de escrita para o worker --net.

    Returns: 0 em encerramento normal.
*/
int run_ui(HINSTANCE hInst,
           HANDLE h_read,  HANDLE h_write,
           HANDLE h_net_read, HANDLE h_net_write);

#endif
