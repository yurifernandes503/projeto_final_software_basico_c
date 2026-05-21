USE mini_orkut;

-- 1) Ver o usuário completo, agora juntando CADASTRO + PERFIL.
SELECT * FROM vw_usuarios_completos;

-- 2) Testar login usando login OU e-mail.
CALL sp_login('yurifernandes', '123456');
CALL sp_login('yuri@email.com', '123456');

-- 3) Buscar perfis para a tela de pesquisa do front em C.
CALL sp_buscar_perfil('yu');

-- 4) Ver o feed paginado, simulando scroll infinito.
CALL sp_listar_feed_paginado(3, 0);
CALL sp_listar_feed_paginado(3, 3);

-- 5) Feed respeitando amigos/publico.
CALL sp_listar_feed_amigos(1, 5, 0);

-- 6) Criar um novo usuário já separando cadastro e perfil.
CALL sp_criar_cadastro_com_perfil(
    'Bruna Costa',
    'brunacosta',
    'bruna@email.com',
    '123456',
    'Bruna',
    'Salvador',
    'BA'
);

-- 7) Atualizar somente informações do perfil, sem mexer no cadastro/login.
CALL sp_atualizar_perfil(1, 'YuriDev', 'Salvador', 'BA', 'Perfil atualizado para apresentação do trabalho.', 'SOLTEIRO', 'GitHub, C, MySQL e jogos');

-- 8) Fluxo de amizade.
CALL sp_solicitar_amizade(5, 1);
CALL sp_listar_solicitacoes_pendentes(1);
CALL sp_listar_amigos(1);
SELECT * FROM vw_amizades_detalhadas;

-- 9) Mensagens.
SELECT * FROM vw_mensagens_detalhadas;
CALL sp_listar_conversa(1, 2);
CALL sp_enviar_mensagem(2, 1, 'Mensagem enviada pela procedure depois das melhorias.');
CALL sp_marcar_conversa_como_lida(1, 2);

-- 10) Notificações.
CALL sp_listar_notificacoes(1);

-- 11) Curtir e comentar.
CALL sp_curtir_post(1, 5);
CALL sp_comentar_post(1, 5, 'Comentário criado pelo teste SQL.');
