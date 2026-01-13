
#include <cJSON.h>
#include <string>

namespace FbtConfig {
    /**
     * 系统配置
     */
    namespace Config {
        /**
         * 是否显示对讲面页的活跃小点
         */
        constexpr bool show_voice_dot = true;
        /**
         * FBT 服务器地址
         */
        constexpr const char *fbt_server = "http://ota.zibby.me";

    } // namespace Config

    /**
     * 音频配置，对应着你 audio_service 的音频配置
     */
    namespace Audio {
        /**
         * 默认的采样率
         */
        constexpr int sample_rate = 24000;
        /**
         * 默认的帧时长（ms);
         */
        constexpr int frame_duration = 60;
        constexpr int channels = 1;
    } // namespace Audio

    class FbtBuilder {
      public:
        static cJSON *buildAudioMedia() {
            cJSON *root = cJSON_CreateObject();
            if (!root) {
                return nullptr;
            };

            cJSON_AddStringToObject(root, "format", "opus");
            cJSON_AddNumberToObject(root, "sampleRate", Audio::sample_rate);
            cJSON_AddNumberToObject(root, "frameDuration", Audio::frame_duration);
            cJSON_AddNumberToObject(root, "channels", Audio::channels);
            return root;
        };
    };

    class Helper {
      public:
        static std::string DecodeHexString(const std::string &hex_string) {
            std::string decoded;
            if (hex_string.size() % 2 != 0) {
            }

            decoded.reserve(hex_string.size() / 2);
            for (size_t i = 0; i < hex_string.size(); i += 2) {
                if (i + 1 >= hex_string.size()) {
                    break; // 处理奇数长度的情况
                }
                char byte = (CharToHex(hex_string[i]) << 4) | CharToHex(hex_string[i + 1]);
                decoded.push_back(byte);
            }
            return decoded;
        };

      private:
        static inline uint8_t CharToHex(char c) {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            return 0; // 对于无效输入，返回0
        };
    };

} // namespace FbtConfig
