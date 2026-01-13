#include "fbt_http.h"

#include "assets/lang_config.h"
#include "fbt_constants.h"
#include "settings.h"
#include "system_info.h"
#include <esp_log.h>
#include <fbt_config.h>

#define TAG "fbt_http"
FbtHttp::FbtHttp() {
}

FbtHttp::~FbtHttp() {
}

std::string FbtHttp::GetConfig() {
    std::string url = get_config_url();
    auto http = setup_http();
    std::string body = gen_config_body();
    if (body.empty()) {
        return "";
    }
    http->SetContent(std::move(body));
    if (!http->Open("POST", url)) {
        int last_error = http->GetLastError();
        ESP_LOGE(TAG, "Failed to open HTTP connection, code=0x%x", last_error);
        return "";
    }

    auto status_code = http->GetStatusCode();
    if (status_code != 200) {
        ESP_LOGE(TAG, "Failed to get config status code: %d", status_code);
        http->Close();
        return "";
    }

    std::string data = http->ReadAll();
    http->Close();
    return data;
}

std::unique_ptr<Http> FbtHttp::setup_http() {

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

std::string FbtHttp::get_ota_url() {
    Settings settings("wifi", false);
    std::string url = settings.GetString("ota_url");
    if (url.empty()) {
        url = CONFIG_OTA_URL;
    }
    return url;
}

std::string FbtHttp::get_config_url() {
    std::string url = FbtConfig::Config::fbt_server;
    if (url.back() != '/') {
        url += "/get_device_config";
    } else {
        url += "get_device_config";
    }
    return url;
}
