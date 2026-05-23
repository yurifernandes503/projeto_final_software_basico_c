USE mini_orkut;

CREATE OR REPLACE VIEW vw_usuarios_completos AS
SELECT
    c.id_cadastro,
    p.id_perfil,
    c.nome_completo,
    c.login,
    c.email,
    c.status_conta,
    p.apelido,
    p.cidade,
    p.estado,
    p.biografia,
    p.status_relacionamento,
    p.dados_sociais,
    c.data_cadastro,
    p.criado_em AS perfil_criado_em
FROM cadastros c
INNER JOIN perfis p ON p.id_cadastro = c.id_cadastro;

CREATE OR REPLACE VIEW vw_feed AS
SELECT
    po.id_post,
    pe.id_perfil,
    pe.apelido,
    po.conteudo,
    po.visibilidade,
    po.criado_em,
    COUNT(DISTINCT cu.id_curtida) AS total_curtidas,
    COUNT(DISTINCT co.id_comentario) AS total_comentarios
FROM posts po
INNER JOIN perfis pe ON pe.id_perfil = po.id_perfil
LEFT JOIN curtidas cu ON cu.id_post = po.id_post
LEFT JOIN comentarios co ON co.id_post = po.id_post
GROUP BY po.id_post, pe.id_perfil, pe.apelido, po.conteudo, po.visibilidade, po.criado_em;

CREATE OR REPLACE VIEW vw_amizades_detalhadas AS
SELECT
    a.id_amizade,
    a.id_perfil_solicitante,
    p1.apelido AS solicitante,
    a.id_perfil_destinatario,
    p2.apelido AS destinatario,
    a.status_amizade,
    a.criada_em,
    a.respondida_em
FROM amizades a
INNER JOIN perfis p1 ON p1.id_perfil = a.id_perfil_solicitante
INNER JOIN perfis p2 ON p2.id_perfil = a.id_perfil_destinatario;

CREATE OR REPLACE VIEW vw_amizades_aceitas AS
SELECT *
FROM vw_amizades_detalhadas
WHERE status_amizade = 'ACEITA';

CREATE OR REPLACE VIEW vw_mensagens_detalhadas AS
SELECT
    m.id_mensagem,
    m.id_remetente,
    pr.apelido AS remetente,
    m.id_destinatario,
    pd.apelido AS destinatario,
    m.mensagem,
    m.enviada_em,
    m.lida
FROM mensagens m
INNER JOIN perfis pr ON pr.id_perfil = m.id_remetente
INNER JOIN perfis pd ON pd.id_perfil = m.id_destinatario;

CREATE OR REPLACE VIEW vw_notificacoes_abertas AS
SELECT
    n.id_notificacao,
    n.id_perfil,
    p.apelido,
    n.tipo,
    n.texto,
    n.criada_em
FROM notificacoes n
INNER JOIN perfis p ON p.id_perfil = n.id_perfil
WHERE n.lida = FALSE;
