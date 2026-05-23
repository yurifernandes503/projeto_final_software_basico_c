#include <winsock2.h>
#include <ws2tcpip.h>
#include "net.h"
#include "login.h"
#include "ui.h"
#include "../libs/nuklear.h"
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define LOG_MAX 8192

/* ── Estado da UI do chat ─────────────────────────────────────────────────── */

static HANDLE g_read;
static HANDLE g_write;
static HANDLE g_net_read;
static HANDLE g_net_write;

static char net_status[64]  = "Desconectado";
static char chat_log[LOG_MAX];
static int  chat_log_len    = 0;
static char msg_buf[256];
static int  msg_len         = 0;
static char ip_buf[64]      = "192.168.0.";
static int  ip_len          = 9;


/*
    Guarda os handles de pipe e inicializa o comprimento do campo de IP.
    Chamada uma vez por run_ui antes do loop principal.
*/
void net_ui_init(HANDLE h_read, HANDLE h_write,
                 HANDLE h_net_read, HANDLE h_net_write) {
    g_read      = h_read;
    g_write     = h_write;
    g_net_read  = h_net_read;
    g_net_write = h_net_write;
    ip_len = (int)strlen(ip_buf);
}


/*
    Adiciona texto ao log do chat com descarte circular:
    quando o buffer enche, a metade mais antiga é descartada.
*/
void log_append(const char *text) {
    int n = (int)strlen(text);

    if (chat_log_len + n + 1 > LOG_MAX) {
        int half = LOG_MAX / 2;
        memmove(chat_log, chat_log + half, chat_log_len - half);
        chat_log_len -= half;
    }

    int to_copy = n;
    if (chat_log_len + to_copy + 1 > LOG_MAX)
        to_copy = LOG_MAX - chat_log_len - 1;

    if (to_copy > 0) {
        memcpy(chat_log + chat_log_len, text, to_copy);
        chat_log_len += to_copy;
        chat_log[chat_log_len] = '\0';
    }
}


/*
    Lê respostas do worker --backend sem bloquear. Chamada a cada frame.
*/
void poll_backend(void) {
    DWORD avail = 0;
    if (!PeekNamedPipe(g_read, NULL, 0, NULL, &avail, NULL) || avail == 0)
        return;

    char buf[512];
    DWORD bytes_read = 0;
    if (ReadFile(g_read, buf, sizeof(buf) - 1, &bytes_read, NULL) && bytes_read > 0) {
        buf[bytes_read] = '\0';
        log_append(buf);
    }
}


/*
    Lê eventos do worker --net sem bloquear. Atualiza log e status de rede.
*/
void poll_net(void) {
    DWORD avail = 0;
    if (!PeekNamedPipe(g_net_read, NULL, 0, NULL, &avail, NULL) || avail == 0)
        return;

    char buf[512];
    DWORD bytes_read = 0;
    if (!ReadFile(g_net_read, buf, sizeof(buf) - 1, &bytes_read, NULL) || bytes_read == 0)
        return;

    buf[bytes_read] = '\0';

    if (strcmp(buf, EVT_LISTENING) == 0) {
        strncpy(net_status, "Aguardando...", sizeof(net_status) - 1);
        log_append("[Rede] Servidor ativo, aguardando conexao.\n");

    } else if (strncmp(buf, EVT_CONNECTED, strlen(EVT_CONNECTED)) == 0) {
        const char *ip = buf + strlen(EVT_CONNECTED);
        _snprintf(net_status, sizeof(net_status), "Conectado: %s", ip);
        char line[128];
        _snprintf(line, sizeof(line), "[Rede] Conectado a %s.\n", ip);
        log_append(line);

    } else if (strncmp(buf, EVT_MSG, strlen(EVT_MSG)) == 0) {
        const char *msg = buf + strlen(EVT_MSG);
        char line[600];
        _snprintf(line, sizeof(line), "%s\n", msg);
        log_append(line);

    } else if (strcmp(buf, EVT_DISCONNECTED) == 0) {
        strncpy(net_status, "Desconectado", sizeof(net_status) - 1);
        log_append("[Rede] Peer encerrou a conexao.\n");

    } else if (strncmp(buf, EVT_ERROR, strlen(EVT_ERROR)) == 0) {
        const char *err = buf + strlen(EVT_ERROR);
        char line[600];
        _snprintf(line, sizeof(line), "[Rede] Erro: %s\n", err);
        log_append(line);
    }
}


/*
    Renderiza a tela principal do chat: painel de rede, log e campo de envio.
*/
void desenhar_chat(struct nk_context *ctx) {
    if (nk_begin(ctx, "Chat",
                 nk_rect(0, 0, WIN_W, WIN_H),
                 NK_WINDOW_NO_SCROLLBAR)) {

        /* ── Painel de rede ── */
        nk_layout_row_begin(ctx, NK_STATIC, 28, 4);

        nk_layout_row_push(ctx, 160);
        nk_label(ctx, net_status, NK_TEXT_LEFT);

        nk_layout_row_push(ctx, 140);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, ip_buf, &ip_len,
                       (int)sizeof(ip_buf) - 1, nk_filter_default);

        nk_layout_row_push(ctx, 90);
        if (nk_button_label(ctx, "Aguardar")) {
            DWORD w;
            WriteFile(g_net_write, CMD_LISTEN, (DWORD)strlen(CMD_LISTEN), &w, NULL);
        }

        nk_layout_row_push(ctx, 90);
        if (nk_button_label(ctx, "Conectar")) {
            ip_buf[ip_len] = '\0';
            char cmd[128];
            _snprintf(cmd, sizeof(cmd), CMD_CONNECT "%s", ip_buf);
            DWORD w;
            WriteFile(g_net_write, cmd, (DWORD)strlen(cmd), &w, NULL);
        }

        nk_layout_row_end(ctx);

        /* ── Separador ── */
        nk_layout_row_dynamic(ctx, 1, 1);
        nk_rule_horizontal(ctx, nk_rgb(60, 60, 70), nk_false);

        /* ── Log do chat ── */
        nk_layout_row_dynamic(ctx, WIN_H - 28 - 1 - 40 - 36, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY,
                                       chat_log, LOG_MAX, nk_filter_default);

        /* ── Campo de envio ── */
        nk_layout_row_begin(ctx, NK_STATIC, 32, 2);

        nk_layout_row_push(ctx, WIN_W - 92 - 16);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, msg_buf, &msg_len,
                       (int)sizeof(msg_buf) - 1, nk_filter_default);

        nk_layout_row_push(ctx, 82);
        if (nk_button_label(ctx, "Enviar")) {
            if (msg_len > 0) {
                msg_buf[msg_len] = '\0';

                /* Prefixo com o apelido do usuário logado */
                char net_cmd[400];
                _snprintf(net_cmd, sizeof(net_cmd),
                          CMD_SEND "%s: %s", g_usuario_logado.apelido, msg_buf);
                DWORD w;
                WriteFile(g_net_write, net_cmd, (DWORD)strlen(net_cmd), &w, NULL);

                char echo[400];
                _snprintf(echo, sizeof(echo), "%s: %s\n",
                          g_usuario_logado.apelido, msg_buf);
                log_append(echo);

                memset(msg_buf, 0, sizeof(msg_buf));
                msg_len = 0;
            }
        }

        nk_layout_row_end(ctx);
    }
    nk_end(ctx);
}


/* ── Processo worker --net ────────────────────────────────────────────────── */

/* Estados internos da máquina de estado do processo de rede */
typedef enum { NET_IDLE, NET_LISTENING, NET_CONNECTED } NetState;


/*
    Lê uma linha do pipe de comandos sem bloquear.
    Retorna 1 se leu algo, 0 se o pipe está vazio, -1 se foi fechado.
*/
static int read_cmd(HANDLE h_read, char *buf, int buf_sz) {
    DWORD avail = 0;
    if (!PeekNamedPipe(h_read, NULL, 0, NULL, &avail, NULL)) return -1;
    if (avail == 0) return 0;

    DWORD bytes_read = 0;
    if (!ReadFile(h_read, buf, buf_sz - 1, &bytes_read, NULL) || bytes_read == 0)
        return -1;

    buf[bytes_read] = '\0';
    /* Remove '\n' final se houver */
    int len = (int)bytes_read;
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
    return 1;
}


/*
    Envia um evento de volta para a UI pelo pipe de escrita.
*/
static void send_evt(HANDLE h_write, const char *evt) {
    DWORD written;
    WriteFile(h_write, evt, (DWORD)strlen(evt), &written, NULL);
}


/*
    Configura um socket como não-bloqueante usando ioctlsocket.
    Equivale ao fcntl(fd, F_SETFL, O_NONBLOCK) do Linux.
*/
static void set_nonblocking(SOCKET s) {
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
}


void run_net(HANDLE h_read, HANDLE h_write) {
    /* Inicializa Winsock — obrigatório antes de qualquer chamada de socket */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        send_evt(h_write, EVT_ERROR "WSAStartup falhou");
        return;
    }

    NetState state   = NET_IDLE;
    SOCKET   srv     = INVALID_SOCKET; /* socket servidor (modo LISTEN) */
    SOCKET   peer    = INVALID_SOCKET; /* socket da conexão ativa */

    char cmd[512];
    char evt[600];
    char recv_buf[512];

    while (1) {
        Sleep(50); /* throttle: 20 verificações/segundo sem busy-wait */

        /* Lê comando vindo da UI */
        int r = read_cmd(h_read, cmd, sizeof(cmd));
        if (r == -1) break; /* pipe fechado → encerra */

        if (r == 1) {
            /* CMD: LISTEN — abre servidor TCP na porta NET_PORT */
            if (strcmp(cmd, CMD_LISTEN) == 0 && state == NET_IDLE) {
                srv = socket(AF_INET, SOCK_STREAM, 0);
                if (srv == INVALID_SOCKET) {
                    send_evt(h_write, EVT_ERROR "socket() falhou");
                    continue;
                }

                /* SO_REUSEADDR evita "Address already in use" ao reiniciar rápido */
                int opt = 1;
                setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

                struct sockaddr_in addr = {0};
                addr.sin_family      = AF_INET;
                addr.sin_addr.s_addr = INADDR_ANY;
                addr.sin_port        = htons(NET_PORT);

                if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) != 0 ||
                    listen(srv, 1) != 0) {
                    send_evt(h_write, EVT_ERROR "bind/listen falhou");
                    closesocket(srv);
                    srv = INVALID_SOCKET;
                    continue;
                }

                set_nonblocking(srv); /* accept não vai bloquear o loop */
                state = NET_LISTENING;
                send_evt(h_write, EVT_LISTENING);

            /* CMD: CONNECT:<ip> — conecta como cliente a outro peer */
            } else if (strncmp(cmd, CMD_CONNECT, strlen(CMD_CONNECT)) == 0 && state == NET_IDLE) {
                const char *ip = cmd + strlen(CMD_CONNECT);

                peer = socket(AF_INET, SOCK_STREAM, 0);
                if (peer == INVALID_SOCKET) {
                    send_evt(h_write, EVT_ERROR "socket() falhou");
                    continue;
                }

                struct sockaddr_in addr = {0};
                addr.sin_family = AF_INET;
                addr.sin_port   = htons(NET_PORT);
                inet_pton(AF_INET, ip, &addr.sin_addr);

                /* connect é bloqueante aqui — aceitável porque é chamado sob demanda */
                if (connect(peer, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
                    _snprintf(evt, sizeof(evt), EVT_ERROR "connect falhou para %s", ip);
                    send_evt(h_write, evt);
                    closesocket(peer);
                    peer = INVALID_SOCKET;
                    continue;
                }

                set_nonblocking(peer);
                state = NET_CONNECTED;
                _snprintf(evt, sizeof(evt), EVT_CONNECTED "%s", ip);
                send_evt(h_write, evt);

            /* CMD: SEND:<texto> — envia mensagem ao peer conectado */
            } else if (strncmp(cmd, CMD_SEND, strlen(CMD_SEND)) == 0 && state == NET_CONNECTED) {
                const char *msg = cmd + strlen(CMD_SEND);
                send(peer, msg, (int)strlen(msg), 0);
            }
        }

        /* Modo servidor: tenta aceitar conexão de entrada */
        if (state == NET_LISTENING && srv != INVALID_SOCKET) {
            struct sockaddr_in cli_addr = {0};
            int cli_len = sizeof(cli_addr);
            SOCKET incoming = accept(srv, (struct sockaddr*)&cli_addr, &cli_len);

            if (incoming != INVALID_SOCKET) {
                peer = incoming;
                set_nonblocking(peer);

                char ip_str[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &cli_addr.sin_addr, ip_str, sizeof(ip_str));

                closesocket(srv); /* aceita apenas um peer por sessão */
                srv = INVALID_SOCKET;

                state = NET_CONNECTED;
                _snprintf(evt, sizeof(evt), EVT_CONNECTED "%s", ip_str);
                send_evt(h_write, evt);
            }
        }

        /* Modo conectado: verifica se há mensagem recebida do peer */
        if (state == NET_CONNECTED && peer != INVALID_SOCKET) {
            int n = recv(peer, recv_buf, sizeof(recv_buf) - 1, 0);

            if (n > 0) {
                recv_buf[n] = '\0';
                _snprintf(evt, sizeof(evt), EVT_MSG "%s", recv_buf);
                send_evt(h_write, evt);

            } else if (n == 0 || (n == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
                /* peer encerrou a conexão */
                closesocket(peer);
                peer  = INVALID_SOCKET;
                state = NET_IDLE;
                send_evt(h_write, EVT_DISCONNECTED);
            }
            /* WSAEWOULDBLOCK = socket não-bloqueante sem dados: ignora */
        }
    }

    /* Limpeza */
    if (peer != INVALID_SOCKET) closesocket(peer);
    if (srv  != INVALID_SOCKET) closesocket(srv);
    WSACleanup();
    CloseHandle(h_read);
    CloseHandle(h_write);
}
