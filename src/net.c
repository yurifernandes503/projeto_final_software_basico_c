#include <winsock2.h>
#include <ws2tcpip.h>
#include "net.h"
#include "login.h"
#include "ui.h"
#include "../database/db.h"
#include "../libs/nuklear.h"
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define LOG_MAX    8192
#define CONTACTS_W 180  /* largura do painel lateral de contatos */

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

/* Lista de contatos — carregada do banco na primeira vez que o chat é exibido */
static Contato g_contatos[MAX_CONTATOS];
static int     g_n_contatos       = 0;
static int     g_contato_ativo    = -1;   /* índice em g_contatos, -1 = nenhum */
static int     g_contatos_prontos = 0;    /* flag: 0 = ainda não carregou do banco */

/* g_novo[i] = 1 quando o contato i conectou em nós (sem que tenhamos clicado nele) */
static int     g_novo[MAX_CONTATOS];

/* flag que diferencia "eu cliquei" de "alguém conectou em mim" no EVT_CONNECTED */
static int     g_eu_conectei = 0;

/* Campos do formulário de adicionar contato */
static char add_name_buf[MAX_APELIDO + 1] = {0};
static int  add_name_len                   = 0;
static char add_ip_buf[MAX_IP + 1]         = {0};
static int  add_ip_len                     = 0;
static char add_port_buf[8]                = "7777";
static int  add_port_len                   = 4;


/*
    Guarda os handles de pipe. A lista de contatos é carregada depois,
    na primeira chamada de desenhar_chat, quando o usuário já está logado.
*/
void net_ui_init(HANDLE h_read, HANDLE h_write,
                 HANDLE h_net_read, HANDLE h_net_write) {
    g_read      = h_read;
    g_write     = h_write;
    g_net_read  = h_net_read;
    g_net_write = h_net_write;
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
    Lê eventos do worker --net sem bloquear. Atualiza log, status e notificações.
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

    } else if (strncmp(buf, EVT_CONNECTED, strlen(EVT_CONNECTED)) == 0) {
        const char *ip = buf + strlen(EVT_CONNECTED);
        _snprintf(net_status, sizeof(net_status), "Conectado: %s", ip);

        /* Procura o contato pelo IP para carregar o histórico */
        g_contato_ativo = -1;
        for (int i = 0; i < g_n_contatos; i++) {
            if (strcmp(g_contatos[i].ip, ip) == 0) {
                g_contato_ativo = i;
                break;
            }
        }

        /* Contato desconhecido: adiciona automaticamente usando o IP como nome */
        if (g_contato_ativo == -1) {
            if (db_adicionar_contato(g_usuario_logado.id_perfil,
                                     ip, ip, NET_PORT) == DB_OK) {
                db_listar_contatos(g_usuario_logado.id_perfil,
                                   g_contatos, MAX_CONTATOS, &g_n_contatos);
                for (int i = 0; i < g_n_contatos; i++) {
                    if (strcmp(g_contatos[i].ip, ip) == 0) {
                        g_contato_ativo = i;
                        break;
                    }
                }
            }
        }

        /* Se não fui eu que cliquei para conectar, marca * no contato */
        if (!g_eu_conectei && g_contato_ativo >= 0)
            g_novo[g_contato_ativo] = 1;
        g_eu_conectei = 0;

        /* Limpa o log e carrega o histórico do contato, se encontrado */
        chat_log[0]  = '\0';
        chat_log_len = 0;
        if (g_contato_ativo >= 0)
            db_carregar_historico(g_contatos[g_contato_ativo].id_contato, chat_log, LOG_MAX);
        chat_log_len = (int)strlen(chat_log);

        char line[128];
        _snprintf(line, sizeof(line), "[Rede] Conectado a %s.\n", ip);
        log_append(line);

    } else if (strncmp(buf, EVT_MSG, strlen(EVT_MSG)) == 0) {
        const char *full_msg = buf + strlen(EVT_MSG);
        char line[600];
        _snprintf(line, sizeof(line), "%s\n", full_msg);
        log_append(line);

        if (g_contato_ativo >= 0) {
            const char *sep = strstr(full_msg, ": ");
            if (sep) {
                char remetente[MAX_APELIDO + 1] = {0};
                int  rem_len = (int)(sep - full_msg);
                if (rem_len > MAX_APELIDO) rem_len = MAX_APELIDO;
                strncpy(remetente, full_msg, rem_len);

                db_salvar_mensagem(g_contatos[g_contato_ativo].id_contato,
                                   remetente, sep + 2);

                /* Correção de nome para contatos auto-adicionados:
                   Quando alguém nos conecta sem estar na nossa lista, adicionamos o
                   contato usando o IP como apelido provisório (único identificador que
                   temos naquele momento). Ao receber a primeira mensagem — que vem no
                   formato "Lara: oi" — extraímos o remetente e atualizamos o banco.
                   A condição apelido == ip detecta exatamente esse caso provisório. */
                if (strcmp(g_contatos[g_contato_ativo].apelido,
                           g_contatos[g_contato_ativo].ip) == 0 && rem_len > 0) {
                    int id_ativo = g_contatos[g_contato_ativo].id_contato;
                    if (db_atualizar_apelido_contato(id_ativo, remetente) == DB_OK) {
                        db_listar_contatos(g_usuario_logado.id_perfil,
                                           g_contatos, MAX_CONTATOS, &g_n_contatos);
                        /* db_listar_contatos ordena por apelido — o índice do contato
                           pode ter mudado depois do rename, então buscamos pelo id */
                        for (int i = 0; i < g_n_contatos; i++) {
                            if (g_contatos[i].id_contato == id_ativo) {
                                g_contato_ativo = i;
                                break;
                            }
                        }
                    }
                }
            }
        }

    } else if (strncmp(buf, EVT_BUSY, strlen(EVT_BUSY)) == 0) {
        /* O processo --net mantém o servidor aberto mesmo durante uma conversa.
           Quando alguém tenta se conectar nesse estado, o --net rejeita e manda
           EVT_BUSY com o IP do intruso para que a UI avise o usuário. */
        const char *ip = buf + strlen(EVT_BUSY);
        char busy_line[128];
        _snprintf(busy_line, sizeof(busy_line),
                  "[Rede] %s tentou conectar mas voce ja esta em uma conversa.\n", ip);
        log_append(busy_line);

    } else if (strcmp(buf, EVT_DISCONNECTED) == 0) {
        strncpy(net_status, "Aguardando...", sizeof(net_status) - 1);
        log_append("[Rede] Peer encerrou a conexao.\n");

        /* Volta a escutar automaticamente após a desconexão */
        DWORD w;
        WriteFile(g_net_write, CMD_LISTEN, (DWORD)strlen(CMD_LISTEN), &w, NULL);

    } else if (strncmp(buf, EVT_ERROR, strlen(EVT_ERROR)) == 0) {
        const char *err = buf + strlen(EVT_ERROR);
        char line[600];
        _snprintf(line, sizeof(line), "[Rede] Erro: %s\n", err);
        log_append(line);
    }
}


/*
    Renderiza a tela principal do chat: painel de chat à esquerda
    e lista de contatos à direita.
*/
void desenhar_chat(struct nk_context *ctx) {
    /* Carrega os contatos e inicia a escuta automaticamente, uma única vez após o login */
    if (!g_contatos_prontos) {
        db_listar_contatos(g_usuario_logado.id_perfil,
                           g_contatos, MAX_CONTATOS, &g_n_contatos);
        g_contatos_prontos = 1;
        strncpy(net_status, "Aguardando...", sizeof(net_status) - 1);
        DWORD w;
        WriteFile(g_net_write, CMD_LISTEN, (DWORD)strlen(CMD_LISTEN), &w, NULL);
    }

    int chat_w = WIN_W - CONTACTS_W;

    /* ── Painel do chat (esquerda) ── */
    if (nk_begin(ctx, "Chat",
                 nk_rect(0, 0, chat_w, WIN_H),
                 NK_WINDOW_NO_SCROLLBAR)) {

        /* Linha de status da conexão */
        nk_layout_row_dynamic(ctx, 24, 1);
        nk_label(ctx, net_status, NK_TEXT_LEFT);

        /* ── Separador ── */
        nk_layout_row_dynamic(ctx, 1, 1);
        nk_rule_horizontal(ctx, nk_rgb(60, 60, 70), nk_false);

        /* ── Log do chat ── */
        nk_layout_row_dynamic(ctx, WIN_H - 24 - 1 - 40 - 36, 1);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY,
                                       chat_log, LOG_MAX, nk_filter_default);

        /* ── Campo de envio ── */
        nk_layout_row_begin(ctx, NK_STATIC, 32, 2);

        nk_layout_row_push(ctx, chat_w - 92 - 16);
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

                /* Echo local e salva no histórico */
                char echo[400];
                _snprintf(echo, sizeof(echo), "%s: %s\n",
                          g_usuario_logado.apelido, msg_buf);
                log_append(echo);

                if (g_contato_ativo >= 0)
                    db_salvar_mensagem(g_contatos[g_contato_ativo].id_contato,
                                       g_usuario_logado.apelido, msg_buf);

                memset(msg_buf, 0, sizeof(msg_buf));
                msg_len = 0;
            }
        }

        nk_layout_row_end(ctx);
    }
    nk_end(ctx);

    /* ── Painel de contatos (direita) ── */
    if (nk_begin(ctx, "Amigos",
                 nk_rect(chat_w, 0, CONTACTS_W, WIN_H),
                 NK_WINDOW_TITLE | NK_WINDOW_BORDER)) {

        /* Lista de contatos — clicar conecta ao IP:porta do contato */
        for (int i = 0; i < g_n_contatos; i++) {
            /* Monta o label: "* Nome" se há mensagem nova, "Nome" caso contrário */
            char label[MAX_APELIDO + 3];
            if (g_novo[i])
                _snprintf(label, sizeof(label), "* %s", g_contatos[i].apelido);
            else
                strncpy(label, g_contatos[i].apelido, sizeof(label) - 1);

            nk_layout_row_dynamic(ctx, 28, 1);
            if (nk_button_label(ctx, label)) {
                g_novo[i]       = 0;   /* limpa a notificação ao abrir a conversa */
                g_eu_conectei   = 1;   /* fui eu que iniciei — não gera * de volta */
                g_contato_ativo = i;

                /* Carrega histórico e dispara a conexão TCP com a porta do contato */
                chat_log[0]  = '\0';
                chat_log_len = 0;
                db_carregar_historico(g_contatos[i].id_contato, chat_log, LOG_MAX);
                chat_log_len = (int)strlen(chat_log);

                char cmd[128];
                _snprintf(cmd, sizeof(cmd), CMD_CONNECT "%s:%d",
                          g_contatos[i].ip, g_contatos[i].porta);
                DWORD w;
                WriteFile(g_net_write, cmd, (DWORD)strlen(cmd), &w, NULL);
            }
        }

        /* ── Separador ── */
        nk_layout_row_dynamic(ctx, 1, 1);
        nk_rule_horizontal(ctx, nk_rgb(60, 60, 70), nk_false);

        /* ── Formulário de adicionar contato ── */
        nk_layout_row_dynamic(ctx, 18, 1);
        nk_label(ctx, "Nome:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 24, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, add_name_buf, &add_name_len,
                       MAX_APELIDO, nk_filter_default);

        nk_layout_row_dynamic(ctx, 18, 1);
        nk_label(ctx, "IP:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 24, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, add_ip_buf, &add_ip_len,
                       MAX_IP, nk_filter_default);

        nk_layout_row_dynamic(ctx, 18, 1);
        nk_label(ctx, "Porta:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 24, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, add_port_buf, &add_port_len,
                       (int)sizeof(add_port_buf) - 1, nk_filter_decimal);

        nk_layout_row_dynamic(ctx, 28, 1);
        if (nk_button_label(ctx, "Adicionar")) {
            add_name_buf[add_name_len] = '\0';
            add_ip_buf[add_ip_len]     = '\0';
            add_port_buf[add_port_len] = '\0';
            int porta = atoi(add_port_buf);
            if (porta <= 0) porta = NET_PORT;

            if (add_name_len > 0 && add_ip_len > 0) {
                if (db_adicionar_contato(g_usuario_logado.id_perfil,
                                         add_name_buf, add_ip_buf, porta) == DB_OK) {
                    /* Recarrega a lista após inserção */
                    db_listar_contatos(g_usuario_logado.id_perfil,
                                       g_contatos, MAX_CONTATOS, &g_n_contatos);
                    memset(add_name_buf, 0, sizeof(add_name_buf)); add_name_len  = 0;
                    memset(add_ip_buf,   0, sizeof(add_ip_buf));   add_ip_len    = 0;
                    memset(add_port_buf, 0, sizeof(add_port_buf));
                    strncpy(add_port_buf, "7777", sizeof(add_port_buf) - 1);
                    add_port_len = 4;
                }
            }
        }
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
    SOCKET   server     = INVALID_SOCKET; /* socket servidor (modo LISTEN) */
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
                server = socket(AF_INET, SOCK_STREAM, 0);
                if (server == INVALID_SOCKET) {
                    send_evt(h_write, EVT_ERROR "socket() falhou");
                    continue;
                }

                /* SO_REUSEADDR evita "Address already in use" ao reiniciar rápido */
                int opt = 1;                                                    // valor da opção: 1 = ativar (habilita o flag)
                setsockopt(server, SOL_SOCKET, SO_REUSEADDR,                    // aplica a opção SO_REUSEADDR no nível do socket (SOL_SOCKET)
                        (char*)&opt, sizeof(opt));                           // passa o ponteiro pro valor e o tamanho dele

                struct sockaddr_in addr = {0};                                 // struct de endereço IPv4, zerada por completo
                addr.sin_family      = AF_INET;                                // família de endereços: IPv4
                addr.sin_addr.s_addr = INADDR_ANY;                             // escuta em qualquer interface/IP local (0.0.0.0)
                addr.sin_port        = htons(NET_PORT);                         // porta convertida p/ network byte order (big-endian)

                if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) != 0 || // associa o socket ao endereço+porta; != 0 = falhou
                    listen(server, 1) != 0) {                                  // coloca em modo escuta, backlog=1 (1 conexão na fila); != 0 = falhou
                    send_evt(h_write, EVT_ERROR "bind/listen falhou");         // notifica a UI que deu erro no bind ou listen
                    closesocket(server);                                       // fecha o socket pra não vazar o recurso
                    server = INVALID_SOCKET;                                   // marca como inválido (evita usar um handle fechado depois)
                    continue;                                                  // volta pro início do while, não prossegue com esse socket
                }

                set_nonblocking(server); /* accept não vai bloquear o loop */
                state = NET_LISTENING;
                send_evt(h_write, EVT_LISTENING);

            /* CMD: LISTEN quando já estamos escutando — isso acontece quando o peer
               desconecta e a UI manda CMD_LISTEN de novo; como o srv já está aberto,
               basta reconfirmar o estado sem tentar criar outro bind/listen. */
            } else if (strcmp(cmd, CMD_LISTEN) == 0 && state == NET_LISTENING) {
                send_evt(h_write, EVT_LISTENING);

            /* CMD: CONNECT:<ip>:<porta> — conecta como cliente a outro peer.
               Funciona também a partir de NET_LISTENING: fecha o servidor primeiro. */
            } else if (strncmp(cmd, CMD_CONNECT, strlen(CMD_CONNECT)) == 0
                       && (state == NET_IDLE || state == NET_LISTENING)) {

                /* Fecha o servidor se estava escutando */
                if (server != INVALID_SOCKET) {
                    closesocket(server);
                    server = INVALID_SOCKET;
                }
                state = NET_IDLE;

                /* Separa IP e porta pelo último ':' — ex: "192.168.0.5:7777" */
                const char *ip_port = cmd + strlen(CMD_CONNECT);
                char ip[MAX_IP]     = {0}; // buffer p/ guardar só o IP, zerado (garante terminador '\0')
                int  porta          = NET_PORT; // porta padrão, usada se o usuário não informar uma
                const char *last_colon = strrchr(ip_port, ':'); // procura o ÚLTIMO ':' na string (strrchr = search reverse); separa IP da porta
                if (last_colon) {
                    int ip_len = (int)(last_colon - ip_port);
                    if (ip_len >= MAX_IP) ip_len = MAX_IP - 1; // trava o tamanho p/ não estourar o buffer (deixa espaço p/ '\0')
                    strncpy(ip, ip_port, ip_len); // copia só a parte do IP (até antes do ':') p/ o buffer
                    porta = atoi(last_colon + 1); // converte o texto após o ':' em número (atoi = ASCII to int)
                    if (porta <= 0) porta = NET_PORT;
                } else {                           // não achou ':', veio só o IP, sem porta
                    strncpy(ip, ip_port, MAX_IP - 1); // copia o IP inteiro (limitado p/ não estourar o buffer)
                }

                peer = socket(AF_INET, SOCK_STREAM, 0); // cria o socket cliente: IPv4 (AF_INET), TCP (SOCK_STREAM)
                if (peer == INVALID_SOCKET) {
                    send_evt(h_write, EVT_ERROR "socket() falhou");
                    continue;
                }

                // TRANSFORMAR EM MÉTODO DE UTILIDADE, ESSE SET DE STRUCT DE CONEXÃO É
                // UTILIADO DIVERSAS VEZES NO CÓDIGO
                struct sockaddr_in addr = {0}; // struct de endereço IPv4 do destino, zerada
                addr.sin_family = AF_INET;
                addr.sin_port   = htons((u_short)porta); // USA A PORTA aqui: converte p/ network byte order (cast p/ u_short = 16 bits)
                inet_pton(AF_INET, ip, &addr.sin_addr); // USA O IP aqui: converte o texto "192.168.0.5" p/ binário e grava em addr.sin_addr

                /* connect é bloqueante aqui — aceitável porque é chamado sob demanda */
                
                if (connect(peer, (struct sockaddr*)&addr, sizeof(addr)) != 0) { // tenta conectar ao destino montado em addr; != 0 = falhou
                    _snprintf(evt, sizeof(evt), EVT_ERROR "connect falhou para %s:%d", ip, porta);
                    send_evt(h_write, evt);
                    closesocket(peer);      // fecha o socket pra não vazar recurso
                    peer = INVALID_SOCKET;  // marca como inválido
                    continue;               // volta pro while, abandona essa tentativa
                }

                set_nonblocking(peer); // já conectou: torna não-bloqueante p/ não travar o loop nas leituras futuras
                state = NET_CONNECTED; // atualiza a máquina de estados
                _snprintf(evt, sizeof(evt), EVT_CONNECTED "%s", ip); // monta o evento de sucesso com o ip
                send_evt(h_write, evt); // notifica a UI que conectou

            /* CMD: SEND:<texto> — envia mensagem ao peer conectado */
            } else if (strncmp(cmd, CMD_SEND, strlen(CMD_SEND)) == 0 && state == NET_CONNECTED) {
                const char *msg = cmd + strlen(CMD_SEND);
                send(peer, msg, (int)strlen(msg), 0);
            }
        }

        /* Modo servidor: tenta aceitar conexão de entrada */
        if (state == NET_LISTENING && server != INVALID_SOCKET) {
            struct sockaddr_in cli_addr = {0};
            int cli_len = sizeof(cli_addr);
            SOCKET incoming = accept(server, (struct sockaddr*)&cli_addr, &cli_len);

            if (incoming != INVALID_SOCKET) {
                peer = incoming;
                set_nonblocking(peer);

                char ip_str[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &cli_addr.sin_addr, ip_str, sizeof(ip_str));

                /* Não fechamos srv aqui — mantê-lo aberto permite detectar quem
                   tenta conectar enquanto já estamos em conversa (ver bloco EVT_BUSY
                   logo abaixo). Só fechamos srv quando o usuário clica para conectar
                   a outro peer (CMD_CONNECT), pois aí trocamos de papel: saímos de
                   servidor e viramos cliente. */
                state = NET_CONNECTED;
                _snprintf(evt, sizeof(evt), EVT_CONNECTED "%s", ip_str);
                send_evt(h_write, evt);
            }
        }

        /* Detecção de "linha ocupada": enquanto já temos um peer conectado, o srv
           continua escutando. Se alguém tentar se conectar nesse momento, aceitamos
           o socket só para pegar o IP — depois fechamos imediatamente e avisamos a UI
           via EVT_BUSY para ela mostrar a mensagem para o usuário. */
        if (state == NET_CONNECTED && server != INVALID_SOCKET) {
            struct sockaddr_in busy_addr = {0};
            int busy_len = sizeof(busy_addr);
            SOCKET incoming = accept(server, (struct sockaddr*)&busy_addr, &busy_len);
            if (incoming != INVALID_SOCKET) {
                char busy_ip[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &busy_addr.sin_addr, busy_ip, sizeof(busy_ip));
                closesocket(incoming); /* recusa: fecha antes de trocar um byte sequer */
                _snprintf(evt, sizeof(evt), EVT_BUSY "%s", busy_ip);
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
                peer = INVALID_SOCKET;
                /* Dois cenários possíveis ao desconectar:
                   1. srv ainda aberto → fomos servidor (aceitamos a conexão). Volta a
                      NET_LISTENING para aceitar a próxima conversa sem precisar de CMD_LISTEN.
                   2. srv fechado → fomos cliente (CMD_CONNECT fechou o srv antes de conectar).
                      Vai para NET_IDLE; a UI envia CMD_LISTEN ao receber EVT_DISCONNECTED. */
                if (server != INVALID_SOCKET)
                    state = NET_LISTENING;
                else
                    state = NET_IDLE;
                send_evt(h_write, EVT_DISCONNECTED);
            }
            /* WSAEWOULDBLOCK = socket não-bloqueante sem dados: ignora */
        }
    }

    /* Limpeza */
    if (peer != INVALID_SOCKET) closesocket(peer);
    if (server  != INVALID_SOCKET) closesocket(server);
    WSACleanup();
    CloseHandle(h_read);
    CloseHandle(h_write);
}
