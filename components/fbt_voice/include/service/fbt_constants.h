// fbt_events.h (事件类型定义)
#pragma once
#include <string>

enum FbtServerState {
    kEventIdle,
    kEventPhone,
    kEventVoice
};

enum FbtPhoneState {
    /**
     * 空闲
     */
    IDLE,
    /**
     * 呼入
     */
    INCOMING,
    /**
     * 呼出
     */
    OUTGOING,
    /**
     * 通话中
     */
    CALLING,
    /**
     * 连接中
     */
    CONNECTING,
};

enum FbtRtcState {
    kIdle,
    kUnavailable,
    kPhoneRing,
    kOffer,
    kConnecting,
    kActive,
    kSpeaking,
    kTuneIn,

};
/**
 * 网络协议
 */
typedef enum : uint8_t {
    CONTROL = 0x00,          // 信令
    RELIABLE_CONTROL = 0x01, // 必达
    KEEPALIVE = 0x02,        // 心跳
    ACK = 0x05,              // 确认
    AUDIO = 0x10,            // 音频
    UNKNOWN = 0xFF           // 未知类型
} PacketType;

namespace FbtEvents {
    // 事件类型常量
    constexpr const char *START_PHONE = "start_phone";
    constexpr const char *CLOSE_PHONE = "close_phone";
    constexpr const char *SEND_TO_SERVER = "send_to_server";
    constexpr const char *PHONE_STARTED = "phone_started";
    constexpr const char *PHONE_CALL_READY = "phone_call_ready";
    constexpr const char *PHONE_CLOSED = "phone_closed";
    constexpr const char *MQTT_MESSAGE = "mqtt_message";
    constexpr const char *CLICK_BUTTON = "click_button";
    constexpr const char *MQTT_VOICE_PAYLOAD = "voice_payload";
    constexpr const char *MQTT_ENTER_INTERCOM_ROOM = "enter_intercom_room";
    constexpr const char *MQTT_VOICE_PING = "udp_voice_ping";
    constexpr const char *VOICE_START_ACTIVE = "voice_start_active";
    constexpr const char *VOICE_END_ACTIVE = "voice_end_active";

} // namespace FbtEvents

namespace FbtCommand {
    // 消息类型常量
    constexpr const char *PHONE_CALL = "fbt_phone_call";
    constexpr const char *PHONE_BYE = "fbt_phone_bye";
    constexpr const char *FBT_OFFER = "fbt_offer";
    constexpr const char *FBT_ANSWER = "fbt_answer";
    constexpr const char *ENTER_INTERCOM_ROOM = "enter_intercom_room";
} // namespace FbtCommand

namespace FbtStruct {
    // 配置结构
    struct PhoneSession {
        std::string deviceId;
        std::string session_id;
        std::string server_addr;
        FbtPhoneState callType;
        std::string name;
        std::string key;
        std::string nonce;
        int server_port;
        int sample_rate;
        int frame_duration;

        void Clear() {
            deviceId.clear();
            session_id.clear();
            server_addr.clear();
            key.clear();
            nonce.clear();
            name.clear();
            callType = FbtPhoneState::IDLE;
            server_port = 0;
            sample_rate = 16000;
            frame_duration = 60;
        }
    };
} // namespace FbtStruct
