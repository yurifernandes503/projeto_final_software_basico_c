USE mini_orkut;

DELIMITER $$

CREATE TRIGGER trg_perfil_atualizado
BEFORE UPDATE ON perfis
FOR EACH ROW
BEGIN
    SET NEW.atualizado_em = NOW();
END $$

CREATE TRIGGER trg_impedir_login_email_vazios
BEFORE INSERT ON cadastros
FOR EACH ROW
BEGIN
    IF NEW.login IS NULL OR TRIM(NEW.login) = '' THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'O login do cadastro não pode ser vazio.';
    END IF;

    IF NEW.email IS NULL OR TRIM(NEW.email) = '' THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'O e-mail do cadastro não pode ser vazio.';
    END IF;
END $$

CREATE TRIGGER trg_impedir_apelido_vazio
BEFORE INSERT ON perfis
FOR EACH ROW
BEGIN
    IF NEW.apelido IS NULL OR TRIM(NEW.apelido) = '' THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'O apelido do perfil não pode ser vazio.';
    END IF;
END $$

CREATE TRIGGER trg_impedir_mensagem_vazia
BEFORE INSERT ON mensagens
FOR EACH ROW
BEGIN
    IF NEW.mensagem IS NULL OR TRIM(NEW.mensagem) = '' THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'A mensagem não pode ser vazia.';
    END IF;
END $$

CREATE TRIGGER trg_impedir_post_vazio
BEFORE INSERT ON posts
FOR EACH ROW
BEGIN
    IF NEW.conteudo IS NULL OR TRIM(NEW.conteudo) = '' THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'O post não pode ser vazio.';
    END IF;
END $$

CREATE TRIGGER trg_validar_amizade
BEFORE INSERT ON amizades
FOR EACH ROW
BEGIN
    IF NEW.id_perfil_solicitante = NEW.id_perfil_destinatario THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Um perfil não pode adicionar ele mesmo como amigo.';
    END IF;
END $$

DELIMITER ;
