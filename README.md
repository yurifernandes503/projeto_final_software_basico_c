# Projeto Final — Chat em C puro

Projeto final da disciplina de Programação Básica em C. Um sistema de chat em tempo real com login, cadastro e troca de mensagens via rede local, construído inteiramente em C puro com interface gráfica nativa do Windows.

---

## Sobre o projeto

O projeto recria as funcionalidades centrais de uma rede social: chat em tempo real, sistema de amizades e feed de posts. Toda a stack é nativa — sem frameworks, sem linguagens adicionais, sem dependências pesadas.

A arquitetura usa **multiprocessamento**: o executável roda em três modos distintos no mesmo binário. O processo da interface gráfica lança os demais como processos filhos e se comunica com eles via pipes Win32, mantendo a UI sempre responsiva enquanto o tráfego de rede ocorre em segundo plano.

---

## Stack técnica

| Camada | Tecnologia |
|---|---|
| Linguagem | C puro (C99) |
| Interface Gráfica | Nuklear GDI — header-only, sem dependências externas |
| Sistema Operacional | Win32 API — `CreateProcess`, `CreatePipe`, `PeekNamedPipe` |
| Rede | Winsock2 — TCP, servidor e cliente no mesmo binário |
| Banco de Dados | SQLite 3 — embarcado no projeto, banco é um arquivo `.db` local |
| Compilador | GCC / MinGW-w64 |

---

## Pré-requisitos

Você precisa de dois programas instalados:

### 1. GCC (compilador C)

No **PowerShell**, instale o MSYS2 e o GCC:

```powershell
winget install -e --id MSYS2.MSYS2
```

```powershell
C:\msys64\usr\bin\bash.exe -lc "pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc"
```

Adicione o GCC ao PATH da sessão atual:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
```

> Para não precisar repetir isso toda vez, adicione `C:\msys64\ucrt64\bin` às variáveis de ambiente permanentes do Windows (Painel de Controle → Sistema → Variáveis de Ambiente).

Verifique se funcionou:

```powershell
gcc --version
```

### 2. SQLite (amalgamação)

> **Os arquivos `sqlite3.h` e `sqlite3.c` já estão incluídos na pasta `database/` do repositório.** Se estiverem lá, pule esta etapa. Refaça apenas se os arquivos estiverem faltando ou corrompidos.

O SQLite precisa de dois arquivos que não estão no repositório por serem grandes. Baixe e extraia com um comando só:

```powershell
Invoke-WebRequest -Uri "https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip" -OutFile "sqlite.zip"
Expand-Archive -Path "sqlite.zip" -DestinationPath "sqlite_tmp"
Move-Item sqlite_tmp\sqlite-amalgamation-3490100\sqlite3.h database\
Move-Item sqlite_tmp\sqlite-amalgamation-3490100\sqlite3.c database\
Remove-Item -Recurse sqlite.zip, sqlite_tmp
```

---

## Compilação

A partir do diretório raiz do projeto:

```powershell
gcc src/main.c src/ui.c src/orquestrador.c src/net.c src/login.c database/db.c database/sqlite3.c `
  -o poc_chat.exe -I./libs -I./database `
  -lgdi32 -lshell32 -lmsimg32 -lws2_32 -mwindows -static-libgcc
```

> O `-static-libgcc` garante que o `.exe` seja portátil — funciona em qualquer Windows 10/11 sem precisar instalar o MinGW na máquina de destino.

Se o terminal não exibir nenhuma mensagem de erro, o arquivo `poc_chat.exe` foi gerado com sucesso.

---

## Execução

Dê dois cliques no `poc_chat.exe` ou execute pelo terminal:

```powershell
.\poc_chat.exe
```

Na primeira execução, o banco de dados `social_media.db` é criado automaticamente na mesma pasta.

---

## Como usar

### Criar uma conta

1. Abra o `poc_chat.exe`
2. Clique em **Criar Conta**
3. Preencha usuário e senha (confirmando a senha)
4. Clique em **Cadastrar** — você será redirecionado para o login

### Entrar

1. Preencha usuário e senha na tela de login
2. Clique em **Entrar** — o chat abre

### Conversar em rede local

Para trocar mensagens entre dois computadores na mesma rede:

**Computador A (quem aguarda conexão):**
1. Abra o `poc_chat.exe` e faça login
2. Clique em **Aguardar**
3. O Windows pode pedir permissão de firewall — clique em **Permitir**

**Computador B (quem inicia a conexão):**
1. Abra o `poc_chat.exe` e faça login
2. Digite o IP do Computador A no campo de IP
3. Clique em **Conectar**

Após a conexão ser estabelecida, as mensagens enviadas aparecem com o nome do usuário logado.

> **Teste na mesma máquina:** abra duas instâncias do `poc_chat.exe`, use `127.0.0.1` como IP. O loopback não passa pelo firewall, funciona sem permissão de admin.

---

## Estrutura do projeto

```
├── src/
│   ├── main.c            — ponto de entrada, detecção de modo, orquestração
│   ├── ui.c / ui.h       — toda a interface gráfica (Nuklear GDI)
│   ├── login.c / login.h — lógica de login e cadastro
│   ├── net.c / net.h     — servidor/cliente TCP via Winsock2
│   └── orquestrador.c/h  — ciclo de vida dos processos workers
├── database/
│   ├── db.c / db.h       — acesso ao banco SQLite
│   ├── schema.sql        — schema de referência (documentação)
│   ├── sqlite3.c         — amalgamação SQLite (baixar conforme instruções)
│   └── sqlite3.h         — header SQLite (baixar conforme instruções)
└── libs/
    ├── nuklear.h          — biblioteca de interface gráfica (header-only)
    └── nuklear_gdi.h      — backend GDI do Nuklear para Windows
```

---

## Features implementadas

- [x] Interface gráfica nativa Win32 com Nuklear
- [x] Arquitetura multiprocesso — UI, backend e rede em processos separados
- [x] IPC não-bloqueante via `CreatePipe` + `PeekNamedPipe`
- [x] Chat em tempo real via TCP (Winsock2)
- [x] Tela de login e cadastro de usuário
- [x] Banco de dados local com SQLite
- [x] Mensagens identificadas pelo nome do usuário logado

## Em desenvolvimento

- [ ] Lista de amigos e solicitações de amizade
- [ ] Feed de posts
