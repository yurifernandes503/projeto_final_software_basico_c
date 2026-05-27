#include "sqlite3.h"
#include "db.h"
#include <string.h>
#include <stdio.h>

/* Conexão global com o banco — um único arquivo .db para o programa inteiro */
static sqlite3 *g_db = NULL;

/*
    Schema embutido no executável.
    Espelha database/schema.sql — qualquer mudança lá deve ser replicada aqui.
*/
static const char SCHEMA_SQL[] =
    "CREATE TABLE IF NOT EXISTS cadastros ("
    "  id_cadastro INTEGER PRIMARY KEY,"
    "  login       TEXT NOT NULL UNIQUE,"
    "  senha       TEXT NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS perfis ("
    "  id_perfil   INTEGER PRIMARY KEY,"
    "  id_cadastro INTEGER NOT NULL UNIQUE,"
    "  apelido     TEXT NOT NULL,"
    "  FOREIGN KEY (id_cadastro) REFERENCES cadastros(id_cadastro) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS contatos ("
    "  id_contato INTEGER PRIMARY KEY,"
    "  id_perfil  INTEGER NOT NULL,"
    "  apelido    TEXT NOT NULL,"
    "  ip_ultimo  TEXT NOT NULL,"
    "  porta      INTEGER NOT NULL DEFAULT 7777,"
    "  FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS mensagens ("
    "  id_mensagem INTEGER PRIMARY KEY,"
    "  id_contato  INTEGER NOT NULL,"
    "  remetente   TEXT NOT NULL,"
    "  texto       TEXT NOT NULL,"
    "  FOREIGN KEY (id_contato) REFERENCES contatos(id_contato) ON DELETE CASCADE"
    ");";


int db_init(void) {
    if (sqlite3_open(DB_FILE, &g_db) != SQLITE_OK) {
        return DB_ERRO;
    }

    /* Foreign keys ficam desligadas por padrão no SQLite — liga explicitamente */
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

    char *err = NULL;
    if (sqlite3_exec(g_db, SCHEMA_SQL, NULL, NULL, &err) != SQLITE_OK) {
        sqlite3_free(err);
        return DB_ERRO;
    }

    return DB_OK;
}


int db_criar_conta(const char *login, const char *senha) {
    sqlite3_stmt *stmt;

    /* Verifica se o login já existe antes de tentar inserir */
    sqlite3_prepare_v2(g_db,
        "SELECT 1 FROM cadastros WHERE login = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);
    int existe = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    if (existe) return DB_LOGIN_EXISTE;

    /* Insere o cadastro (credenciais de acesso) */
    sqlite3_prepare_v2(g_db,
        "INSERT INTO cadastros (login, senha) VALUES (?, ?);",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, senha, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DB_ERRO;
    }
    sqlite3_finalize(stmt);

    sqlite3_int64 id_cad = sqlite3_last_insert_rowid(g_db);

    /* Insere o perfil (identidade social) — apelido começa igual ao login */
    sqlite3_prepare_v2(g_db,
        "INSERT INTO perfis (id_cadastro, apelido) VALUES (?, ?);",
        -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, id_cad);
    sqlite3_bind_text(stmt, 2, login, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DB_ERRO;
    }
    sqlite3_finalize(stmt);

    return DB_OK;
}


int db_login(const char *login, const char *senha, UsuarioLogado *out) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(g_db,
        "SELECT p.id_perfil, c.login, p.apelido "
        "FROM cadastros c "
        "INNER JOIN perfis p ON p.id_cadastro = c.id_cadastro "
        "WHERE c.login = ? AND c.senha = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, senha, -1, SQLITE_STATIC);

    int resultado = DB_CRED_ERRADA;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out->id_perfil = sqlite3_column_int(stmt, 0);
        strncpy(out->login,   (const char *)sqlite3_column_text(stmt, 1), MAX_LOGIN);
        strncpy(out->apelido, (const char *)sqlite3_column_text(stmt, 2), MAX_APELIDO);
        out->login[MAX_LOGIN]     = '\0';
        out->apelido[MAX_APELIDO] = '\0';
        resultado = DB_OK;
    }

    sqlite3_finalize(stmt);
    return resultado;
}


int db_adicionar_contato(int id_perfil, const char *apelido,
                         const char *ip, int porta) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(g_db,
        "INSERT INTO contatos (id_perfil, apelido, ip_ultimo, porta) VALUES (?, ?, ?, ?);",
        -1, &stmt, NULL);
    sqlite3_bind_int (stmt, 1, id_perfil);
    sqlite3_bind_text(stmt, 2, apelido, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, ip,      -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, porta);

    int r = (sqlite3_step(stmt) == SQLITE_DONE) ? DB_OK : DB_ERRO;
    sqlite3_finalize(stmt);
    return r;
}


int db_listar_contatos(int id_perfil, Contato *out, int max, int *count) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(g_db,
        "SELECT id_contato, apelido, ip_ultimo, porta "
        "FROM contatos WHERE id_perfil = ? ORDER BY apelido;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id_perfil);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max) {
        out[*count].id_contato = sqlite3_column_int(stmt, 0);
        strncpy(out[*count].apelido, (const char *)sqlite3_column_text(stmt, 1), MAX_APELIDO);
        strncpy(out[*count].ip,      (const char *)sqlite3_column_text(stmt, 2), MAX_IP);
        out[*count].porta            = sqlite3_column_int(stmt, 3);
        out[*count].apelido[MAX_APELIDO] = '\0';
        out[*count].ip[MAX_IP]           = '\0';
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return DB_OK;
}


int db_atualizar_apelido_contato(int id_contato, const char *apelido) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(g_db,
        "UPDATE contatos SET apelido = ? WHERE id_contato = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, apelido,    -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, id_contato);

    int r = (sqlite3_step(stmt) == SQLITE_DONE) ? DB_OK : DB_ERRO;
    sqlite3_finalize(stmt);
    return r;
}


int db_salvar_mensagem(int id_contato, const char *remetente, const char *texto) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(g_db,
        "INSERT INTO mensagens (id_contato, remetente, texto) VALUES (?, ?, ?);",
        -1, &stmt, NULL);
    sqlite3_bind_int (stmt, 1, id_contato);
    sqlite3_bind_text(stmt, 2, remetente, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, texto,     -1, SQLITE_STATIC);

    int r = (sqlite3_step(stmt) == SQLITE_DONE) ? DB_OK : DB_ERRO;
    sqlite3_finalize(stmt);
    return r;
}


int db_carregar_historico(int id_contato, char *buf, int buf_sz) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(g_db,
        "SELECT remetente, texto FROM mensagens WHERE id_contato = ? ORDER BY id_mensagem;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id_contato);

    buf[0] = '\0';
    int pos = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *rem = (const char *)sqlite3_column_text(stmt, 0);
        const char *txt = (const char *)sqlite3_column_text(stmt, 1);
        char line[400];
        _snprintf(line, sizeof(line), "%s: %s\n", rem, txt);
        int n = (int)strlen(line);
        /* Para quando o buffer estiver quase cheio */
        if (pos + n + 1 >= buf_sz) break;
        memcpy(buf + pos, line, n);
        pos += n;
        buf[pos] = '\0';
    }

    sqlite3_finalize(stmt);
    return DB_OK;
}


void db_fechar(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}
