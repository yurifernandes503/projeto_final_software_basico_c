USE mini_orkut;

DELIMITER $$

CREATE PROCEDURE sp_criar_cadastro_com_perfil(
    IN p_nome_completo VARCHAR(120),
    IN p_login VARCHAR(50),
    IN p_email VARCHAR(120),
    IN p_senha VARCHAR(120),
    IN p_apelido VARCHAR(60),
    IN p_cidade VARCHAR(80),
    IN p_estado VARCHAR(40)
)
BEGIN
    DECLARE novo_id_cadastro INT;

    INSERT INTO cadastros (nome_completo, login, email, senha)
    VALUES (p_nome_completo, p_login, p_email, p_senha);

    SET novo_id_cadastro = LAST_INSERT_ID();

    INSERT INTO perfis (id_cadastro, apelido, cidade, estado, biografia, dados_sociais)
    VALUES (novo_id_cadastro, p_apelido, p_cidade, p_estado, 'Novo usuário do Mini Orkut', 'Ainda não informado.');

    SELECT novo_id_cadastro AS id_cadastro, LAST_INSERT_ID() AS id_perfil;
END $$

CREATE PROCEDURE sp_login(
    IN p_usuario VARCHAR(120),
    IN p_senha VARCHAR(120)
)
BEGIN
    SELECT
        c.id_cadastro,
        p.id_perfil,
        c.nome_completo,
        c.login,
        c.email,
        p.apelido,
        c.status_conta
    FROM cadastros c
    INNER JOIN perfis p ON p.id_cadastro = c.id_cadastro
    WHERE (c.login = p_usuario OR c.email = p_usuario)
      AND c.senha = p_senha
      AND c.status_conta = 'ATIVA';
END $$

CREATE PROCEDURE sp_buscar_perfil(
    IN p_busca VARCHAR(80)
)
BEGIN
    SELECT id_perfil, apelido, cidade, estado, biografia, status_relacionamento
    FROM vw_usuarios_completos
    WHERE apelido LIKE CONCAT('%', p_busca, '%')
       OR login LIKE CONCAT('%', p_busca, '%')
    ORDER BY apelido;
END $$

CREATE PROCEDURE sp_atualizar_perfil(
    IN p_id_perfil INT,
    IN p_apelido VARCHAR(60),
    IN p_cidade VARCHAR(80),
    IN p_estado VARCHAR(40),
    IN p_biografia VARCHAR(255),
    IN p_status_relacionamento VARCHAR(20),
    IN p_dados_sociais VARCHAR(255)
)
BEGIN
    UPDATE perfis
    SET apelido = p_apelido,
        cidade = p_cidade,
        estado = p_estado,
        biografia = p_biografia,
        status_relacionamento = p_status_relacionamento,
        dados_sociais = p_dados_sociais,
        atualizado_em = NOW()
    WHERE id_perfil = p_id_perfil;
END $$

CREATE PROCEDURE sp_solicitar_amizade(
    IN p_solicitante INT,
    IN p_destinatario INT
)
BEGIN
    INSERT INTO amizades (id_perfil_solicitante, id_perfil_destinatario, status_amizade)
    VALUES (p_solicitante, p_destinatario, 'PENDENTE');

    INSERT INTO notificacoes (id_perfil, tipo, texto)
    SELECT p_destinatario, 'AMIZADE', CONCAT(p.apelido, ' enviou uma solicitação de amizade.')
    FROM perfis p
    WHERE p.id_perfil = p_solicitante;
END $$

CREATE PROCEDURE sp_responder_amizade(
    IN p_id_amizade INT,
    IN p_status VARCHAR(20)
)
BEGIN
    UPDATE amizades
    SET status_amizade = p_status,
        respondida_em = NOW()
    WHERE id_amizade = p_id_amizade
      AND status_amizade = 'PENDENTE';
END $$

CREATE PROCEDURE sp_listar_amigos(
    IN p_id_perfil INT
)
BEGIN
    SELECT
        CASE
            WHEN a.id_perfil_solicitante = p_id_perfil THEN a.id_perfil_destinatario
            ELSE a.id_perfil_solicitante
        END AS id_amigo,
        CASE
            WHEN a.id_perfil_solicitante = p_id_perfil THEN p2.apelido
            ELSE p1.apelido
        END AS apelido_amigo,
        a.criada_em
    FROM amizades a
    INNER JOIN perfis p1 ON p1.id_perfil = a.id_perfil_solicitante
    INNER JOIN perfis p2 ON p2.id_perfil = a.id_perfil_destinatario
    WHERE (a.id_perfil_solicitante = p_id_perfil OR a.id_perfil_destinatario = p_id_perfil)
      AND a.status_amizade = 'ACEITA'
    ORDER BY apelido_amigo;
END $$

CREATE PROCEDURE sp_listar_solicitacoes_pendentes(
    IN p_id_perfil INT
)
BEGIN
    SELECT a.id_amizade, a.id_perfil_solicitante, p.apelido AS solicitante, a.criada_em
    FROM amizades a
    INNER JOIN perfis p ON p.id_perfil = a.id_perfil_solicitante
    WHERE a.id_perfil_destinatario = p_id_perfil
      AND a.status_amizade = 'PENDENTE'
    ORDER BY a.criada_em DESC;
END $$

CREATE PROCEDURE sp_criar_post(
    IN p_id_perfil INT,
    IN p_conteudo TEXT,
    IN p_visibilidade VARCHAR(10)
)
BEGIN
    INSERT INTO posts (id_perfil, conteudo, visibilidade)
    VALUES (p_id_perfil, p_conteudo, p_visibilidade);
END $$

CREATE PROCEDURE sp_listar_feed_paginado(
    IN p_limite INT,
    IN p_offset INT
)
BEGIN
    SELECT *
    FROM vw_feed
    ORDER BY criado_em DESC
    LIMIT p_limite OFFSET p_offset;
END $$

CREATE PROCEDURE sp_listar_feed_amigos(
    IN p_id_perfil INT,
    IN p_limite INT,
    IN p_offset INT
)
BEGIN
    SELECT f.*
    FROM vw_feed f
    WHERE f.visibilidade = 'PUBLICO'
       OR f.id_perfil = p_id_perfil
       OR f.id_perfil IN (
            SELECT CASE
                WHEN a.id_perfil_solicitante = p_id_perfil THEN a.id_perfil_destinatario
                ELSE a.id_perfil_solicitante
            END
            FROM amizades a
            WHERE (a.id_perfil_solicitante = p_id_perfil OR a.id_perfil_destinatario = p_id_perfil)
              AND a.status_amizade = 'ACEITA'
       )
    ORDER BY f.criado_em DESC
    LIMIT p_limite OFFSET p_offset;
END $$

CREATE PROCEDURE sp_curtir_post(
    IN p_id_post INT,
    IN p_id_perfil INT
)
BEGIN
    INSERT IGNORE INTO curtidas (id_post, id_perfil)
    VALUES (p_id_post, p_id_perfil);
END $$

CREATE PROCEDURE sp_comentar_post(
    IN p_id_post INT,
    IN p_id_perfil INT,
    IN p_comentario TEXT
)
BEGIN
    INSERT INTO comentarios (id_post, id_perfil, comentario)
    VALUES (p_id_post, p_id_perfil, p_comentario);
END $$

CREATE PROCEDURE sp_enviar_mensagem(
    IN p_id_remetente INT,
    IN p_id_destinatario INT,
    IN p_mensagem TEXT
)
BEGIN
    INSERT INTO mensagens (id_remetente, id_destinatario, mensagem)
    VALUES (p_id_remetente, p_id_destinatario, p_mensagem);

    INSERT INTO notificacoes (id_perfil, tipo, texto)
    SELECT p_id_destinatario, 'MENSAGEM', CONCAT('Nova mensagem de ', p.apelido, '.')
    FROM perfis p
    WHERE p.id_perfil = p_id_remetente;
END $$

CREATE PROCEDURE sp_listar_conversa(
    IN p_usuario1 INT,
    IN p_usuario2 INT
)
BEGIN
    SELECT *
    FROM vw_mensagens_detalhadas
    WHERE (id_remetente = p_usuario1 AND id_destinatario = p_usuario2)
       OR (id_remetente = p_usuario2 AND id_destinatario = p_usuario1)
    ORDER BY enviada_em ASC;
END $$

CREATE PROCEDURE sp_marcar_conversa_como_lida(
    IN p_id_logado INT,
    IN p_id_outro INT
)
BEGIN
    UPDATE mensagens
    SET lida = TRUE
    WHERE id_destinatario = p_id_logado
      AND id_remetente = p_id_outro;
END $$

CREATE PROCEDURE sp_listar_notificacoes(
    IN p_id_perfil INT
)
BEGIN
    SELECT id_notificacao, tipo, texto, criada_em
    FROM notificacoes
    WHERE id_perfil = p_id_perfil AND lida = FALSE
    ORDER BY criada_em DESC;
END $$

DELIMITER ;
