#ifndef DB_H
#define DB_H

#define DB_FILE      "social_media.db"
#define MAX_LOGIN    50
#define MAX_SENHA    120
#define MAX_APELIDO  60

/* Códigos de retorno das funções de banco */
#define DB_OK           0
#define DB_ERRO         1
#define DB_LOGIN_EXISTE 2
#define DB_CRED_ERRADA  3

/*
    Dados do usuário que ficam na memória após o login bem-sucedido.
    Usado pelo resto do programa para saber quem está logado.
*/
typedef struct {
    int  id_perfil;
    char login[MAX_LOGIN + 1];
    char apelido[MAX_APELIDO + 1];
} UsuarioLogado;

/*
    Abre (ou cria) o banco social_media.db e inicializa as tabelas.
    Deve ser chamado uma vez no início do programa.
    Returns: DB_OK ou DB_ERRO.
*/
int db_init(void);

/*
    Cadastra um novo usuário. Cria uma linha em cadastros e outra em perfis.
    O apelido é igual ao login por enquanto.
    Returns: DB_OK, DB_ERRO ou DB_LOGIN_EXISTE.
*/
int db_criar_conta(const char *login, const char *senha);

/*
    Valida login e senha. Se corretos, preenche *out com os dados do usuário.
    Returns: DB_OK ou DB_CRED_ERRADA.
*/
int db_login(const char *login, const char *senha, UsuarioLogado *out);

/*
    Fecha a conexão com o banco. Chamar antes de encerrar o programa.
*/
void db_fechar(void);

#endif
