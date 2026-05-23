-- Schema MVP — só o necessário para login e cadastro funcionar.
-- Tabelas das fases futuras (amizades, posts, etc.) serão adicionadas quando chegar a hora.
--
-- Adaptado para SQLite:
--   AUTO_INCREMENT → INTEGER PRIMARY KEY
--   Sem ENUM, sem procedures, sem campos que não usamos agora.
--
-- ATENÇÃO: este arquivo é referência de documentação.
-- O schema real embutido no programa está em database/db.c (string SCHEMA_SQL).

PRAGMA foreign_keys = ON;

-- login e senha são tudo que precisamos para autenticar.
CREATE TABLE IF NOT EXISTS cadastros (
    id_cadastro INTEGER PRIMARY KEY,
    login       TEXT NOT NULL UNIQUE,
    senha       TEXT NOT NULL
);

-- apelido é o nome exibido no chat.
-- Por enquanto é igual ao login, mas fica separado para crescer depois.
CREATE TABLE IF NOT EXISTS perfis (
    id_perfil   INTEGER PRIMARY KEY,
    id_cadastro INTEGER NOT NULL UNIQUE,
    apelido     TEXT NOT NULL,
    FOREIGN KEY (id_cadastro) REFERENCES cadastros(id_cadastro) ON DELETE CASCADE
);
