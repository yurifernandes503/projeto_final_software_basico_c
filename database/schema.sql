-- Schema MVP — login, cadastro, contatos e histórico de mensagens.
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
CREATE TABLE IF NOT EXISTS perfis (
    id_perfil   INTEGER PRIMARY KEY,
    id_cadastro INTEGER NOT NULL UNIQUE,
    apelido     TEXT NOT NULL,
    FOREIGN KEY (id_cadastro) REFERENCES cadastros(id_cadastro) ON DELETE CASCADE
);

-- contatos locais por usuário — cada perfil tem sua própria lista.
-- porta permite distinguir instâncias diferentes na mesma máquina.
CREATE TABLE IF NOT EXISTS contatos (
    id_contato INTEGER PRIMARY KEY,
    id_perfil  INTEGER NOT NULL,
    apelido    TEXT NOT NULL,
    ip_ultimo  TEXT NOT NULL,
    porta      INTEGER NOT NULL DEFAULT 7777,
    FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE
);

-- histórico de mensagens por contato, em ordem de envio.
CREATE TABLE IF NOT EXISTS mensagens (
    id_mensagem INTEGER PRIMARY KEY,
    id_contato  INTEGER NOT NULL,
    remetente   TEXT NOT NULL,
    texto       TEXT NOT NULL,
    FOREIGN KEY (id_contato) REFERENCES contatos(id_contato) ON DELETE CASCADE
);
