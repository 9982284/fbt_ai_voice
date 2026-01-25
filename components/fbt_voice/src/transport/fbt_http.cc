#include "fbt_http.h"

#include "assets/lang_config.h"
#include "fbt_constants.h"
#include "settings.h"
#include "system_info.h"
#include <esp_log.h>
#include <fbt_config.h>

#define TAG "fbt_http"

void FbtHttp::GetConfig() {
    std::string url = FbtConfig::FbtBuilder::getUrl("get_device_config");
    if (url.empty()) {
        return;
    }
    auto http = SetupHttp();
    std::string body = gen_config_body();
    if (body.empty()) {
        return;
    }
    http->SetContent(std::move(body));
    if (!http->Open("POST", url)) {
        int last_error = http->GetLastError();
        ESP_LOGE(TAG, "Failed to open HTTP connection, code=0x%x", last_error);
        return;
    }

    auto status_code = http->GetStatusCode();
    if (status_code != 200) {
        ESP_LOGE(TAG, "Failed to get config status code: %d", status_code);
        http->Close();
        return;
    }

    std::string data = http->ReadAll();
    http->Close();
    cJSON *root = cJSON_Parse(data.c_str());
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return;
    }
    Settings settings("fbt_config", true);
    cJSON *deviceId = cJSON_GetObjectItem(root, "deviceId");
    if (deviceId && cJSON_IsString(deviceId)) {
        settings.SetString("deviceId", deviceId->valuestring);
    }
    cJSON *mqtt = cJSON_GetObjectItem(root, "mqtt");
    if (cJSON_IsObject(mqtt)) {
        Settings settings("fbt_mqtt", true);
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, mqtt) {
            if (cJSON_IsString(item)) {
                if (settings.GetString(item->string) != item->valuestring) {
                    settings.SetString(item->string, item->valuestring);
                }
            } else if (cJSON_IsNumber(item)) {
                if (settings.GetInt(item->string) != item->valueint) {
                    settings.SetInt(item->string, item->valueint);
                }
            }
        }
    }
    cJSON *voice = cJSON_GetObjectItem(root, "voice");
    if (cJSON_IsObject(voice)) {
        Settings settings("fbt_voice", true);
        settings.SetString("groupId", "");
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, voice) {
            if (cJSON_IsString(item)) {
                if (settings.GetString(item->string) != item->valuestring) {
                    settings.SetString(item->string, item->valuestring);
                }
            } else if (cJSON_IsNumber(item)) {
                if (settings.GetInt(item->string) != item->valueint) {
                    settings.SetInt(item->string, item->valueint);
                }
            }
        }
    }
    cJSON_Delete(root);
    return;
}

std::unique_ptr<Http> FbtHttp::SetupHttp() {

    auto &board = Board::GetInstance();
    auto network = board.GetNetwork();
    auto http = network->CreateHttp(1);

    http->SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
    http->SetHeader("Accept-Language", Lang::CODE);
    http->SetHeader("Content-Type", "application/json");
    return http;
}

std::string FbtHttp::gen_config_body() {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return "";
    }
    cJSON_AddStringToObject(root, "mac", SystemInfo::GetMacAddress().c_str());

    char *json_str = cJSON_PrintUnformatted(root);
    std::string body;
    if (json_str) {
        body = json_str;
        free(json_str);
    }
    cJSON_Delete(root);

    return body;
}
