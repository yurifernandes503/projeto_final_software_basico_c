CREATE DATABASE IF NOT EXISTS mini_orkut CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE mini_orkut;

DROP TABLE IF EXISTS notificacoes;
DROP TABLE IF EXISTS curtidas;
DROP TABLE IF EXISTS comentarios;
DROP TABLE IF EXISTS mensagens;
DROP TABLE IF EXISTS posts;
DROP TABLE IF EXISTS amizades;
DROP TABLE IF EXISTS comunidades_membros;
DROP TABLE IF EXISTS comunidades;
DROP TABLE IF EXISTS perfis;
DROP TABLE IF EXISTS cadastros;

-- =========================================================
-- CADASTRO: parte privada/administrativa usada no login.
-- Aqui ficam os campos que não dependem do perfil social.
-- login e email são UNIQUE para impedir conflitos.
-- =========================================================
CREATE TABLE cadastros (
    id_cadastro INT AUTO_INCREMENT PRIMARY KEY,
    nome_completo VARCHAR(120) NOT NULL,
    login VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(120) NOT NULL UNIQUE,
    senha VARCHAR(120) NOT NULL,
    data_cadastro DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    status_conta ENUM('ATIVA','BLOQUEADA','INATIVA') NOT NULL DEFAULT 'ATIVA'
);

-- =========================================================
-- PERFIL: parte pública/social do usuário.
-- Cada cadastro tem apenas um perfil.
-- =========================================================
CREATE TABLE perfis (
    id_perfil INT AUTO_INCREMENT PRIMARY KEY,
    id_cadastro INT NOT NULL UNIQUE,
    apelido VARCHAR(60) NOT NULL,
    cidade VARCHAR(80),
    estado VARCHAR(40),
    biografia VARCHAR(255),
    foto_url VARCHAR(255),
    status_relacionamento ENUM('SOLTEIRO','NAMORANDO','CASADO','COMPLICADO','NAO_INFORMADO') DEFAULT 'NAO_INFORMADO',
    dados_sociais VARCHAR(255),
    data_nascimento DATE,
    criado_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    atualizado_em DATETIME NULL,
    CONSTRAINT fk_perfil_cadastro FOREIGN KEY (id_cadastro)
        REFERENCES cadastros(id_cadastro)
        ON DELETE CASCADE
        ON UPDATE CASCADE
);

-- =========================================================
-- AMIZADES: relacionamento social entre dois perfis.
-- id_perfil_menor/id_perfil_maior são gerados para impedir
-- amizade duplicada invertida, exemplo: (1,2) e (2,1).
-- =========================================================
CREATE TABLE amizades (
    id_amizade INT AUTO_INCREMENT PRIMARY KEY,
    id_perfil_solicitante INT NOT NULL,
    id_perfil_destinatario INT NOT NULL,
    id_perfil_menor INT GENERATED ALWAYS AS (LEAST(id_perfil_solicitante, id_perfil_destinatario)) STORED,
    id_perfil_maior INT GENERATED ALWAYS AS (GREATEST(id_perfil_solicitante, id_perfil_destinatario)) STORED,
    status_amizade ENUM('PENDENTE','ACEITA','RECUSADA','BLOQUEADA') NOT NULL DEFAULT 'PENDENTE',
    criada_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    respondida_em DATETIME NULL,
    CONSTRAINT fk_amizade_solicitante FOREIGN KEY (id_perfil_solicitante) REFERENCES perfis(id_perfil) ON DELETE CASCADE,
    CONSTRAINT fk_amizade_destinatario FOREIGN KEY (id_perfil_destinatario) REFERENCES perfis(id_perfil) ON DELETE CASCADE,
    CONSTRAINT chk_amizade_diferente CHECK (id_perfil_solicitante <> id_perfil_destinatario),
    CONSTRAINT uq_amizade_unica UNIQUE (id_perfil_menor, id_perfil_maior)
);

CREATE TABLE posts (
    id_post INT AUTO_INCREMENT PRIMARY KEY,
    id_perfil INT NOT NULL,
    conteudo TEXT NOT NULL,
    visibilidade ENUM('PUBLICO','AMIGOS') NOT NULL DEFAULT 'PUBLICO',
    criado_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_post_perfil FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE
);

CREATE TABLE comentarios (
    id_comentario INT AUTO_INCREMENT PRIMARY KEY,
    id_post INT NOT NULL,
    id_perfil INT NOT NULL,
    comentario TEXT NOT NULL,
    criado_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_comentario_post FOREIGN KEY (id_post) REFERENCES posts(id_post) ON DELETE CASCADE,
    CONSTRAINT fk_comentario_perfil FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE
);

CREATE TABLE curtidas (
    id_curtida INT AUTO_INCREMENT PRIMARY KEY,
    id_post INT NOT NULL,
    id_perfil INT NOT NULL,
    criado_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_curtida_post FOREIGN KEY (id_post) REFERENCES posts(id_post) ON DELETE CASCADE,
    CONSTRAINT fk_curtida_perfil FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE,
    CONSTRAINT uq_curtida UNIQUE (id_post, id_perfil)
);

CREATE TABLE mensagens (
    id_mensagem INT AUTO_INCREMENT PRIMARY KEY,
    id_remetente INT NOT NULL,
    id_destinatario INT NOT NULL,
    mensagem TEXT NOT NULL,
    enviada_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    lida BOOLEAN NOT NULL DEFAULT FALSE,
    CONSTRAINT fk_msg_remetente FOREIGN KEY (id_remetente) REFERENCES perfis(id_perfil) ON DELETE CASCADE,
    CONSTRAINT fk_msg_destinatario FOREIGN KEY (id_destinatario) REFERENCES perfis(id_perfil) ON DELETE CASCADE,
    CONSTRAINT chk_msg_diferente CHECK (id_remetente <> id_destinatario)
);

CREATE TABLE comunidades (
    id_comunidade INT AUTO_INCREMENT PRIMARY KEY,
    nome VARCHAR(100) NOT NULL UNIQUE,
    descricao VARCHAR(255),
    id_dono INT NOT NULL,
    criada_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_comunidade_dono FOREIGN KEY (id_dono) REFERENCES perfis(id_perfil) ON DELETE CASCADE
);

CREATE TABLE comunidades_membros (
    id_comunidade INT NOT NULL,
    id_perfil INT NOT NULL,
    entrou_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id_comunidade, id_perfil),
    CONSTRAINT fk_cm_comunidade FOREIGN KEY (id_comunidade) REFERENCES comunidades(id_comunidade) ON DELETE CASCADE,
    CONSTRAINT fk_cm_perfil FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE
);

-- Ajuda o front em C a montar uma tela de avisos sem precisar cruzar várias tabelas.
CREATE TABLE notificacoes (
    id_notificacao INT AUTO_INCREMENT PRIMARY KEY,
    id_perfil INT NOT NULL,
    tipo ENUM('AMIZADE','MENSAGEM','POST') NOT NULL,
    texto VARCHAR(255) NOT NULL,
    lida BOOLEAN NOT NULL DEFAULT FALSE,
    criada_em DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_notificacao_perfil FOREIGN KEY (id_perfil) REFERENCES perfis(id_perfil) ON DELETE CASCADE
);

CREATE INDEX idx_cadastros_login ON cadastros(login);
CREATE INDEX idx_cadastros_email ON cadastros(email);
CREATE INDEX idx_perfis_apelido ON perfis(apelido);
CREATE INDEX idx_posts_data ON posts(criado_em);
CREATE INDEX idx_mensagens_conversa ON mensagens(id_remetente, id_destinatario, enviada_em);
CREATE INDEX idx_amizades_status ON amizades(status_amizade);
CREATE INDEX idx_notificacoes_perfil ON notificacoes(id_perfil, lida, criada_em);
