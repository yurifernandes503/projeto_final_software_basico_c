USE mini_orkut;

-- Exemplos diretos para a galera do front em C chamar pelo mysql_query().

-- Tela 1: cadastro completo.
CALL sp_criar_cadastro_com_perfil('Nome Teste', 'nometeste', 'teste@email.com', '123456', 'Teste', 'Salvador', 'BA');

-- Tela 2: login com login ou e-mail.
CALL sp_login('nometeste', '123456');
CALL sp_login('teste@email.com', '123456');

-- Guardar no C o id_perfil retornado pelo login. Esse id será usado nas outras telas.

-- Tela 3: perfil do usuário logado.
SELECT * FROM vw_usuarios_completos WHERE id_perfil = 1;

-- Tela 4: editar perfil.
CALL sp_atualizar_perfil(1, 'NovoApelido', 'Salvador', 'BA', 'Bio editada pelo sistema em C', 'NAO_INFORMADO', 'Dados sociais editados pelo C');

-- Tela 5: pesquisar pessoas.
CALL sp_buscar_perfil('yuri');

-- Tela 6: solicitar amizade.
CALL sp_solicitar_amizade(1, 4);

-- Tela 7: ver solicitações recebidas.
CALL sp_listar_solicitacoes_pendentes(4);

-- Tela 8: aceitar ou recusar amizade.
CALL sp_responder_amizade(3, 'ACEITA');
CALL sp_responder_amizade(3, 'RECUSADA');

-- Tela 9: listar amigos.
CALL sp_listar_amigos(1);

-- Tela 10: feed paginado / scroll infinito.
CALL sp_listar_feed_amigos(1, 5, 0);
CALL sp_listar_feed_amigos(1, 5, 5);

-- Tela 11: criar post.
CALL sp_criar_post(1, 'Post criado pelo programa em C.', 'PUBLICO');

-- Tela 12: curtir e comentar.
CALL sp_curtir_post(1, 2);
CALL sp_comentar_post(1, 2, 'Comentário enviado pelo C.');

-- Tela 13: enviar mensagem.
CALL sp_enviar_mensagem(1, 2, 'Mensagem enviada pelo sistema em C.');

-- Tela 14: listar conversa.
CALL sp_listar_conversa(1, 2);

-- Tela 15: notificações.
CALL sp_listar_notificacoes(1);
