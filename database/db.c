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


void db_fechar(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}
