USE mini_orkut;

INSERT INTO cadastros (nome_completo, login, email, senha) VALUES
('Yuri Fernandes', 'yurifernandes', 'yuri@email.com', '123456'),
('Lucas Almeida', 'lucasalmeida', 'lucas@email.com', '123456'),
('Eric Trimer', 'erictrimer', 'eric@email.com', '123456'),
('Ana Souza', 'anasouza', 'ana@email.com', '123456'),
('Marcos Lima', 'marcoslima', 'marcos@email.com', '123456');

INSERT INTO perfis (id_cadastro, apelido, cidade, estado, biografia, status_relacionamento, dados_sociais, data_nascimento) VALUES
(1, 'Yuri', 'Salvador', 'BA', 'Gosto de tecnologia, jogos e projetos em C.', 'SOLTEIRO', 'GitHub: yurifernandes | Interesses: C, MySQL e games', '2004-09-10'),
(2, 'Lucas', 'Salvador', 'BA', 'Desenvolvedor do mini Orkut.', 'NAO_INFORMADO', 'Interesses: backend e banco de dados', '2003-05-21'),
(3, 'Eric', 'Lauro de Freitas', 'BA', 'Curto banco de dados e programação.', 'NAMORANDO', 'Interesses: modelagem, SQL e redes sociais antigas', '2004-02-17'),
(4, 'Ana', 'Feira de Santana', 'BA', 'Amo comunidades antigas da internet.', 'SOLTEIRO', 'Interesses: comunidades, design e posts', '2002-11-03'),
(5, 'Marcos', 'Salvador', 'BA', 'Fã de redes sociais clássicas.', 'COMPLICADO', 'Interesses: fóruns, comunidades e tecnologia', '2001-08-29');

INSERT INTO amizades (id_perfil_solicitante, id_perfil_destinatario, status_amizade, respondida_em) VALUES
(1, 2, 'ACEITA', NOW()),
(1, 3, 'ACEITA', NOW()),
(2, 4, 'PENDENTE', NULL),
(3, 5, 'ACEITA', NOW());

INSERT INTO posts (id_perfil, conteudo, visibilidade) VALUES
(1, 'Primeiro post no nosso mini Orkut feito em C com MySQL.', 'PUBLICO'),
(2, 'A ideia agora é separar cadastro de perfil para ficar mais organizado.', 'PUBLICO'),
(3, 'Banco de dados relacional deixando o projeto mais profissional.', 'PUBLICO'),
(4, 'Saudades das comunidades do Orkut!', 'PUBLICO'),
(5, 'Esse post aparece como exemplo de feed paginado no front em C.', 'AMIGOS');

INSERT INTO comentarios (id_post, id_perfil, comentario) VALUES
(1, 2, 'Ficou massa demais!'),
(1, 3, 'Agora o modelo do banco está mais correto.'),
(2, 1, 'Boa, isso ajuda muito na explicação.'),
(4, 5, 'Comunidade era a melhor parte.');

INSERT INTO curtidas (id_post, id_perfil) VALUES
(1, 2), (1, 3), (1, 4),
(2, 1), (2, 3),
(3, 1),
(4, 5);

INSERT INTO mensagens (id_remetente, id_destinatario, mensagem, lida) VALUES
(1, 2, 'Lucas, já separamos cadastro e perfil no banco.', TRUE),
(2, 1, 'Perfeito, agora dá pra explicar melhor.', FALSE),
(3, 1, 'Yuri, vê se as procedures ficaram boas.', FALSE),
(1, 3, 'Vou revisar tudo antes de apresentar.', TRUE);

INSERT INTO comunidades (nome, descricao, id_dono) VALUES
('Eu amo programar em C', 'Comunidade para quem sofre mas gosta de C.', 1),
('Orkut raiz', 'Pessoas que sentem saudade do Orkut antigo.', 4);

INSERT INTO comunidades_membros (id_comunidade, id_perfil) VALUES
(1, 1), (1, 2), (1, 3),
(2, 1), (2, 4), (2, 5);

INSERT INTO notificacoes (id_perfil, tipo, texto, lida) VALUES
(1, 'MENSAGEM', 'Você recebeu uma mensagem de Lucas.', FALSE),
(1, 'MENSAGEM', 'Você recebeu uma mensagem de Eric.', FALSE),
(4, 'AMIZADE', 'Lucas enviou uma solicitação de amizade.', FALSE);
