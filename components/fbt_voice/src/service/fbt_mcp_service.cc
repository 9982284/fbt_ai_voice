#include "fbt_mcp_service.h"
#include "fbt_http.h"
#include <esp_log.h>
#include <fbt_config.h>
#include <settings.h>
#include <string>

#define TAG "FbtMcpServer"

void FbtMcpServer::Init() {
    auto http_ = FbtHttp::Instance().SetupHttp();
    std::string url_ = FbtConfig::FbtBuilder::getUrl("get_tool");

    if (!http_->Open("GET", url_)) {
        ESP_LOGI(TAG, "Failed to get_tool error");
    }

    auto status_code = http_->GetStatusCode();
    if (status_code != 200) {
        ESP_LOGE(TAG, "Failed to get config status code: %d", status_code);
        http_->Close();
        return;
    }

    std::string data = http_->ReadAll();
    http_->Close();
    if (data.empty()) {
        return;
    }
    auto &mcp_server_ = McpServer::GetInstance();
    mcp_server_.SetRemoteToolsJson(data);
    mcp_server_.SetRemoteToolHandler([this](int id, const std::string &tool_name, const cJSON *args) {
        handle_remote_tool(id, tool_name, args);
    });
}

void FbtMcpServer::handle_remote_tool(int id, const std::string &tool_name, const cJSON *args) {

    // 在后台线程中执行HTTP调用
    cJSON *args_copy = args ? cJSON_Duplicate(args, true) : cJSON_CreateObject();
    std::thread([this, id, tool_name, args_copy]() {
        auto &app = Application::GetInstance();
        try {
            cJSON *root = cJSON_CreateObject();
            if (!root) {
                return;
            }
            cJSON_AddStringToObject(root, "name", tool_name.c_str());

            Settings config_setting("fbt_config", true);
            std::string device_id = config_setting.GetString("deviceId");
            cJSON_AddStringToObject(root, "deviceId", device_id.c_str());
            cJSON_AddItemToObject(root, "param", args_copy);

            char *json_str = cJSON_PrintUnformatted(root);
            std::string body;
            if (json_str) {
                body = json_str;
                free(json_str);
            }
            cJSON_Delete(root);

            auto http_ = FbtHttp::Instance().SetupHttp();
            http_->SetContent(std::move(body));
            std::string url_ = FbtConfig::FbtBuilder::getUrl("execute_mcp_tool");
            if (!http_->Open("POST", url_)) {
                ESP_LOGI(TAG, "execute_mcp_tool error");
            }

            auto status_code = http_->GetStatusCode();
            if (status_code != 200) {
                ESP_LOGE(TAG, "Failed to execute mcp tool status code: %d", status_code);
                http_->Close();
                return;
            }
            std::string result = http_->ReadAll();
            http_->Close();

            app.Schedule([this, id, result]() {
                auto &mcp_server_ = McpServer::GetInstance();
                mcp_server_.ReplyResult(id, result);
            });

        } catch (const std::exception &e) {
            std::string message = e.what();
            app.Schedule([this, id, message]() {
                auto &mcp_server_ = McpServer::GetInstance();
                mcp_server_.ReplyError(id, message.c_str());
            });
        }
    }).detach();
}
