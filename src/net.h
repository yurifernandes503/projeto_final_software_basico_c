#ifndef NET_H
#define NET_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef NET_PORT
#define NET_PORT 7777  /* sobrescrito via -DNET_PORT=xxxx no build */
#endif

/* Forward declaration — evita incluir nuklear.h neste header */
struct nk_context;

/* Comandos: UI → processo --net (via pipe) */
#define CMD_LISTEN   "LISTEN"    /* inicia servidor TCP na porta NET_PORT */
#define CMD_CONNECT  "CONNECT:"  /* conecta a um IP:  CONNECT:192.168.0.5 */
#define CMD_SEND     "SEND:"     /* envia mensagem:   SEND:texto aqui     */

/* Eventos: processo --net → UI (via pipe) */
#define EVT_LISTENING    "LISTENING"     /* servidor ativo, aguardando peer */
#define EVT_CONNECTED    "CONNECTED:"    /* conexão ok:  CONNECTED:192.168.0.5 */
#define EVT_MSG          "MSG:"          /* mensagem recebida: MSG:texto aqui  */
#define EVT_DISCONNECTED "DISCONNECTED"  /* peer encerrou a conexão */
#define EVT_ERROR        "ERROR:"        /* falha: ERROR:descrição */

/*
    Inicializa os handles de pipe e o estado de UI do chat.
    Deve ser chamada por run_ui antes do loop principal.
*/
void net_ui_init(HANDLE h_read, HANDLE h_write,
                 HANDLE h_net_read, HANDLE h_net_write);

/*
    Adiciona texto ao log do chat (buffer circular).
*/
void log_append(const char *text);

/*
    Lê respostas do worker --backend sem bloquear. Chamada a cada frame.
*/
void poll_backend(void);

/*
    Lê eventos do worker --net sem bloquear. Atualiza log e status de rede.
*/
void poll_net(void);

/*
    Renderiza a tela principal do chat (STATE_CHAT).
    Chamada por ui.c a cada frame após o login.
*/
void desenhar_chat(struct nk_context *ctx);

/*
    Ponto de entrada do processo worker --net.
    Gerencia a conexão TCP e retransmite eventos para a UI pelo pipe.

    Args:
        h_read:  lê comandos vindos da UI.
        h_write: escreve eventos de volta para a UI.

    Returns: void. Encerra quando o pipe for fechado.
*/
void run_net(HANDLE h_read, HANDLE h_write);

#endif /* NET_H */
