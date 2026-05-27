#ifndef DB_H
#define DB_H

#define DB_FILE      "social_media.db"
#define MAX_LOGIN    50
#define MAX_SENHA    120
#define MAX_APELIDO  60
#define MAX_IP       64
#define MAX_CONTATOS 50

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
    Um contato da lista local — pessoa com quem este usuário pode conversar.
    porta permite distinguir instâncias diferentes rodando na mesma máquina.
*/
typedef struct {
    int  id_contato;
    char apelido[MAX_APELIDO + 1];
    char ip[MAX_IP + 1];
    int  porta;
} Contato;

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
    Insere um novo contato na lista do usuário id_perfil.
    Returns: DB_OK ou DB_ERRO.
*/
int db_adicionar_contato(int id_perfil, const char *apelido,
                         const char *ip, int porta);

/*
    Preenche out[] com os contatos do usuário id_perfil, ordenados por apelido.
    max é o tamanho máximo do array; *count recebe quantos foram preenchidos.
    Returns: DB_OK ou DB_ERRO.
*/
int db_listar_contatos(int id_perfil, Contato *out, int max, int *count);

/*
    Atualiza o apelido de um contato — usado quando o apelido real chega
    na primeira mensagem de um contato auto-adicionado pelo IP.
    Returns: DB_OK ou DB_ERRO.
*/
int db_atualizar_apelido_contato(int id_contato, const char *apelido);

/*
    Salva uma mensagem no histórico de um contato.
    remetente é o apelido de quem enviou (pode ser o usuário logado ou o contato).
    Returns: DB_OK ou DB_ERRO.
*/
int db_salvar_mensagem(int id_contato, const char *remetente, const char *texto);

/*
    Carrega o histórico de mensagens de um contato em buf, no formato "remetente: texto\n".
    buf_sz é o tamanho do buffer em bytes.
    Returns: DB_OK ou DB_ERRO.
*/
int db_carregar_historico(int id_contato, char *buf, int buf_sz);

/*
    Fecha a conexão com o banco. Chamar antes de encerrar o programa.
*/
void db_fechar(void);

#endif
