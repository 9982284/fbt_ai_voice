#ifndef FBT_MCP_SERVICE_H
#define FBT_MCP_SERVICE_H

#include <application.h>
#include <mcp_server.h>

class FbtMcpServer {
  private:
    McpServer *mcp_server_;

  public:
    static FbtMcpServer &Instance() {
        static FbtMcpServer instance;
        return instance;
    }

    void Init();

  private:
    FbtMcpServer() {};
    ~FbtMcpServer() {};
    FbtMcpServer(const FbtMcpServer &) = delete;
    FbtMcpServer &operator=(const FbtMcpServer &) = delete;

    void handle_remote_tool(int id, const std::string &tool_name, const cJSON *args);
};

#endif