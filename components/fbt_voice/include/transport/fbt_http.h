#if !defined(FBT_HTTP)
#define FBT_HTTP

#include "board.h"
#include <memory>

class FbtHttp {

  public:
    static FbtHttp &Instance() {
        static FbtHttp instance;
        return instance;
    }

    void GetConfig();
    std::unique_ptr<Http> SetupHttp();

  private:
    FbtHttp() {};
    ~FbtHttp() {};

    FbtHttp(const FbtHttp &) = delete;
    FbtHttp &operator=(const FbtHttp &) = delete;

    std::string gen_config_body();
};

#endif // FBT_HTTP
