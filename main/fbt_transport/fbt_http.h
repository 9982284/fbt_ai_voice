#if !defined(FBT_HTTP)
#define FBT_HTTP

#include "board.h"
#include <memory>

class FbtHttp {

  public:
    FbtHttp(/* args */);
    ~FbtHttp();
    std::string GetConfig();

  private:
    std::unique_ptr<Http> setup_http();
    std::string gen_config_body();
    std::string get_ota_url();
    std::string get_config_url();
};

#endif // FBT_HTTP
