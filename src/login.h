#ifndef LOGIN_H
#define LOGIN_H

#include "../database/db.h"

/* Forward declaration — evita incluir nuklear.h neste header */
struct nk_context;

/* Estado global da aplicação — controla qual tela o loop Nuklear renderiza */
typedef enum {
    STATE_LOGIN,   /* tela de entrar com login e senha  */
    STATE_CREATE,  /* tela de criar nova conta           */
    STATE_CHAT     /* tela principal do chat             */
} AppState;

/* Variáveis globais definidas em login.c, lidas por ui.c e net.c */
extern AppState      app_state;
extern UsuarioLogado g_usuario_logado;

/* Buffers dos campos do formulário — o Nuklear escreve aqui via nk_edit_string */
extern char login_buf[MAX_LOGIN + 1];
extern int  login_len;
extern char senha_buf[MAX_SENHA + 1];
extern int  senha_len;
extern char confirmar_buf[MAX_SENHA + 1];
extern int  confirmar_len;
extern char erro_msg[128];

/*
    Renderiza a tela de login ou cadastro de acordo com app_state.
    Chamada por ui.c a cada frame enquanto app_state != STATE_CHAT.
*/
void desenhar_login(struct nk_context *ctx);

/*
    Valida os campos de login e chama db_login.
    Atualiza app_state (→ STATE_CHAT) e erro_msg conforme resultado.
*/
void login_tentar_entrar(void);

/*
    Valida os campos de cadastro (senhas iguais, campos preenchidos)
    e chama db_criar_conta.
    Atualiza app_state (→ STATE_LOGIN) e erro_msg conforme resultado.
*/
void login_tentar_cadastrar(void);

/*
    Limpa os campos e muda para STATE_CREATE.
*/
void login_ir_para_cadastro(void);

/*
    Limpa os campos e muda para STATE_LOGIN.
*/
void login_ir_para_login(void);

#endif
