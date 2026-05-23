#define STB_DS_IMPLEMENTATION

#include <libwebsockets.h>
#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include <stdbool.h>

int conection_alive_number = 0;



typedef struct {
    struct lws *connection;
    char name[100];
} UserInChat;


typedef struct {
    char content[200];
    char to[200];
    bool for_all_users;
} ReceveidMessage;


UserInChat *pool;



void init_pool(int size) {
    // 2. Aloca a memória em tempo de execução
    pool = malloc(size * sizeof(UserInChat));

    if (pool == NULL) {
        printf("Erro: Falha na alocação de memória!\n");
        exit(1);
    }
}


ReceveidMessage json_to_receveid_message(char json[300]) {

    ReceveidMessage message;

    cJSON *parsed_json = cJSON_Parse(json);

    cJSON *content = cJSON_GetObjectItem(parsed_json, "content");
    cJSON *to = cJSON_GetObjectItem(parsed_json, "to");
    cJSON *for_all_users = cJSON_GetObjectItem(parsed_json, "all_users");




    if (cJSON_IsString(content) && (content->valuestring != NULL)) {
        strncpy(message.content, content->valuestring, sizeof(message.content) - 1);
        message.to[sizeof(message.content) - 1] = '\0';
    }

    if (cJSON_IsString(to) && (to->valuestring != NULL)) {
        strncpy(message.to, to->valuestring, sizeof(message.to) - 1);
        message.to[sizeof(message.to) - 1] = '\0';
    }

    if (cJSON_IsBool(for_all_users)) {
       message.for_all_users =  cJSON_IsTrue(for_all_users);
    }


    return message;

}

void send_to_dest(char *msg, struct lws *to) {
    size_t msg_len = strlen(msg);
    lws_write(to, msg, msg_len, LWS_WRITE_TEXT);
}




void append_alive_user(struct lws *wsi) {
    char name[50];
    char default_name[100] = "Teste";
    UserInChat connection;


    // Concatena texto, string e número na variável 'mensagem'
    snprintf(name, sizeof(name), "%s %d", default_name, conection_alive_number + 1);


    strcpy(connection.name, name);

    connection.connection = wsi;


    pool[conection_alive_number] = connection;

    printf("%s foi adicionado ao chat\n", name);

    conection_alive_number++;
}


void remove_alive_user(struct lws *wsi) {
    UserInChat empty_user;

    for (int i = 0; i < conection_alive_number; i++) {

        UserInChat temp_connection = pool[i];


        if (temp_connection.connection == wsi) {
            printf("%s foi desconectado do chat \n", temp_connection.name);
            pool[i] = empty_user;
            conection_alive_number--;
        }
    }

    fflush(stdout);
}


void send_msg_to_all_users(const struct lws *user_wsi, const char *msg) {
    for (int i = 0; i < conection_alive_number; i++) {
        struct lws *temp_wsi  = pool[i].connection;
        if (user_wsi != temp_wsi) {
            send_to_dest(msg, temp_wsi);
        }
    }
}

struct lws* search_wsi(const char search[100]) {
    for (int i = 0; i<conection_alive_number;i++) {
        if (strcmp(pool[i].name, search) == 0) {
            return pool[i].connection;
        }
    }

    return NULL;
}




static int callback_ws(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            printf("Cliente conectado\n");

            append_alive_user(wsi);
            printf("Número conexões ativo: %d\n", conection_alive_number);
            break;

        case LWS_CALLBACK_RECEIVE:
            char receveid_msg[1024];

            memcpy(receveid_msg, in, len);
            receveid_msg[len] = '\0';

            printf("Recebido a mensagem: %s\n", receveid_msg);

            ReceveidMessage message = json_to_receveid_message(receveid_msg);

            if (message.to != NULL) {

                if (message.for_all_users == true) {
                    printf("Enviando mensagem geral ...", message.to);

                    // Passa o wsi do usuário atual
                    send_msg_to_all_users(wsi, message.content);
                }
                else {
                    printf("Para: %s\n", message.to);

                    struct lws *dest_wsi = search_wsi(message.to);

                    if (dest_wsi != NULL) {
                        send_to_dest(message.content, dest_wsi);
                    }
                }
            }

            break;

        case LWS_CALLBACK_CLOSED:
            remove_alive_user(wsi);
            printf("Cliente desconectado\n");
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        .name = "teste",
        .callback = callback_ws,
        .per_session_data_size = 0,
        .rx_buffer_size = 4096,
        .id = 0,
        .user = NULL,
        .tx_packet_size = 0,
    },
    LWS_PROTOCOL_LIST_TERM
};

int main()
{
    struct lws_context_creation_info info;

    init_pool(10);

    memset(&info, 0, sizeof(info));

    info.port = 9000;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_DISABLE_IPV6;

    struct lws_context *context = lws_create_context(&info);

    if (!context)
    {
        printf("Erro ao criar servidor\n");
        return -1;
    }

    printf("Servidor WebSocket na porta 9000\n");

    while (lws_service(context, 0) >= 0);

    lws_context_destroy(context);

    return 0;
}