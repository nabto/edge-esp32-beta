#include "tcp_tunnel_state.h"

#include <modules/iam/nm_iam_to_json.h>
#include <modules/iam/nm_iam_from_json.h>

#include "json_config.h"
#include "random_string.h"

#include <cjson/cJSON.h>
#include <nn/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

static const char* LOGM = "tcp_tunnel_state";
static const char* STATEKEY = "tunnel_state";


static bool write_state_to_nvs(struct tcp_tunnel_state* state);
static bool create_default_tcp_tunnel_state();

void tcp_tunnel_state_init(struct tcp_tunnel_state* state)
{
    memset(state, 0, sizeof(struct tcp_tunnel_state));
    nn_vector_init(&state->users, sizeof(void*));
}

void tcp_tunnel_state_deinit(struct tcp_tunnel_state* state)
{
    nn_vector_deinit(&state->users);
    free(state->pairingPassword);
    free(state->pairingServerConnectToken);
}


bool load_tcp_tunnel_state(struct tcp_tunnel_state* state, struct nn_log* logger)
{

    
    if (!json_config_exists(STATEKEY)) {
        NN_LOG_INFO(logger, LOGM, "State file does not exists (%s), creating a new default NVS entry", STATEKEY);
        create_default_tcp_tunnel_state();
    }

    cJSON* json;

    if (!json_config_load(STATEKEY, &json, logger)) {
        return false;
    }

    cJSON* pairingPassword = cJSON_GetObjectItem(json, "PairingPassword");
    cJSON* pairingServerConnectToken = cJSON_GetObjectItem(json, "PairingServerConnectToken");
    cJSON* users = cJSON_GetObjectItem(json, "Users");

    if (cJSON_IsString(pairingPassword)) {
        state->pairingPassword = strdup(pairingPassword->valuestring);
    }

    if (cJSON_IsString(pairingServerConnectToken)) {
        state->pairingServerConnectToken = strdup(pairingServerConnectToken->valuestring);
    }

    if (users != NULL && cJSON_IsArray(users)) {

        size_t usersSize = cJSON_GetArraySize(users);
        for (size_t i = 0; i < usersSize; i++) {
            cJSON* item = cJSON_GetArrayItem(users, i);
            struct nm_iam_user* user = nm_iam_user_from_json(item);
            if (user != NULL) {
                nn_vector_push_back(&state->users, &user);
            }
        }
    }

    cJSON_Delete(json);

    return true;
}


bool create_default_tcp_tunnel_state()
{
    struct tcp_tunnel_state state;
    tcp_tunnel_state_init(&state);
    state.pairingPassword = random_password(20);
    state.pairingServerConnectToken = random_password(20);


    bool status = write_state_to_nvs(&state);

    tcp_tunnel_state_deinit(&state);

    return status;
}

bool write_state_to_nvs(struct tcp_tunnel_state* state)
{
    cJSON* json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "PairingPassword", cJSON_CreateString(state->pairingPassword));
    cJSON_AddItemToObject(json, "PairingServerConnectToken", cJSON_CreateString(state->pairingServerConnectToken));

    cJSON* usersArray = cJSON_CreateArray();

    struct nm_iam_user* user;
    NN_VECTOR_FOREACH(&user, &state->users)
    {
        cJSON* encodedUser = nm_iam_user_to_json(user);
        cJSON_AddItemToArray(usersArray, encodedUser);
    }
    cJSON_AddItemToObject(json, "Users", usersArray);

    json_config_save(STATEKEY, json);

    cJSON_Delete(json);
    return true;
}

bool save_tcp_tunnel_state(struct tcp_tunnel_state* state)
{
    return write_state_to_nvs(state);
}
