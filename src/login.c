#include "login.h"
#include "ui.h"
#include "../database/db.h"
#include "../libs/nuklear.h"
#include <string.h>

/* Estado e identidade do usuário após o login */
AppState      app_state        = STATE_LOGIN;
UsuarioLogado g_usuario_logado = {0};

/* Buffers compartilhados com ui.c — o Nuklear escreve aqui via nk_edit_string */
char login_buf[MAX_LOGIN + 1]     = {0};
int  login_len                    = 0;
char senha_buf[MAX_SENHA + 1]     = {0};
int  senha_len                    = 0;
char confirmar_buf[MAX_SENHA + 1] = {0};
int  confirmar_len                = 0;
char erro_msg[128]                = {0};


/*
    Zera todos os campos e a mensagem de erro.
    Chamada internamente ao trocar de tela para não vazar dados.
*/
static void limpar_campos(void) {
    memset(login_buf,     0, sizeof(login_buf));
    memset(senha_buf,     0, sizeof(senha_buf));
    memset(confirmar_buf, 0, sizeof(confirmar_buf));
    login_len = senha_len = confirmar_len = 0;
    erro_msg[0] = '\0';
}


void login_tentar_entrar(void) {
    erro_msg[0] = '\0';

    /* Garante terminadores antes de passar para o banco */
    login_buf[login_len] = '\0';
    senha_buf[senha_len] = '\0';

    if (login_len == 0 || senha_len == 0) {
        strncpy(erro_msg, "Preencha todos os campos.", sizeof(erro_msg) - 1);
        return;
    }

    int r = db_login(login_buf, senha_buf, &g_usuario_logado);
    if (r == DB_OK) {
        limpar_campos();
        app_state = STATE_CHAT;
    } else {
        strncpy(erro_msg, "Usuario ou senha incorretos.", sizeof(erro_msg) - 1);
    }
}


void login_tentar_cadastrar(void) {
    erro_msg[0] = '\0';

    login_buf[login_len]         = '\0';
    senha_buf[senha_len]         = '\0';
    confirmar_buf[confirmar_len] = '\0';

    if (login_len == 0 || senha_len == 0 || confirmar_len == 0) {
        strncpy(erro_msg, "Preencha todos os campos.", sizeof(erro_msg) - 1);
        return;
    }

    if (strcmp(senha_buf, confirmar_buf) != 0) {
        strncpy(erro_msg, "As senhas nao coincidem.", sizeof(erro_msg) - 1);
        return;
    }

    int r = db_criar_conta(login_buf, senha_buf);
    if (r == DB_OK) {
        limpar_campos();
        /* Volta para o login com confirmação de sucesso */
        strncpy(erro_msg, "Conta criada! Faca o login.", sizeof(erro_msg) - 1);
        app_state = STATE_LOGIN;
    } else if (r == DB_LOGIN_EXISTE) {
        strncpy(erro_msg, "Este usuario ja existe.", sizeof(erro_msg) - 1);
    } else {
        strncpy(erro_msg, "Erro ao criar conta.", sizeof(erro_msg) - 1);
    }
}


void login_ir_para_cadastro(void) {
    limpar_campos();
    app_state = STATE_CREATE;
}


void login_ir_para_login(void) {
    limpar_campos();
    app_state = STATE_LOGIN;
}


/*
    Renderiza a tela de login ou cadastro centralizada na janela.
    Chamada por ui.c a cada frame enquanto app_state != STATE_CHAT.
*/
void desenhar_login(struct nk_context *ctx) {
    int panel_w = 340;
    int panel_h = (app_state == STATE_CREATE) ? 290 : 225;
    int x = (WIN_W - panel_w) / 2;
    int y = (WIN_H - panel_h) / 2;

    const char *titulo = (app_state == STATE_CREATE) ? "Criar Conta" : "Login";

    if (nk_begin(ctx, titulo,
                 nk_rect(x, y, panel_w, panel_h),
                 NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Usuario:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 28, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE,
                       login_buf, &login_len, MAX_LOGIN, nk_filter_default);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Senha:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 28, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE,
                       senha_buf, &senha_len, MAX_SENHA, nk_filter_default);

        /* Campo extra só na tela de cadastro */
        if (app_state == STATE_CREATE) {
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "Confirmar Senha:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 28, 1);
            nk_edit_string(ctx, NK_EDIT_SIMPLE,
                           confirmar_buf, &confirmar_len, MAX_SENHA, nk_filter_default);
        }

        /* Mensagem de erro/sucesso */
        nk_layout_row_dynamic(ctx, 20, 1);
        if (erro_msg[0])
            nk_label_colored(ctx, erro_msg, NK_TEXT_CENTERED, nk_rgb(220, 80, 80));
        else
            nk_spacing(ctx, 1);

        /* Botões dependem da tela atual */
        nk_layout_row_dynamic(ctx, 32, 2);
        if (app_state == STATE_LOGIN) {
            if (nk_button_label(ctx, "Entrar"))      login_tentar_entrar();
            if (nk_button_label(ctx, "Criar Conta")) login_ir_para_cadastro();
        } else {
            if (nk_button_label(ctx, "Cadastrar"))   login_tentar_cadastrar();
            if (nk_button_label(ctx, "Voltar"))      login_ir_para_login();
        }
    }
    nk_end(ctx);
}
