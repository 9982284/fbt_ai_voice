# AI Communication System Based on MCP

（ [中文](README.md) | English ）

## 📖 Project Introduction

-   A high-performance AI voice communication solution based on the ESP32 platform, providing HD, low-latency voice call and multi-party intercom capabilities for smart hardware。
-   This project is developed based on Brother Xia's open-source project - Xiaozhi AI Chatbot esp32 (latest V2.0 version). [Xiaozhi Open Source Address](https://github.com/78/xiaozhi-esp32)

## ✨ Core Capabilities

-   **AI Intelligent Control**: Realize AI automatic dialing, contact search, group joining and group rule compliance based on MCP protocol
-   **Network Stability**: Exclusive proprietary protocol, effectively adapt to mobile network base station switching and network fluctuations
-   **Ultimate Simplification**: Minimalist protocol design, reduce bandwidth occupation and improve stability
-   **Modular Design**: Adopt business manager and event bus design, realize decoupling between modules and unified interface management, greatly improve code maintainability and scalability.
-   **Audio Repeater Design**: Encapsulate system audio services through audio repeater and provide standardized interfaces, which greatly simplifies code transplantation and multi-platform adaptation process.

## 🏗️ Design Highlights

### ▸ Event-driven Architecture

-   Module communication mechanism based on event bus, reduce coupling degree
-   Support plug-in expansion, new functions can be integrated quickly

### ▸ Platform-independent Audio Layer

-   Audio repeater abstraction layer, shield underlying hardware differences
-   Standardized audio interface, support rapid transplantation to new platforms

### ▸ Dual-channel Network Adaptation

-   TCP/UDP hybrid strategy to cope with complex network environment
-   Intelligent retransmission and adaptive bit rate, ensure call continuity

## ✅ Implemented Functions

### 🎤 Voice Communication

-   AI automatic dialing for voice calls
-   AI automatically joins intercom rooms for multi-party voice intercom
-   Support multi-group management (create, join, switch)
-   AI automatically search contacts and manage communication
-   Multi-language support

### 🔊 Audio Processing

-   Adopt OPUS single-channel audio codec
-   Support intercommunication of devices with different sampling rates
-   ESP32S3 supports echo cancellation and audio noise reduction
-   Support ringtone interruption

### 📦 Data Transmission

-   **AES-128 Encrypted Transmission**: Ensure the security and privacy of voice data
-   **Raw Stream Transmission Mode**: Reserve interfaces for expanding more audio protocols in the future
-   **Private Lightweight Protocol**: Minimalist design, maximize effective payload, ultra-low bandwidth occupation
-   **UDP Media Stream Transmission**: Realized based on UDP protocol, providing low-latency and high-efficiency real-time audio transmission
-   **TCP/UDP Dual-channel Service Discovery**: Solve the problem of frequent UDP port/IP changes in 4G network environment through dual-protocol channels to ensure stable service reachability

## Future Planning

### Cross-platform Expansion

-   Support Android/iOS APP, WebRTC web client, WeChat Mini Program
-   Realize full-scenario interconnection between AI devices, mobile terminals and desktop terminals

### Function Enhancement

-   Video call and real-time video intercom functions
-   Support more audio codecs (G.711A/B, G.722, Speex, etc.)

## Quick Integration to Your Xiaozhi Chatbot (main)

### Copy FBT source code to your project

```
fbt_repeater
fbt_service
fbt_transport
fbt_utils
```

### Add Ringtone

#### Copy main/assets/common/phone_in.ogg to your project

#### Add ringtone file mapping on `main/assets/lang_config.h`

```cpp
namespace Lang {

    //.....Other existing content.....

    namespace Sounds {

        //.....Other existing content.....

        extern const char ogg_phone_in_start[] asm("_binary_phone_in_ogg_start");
        extern const char ogg_phone_in_end[] asm("_binary_phone_in_ogg_end");
        static const std::string_view OGG_PHONE_IN {
        static_cast<const char*>(ogg_phone_in_start),
        static_cast<size_t>(ogg_phone_in_end - ogg_phone_in_start)
        };
    }
}
```

#### Add language description `assets/locales/zh-CN/language.json` (Same for other languages)

```json
{
    "language": {
        "type": "zh-CN"
    },
    "strings": {
        //.....Other existing content....

        "FBT_PHONE_STARTED": "通话中",
        "FBT_PHONE_AUDIO_IN": "语音来电",
        "FBT_PHONE_VIDEO_IN": "视频来电",
        "FBT_PHONE_RINGING": "等待接听",
        "FBT_PHONE_CLOSED": "通话结束",
        "FBT_HOLD_TO_TALK": "对讲模式",
        "FBT_VOICE_SPEAKING": "正在讲话",
        "FBT_VOICE_IN_SPEECH": "发言中"
    }
}
```

### Add the following source files in the project's `CMakeLists.txt`:

```cmake
# Core service layer
set(SOURCES
    # ... Other existing files ...
    "fbt_service/fbt_manager.cc"
    "fbt_transport/fbt_phone_transport.cc"
    "fbt_transport/fbt_voice_transport.cc"
    "fbt_transport/fbt_mqtt_server.cc"
    "fbt_transport/fbt_http.cc"
    "fbt_repeater/fbt_audio_repeater.cc"
);
```

```cmake
# Include directory configuration
set(INCLUDE_DIRS
    "."
    # ... Other existing directories ...
    "fbt_service"
    "fbt_transport"
    "fbt_repeater"
    "fbt_utils"
)
```

### Add new device state (`device_state.h`)

```cpp
enum DeviceState {
    //....Other existing states...
    kDeviceStateFbtActive, // FBT service active state
    kDeviceStateFatalError
};
```

### Application Layer Integration

#### `application.h`

-   **Define global events**

```cpp
#include "fbt_manager.h"

/**
 * Send audio event
 */
#define MAIN_EVENT_FBT_SEND_AUDIO (1 << 10)
/**
 * FBT event activated, service managed by FBT
 */
#define MAIN_EVENT_FBT_START_SESSION (1 << 11)
/**
 * FBT event deactivated, switch back to normal mode
 */
#define MAIN_EVENT_FBT_STOP_SESSION (1 << 12)
```

-   **Define manager**

```cpp
class Application {
  private:
    // ... Other existing content ...
    FbtManager &fbt_manager_;
};
```

### Modify `application.cc`

-   **Initialization**

```cpp
// Initialize FBT manager in constructor
Application::Application() : fbt_manager_(FbtManager::GetInstance()) {
  // ... Other existing content ...
}
```

-   **Startup function `Application::Start()`**

```cpp
void Application::Start() {

    AudioServiceCallbacks callbacks;
    //**Important: Modify audio queue listening event
    callbacks.on_send_queue_available = [this]() {
        // Send to FBT if FBT service is running
        if (fbt_manager_.IsAudioStarted()) {
            xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO);
        } else {
            // Otherwise send to original protocol
            xEventGroupSetBits(event_group_, MAIN_EVENT_SEND_AUDIO);
        }
    };

      // Start FBT service manager
    if (!fbt_manager_.Start(event_group_, audio_service_)) {
        ESP_LOGW(TAG, "FBT services failed to start, continuing...");
    }

    //Modify chat close logic
    protocol_->OnAudioChannelClosed([this, &board]() {
        if (device_state_ != kDeviceStateFbtActive) {
            board.SetPowerSaveMode(true);
            ESP_LOGI(TAG, "Entering power save mode");
        } else {
            ESP_LOGI(TAG, "Skipping power save - FBT active");
        }

        Schedule([this]() {
            if (device_state_ != kDeviceStateFbtActive) {
                auto display = Board::GetInstance().GetDisplay();
                display->SetChatMessage("system", "");
                ESP_LOGI(TAG, "OnAudioChannelClosed to idle");
                SetDeviceState(kDeviceStateIdle);
            }
        });
    });

}
```

-   **Main Event Loop `Application::MainEventLoop()`**

```cpp
void Application::MainEventLoop() {
    while (true) {
        auto bits = xEventGroupWaitBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO | MAIN_EVENT_FBT_START_SESSION | MAIN_EVENT_FBT_STOP_SESSION | MAIN_EVENT_SCHEDULE | MAIN_EVENT_SEND_AUDIO | MAIN_EVENT_WAKE_WORD_DETECTED | MAIN_EVENT_VAD_CHANGE | MAIN_EVENT_CLOCK_TICK | MAIN_EVENT_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);

        //.....Other existing content.....

         // Transfer microphone audio to FBT management
        if (bits & MAIN_EVENT_FBT_SEND_AUDIO) {
            while (auto packet = audio_service_.PopPacketFromSendQueue()) {
                if (!fbt_manager_.SendAudio(std::move(packet))) {
                    ESP_LOGW(TAG, "Failed to send audio to FBT service");
                    break;
                }
            }
        }

        if (bits & MAIN_EVENT_FBT_START_SESSION) {
            ESP_LOGI(TAG, "MAIN_EVENT_FBT_START_SESSION");
            SetDeviceState(kDeviceStateFbtActive);
        }
        if (bits & MAIN_EVENT_FBT_STOP_SESSION) {
            ESP_LOGI(TAG, "MAIN_EVENT_FBT_STOP_SESSION");
            SetDeviceState(kDeviceStateIdle);
        }
    }
}
```

-   **State Machine Processing `Application::SetDeviceState()`**

```cpp
void Application::SetDeviceState(DeviceState state) {
  switch (state) {
      //Add FBT state machine
      case kDeviceStateFbtActive:
          protocol_->CloseAudioChannel();
          //Enable audio processing
          audio_service_.EnableVoiceProcessing(true);
          //Disable wake word detection
          audio_service_.EnableWakeWordDetection(false);
          break;
  }
}
```

#### Add Ringtone Interruption `main/audio/audio_service.cc`

```cpp
// audio_service.h

class AudioService {
  public:
    //......Other existing content.....
    void FbtInterruptPlayback(); //Interrupt method
  private:
    //......Other existing content.....
    bool fbt_interrupt_playback_; //Interrupt flag
};
```

```cpp
// audio_service.cc

void AudioService::FbtInterruptPlayback() {
    fbt_interrupt_playback_ = true;
    std::lock_guard<std::mutex> lock(audio_queue_mutex_);
    audio_decode_queue_.clear();
    audio_playback_queue_.clear();
    timestamp_queue_.clear();
    audio_queue_cv_.notify_all();
}
```

```cpp
//Ringtone playback
void AudioService::PlaySound(const std::string_view &ogg) {

    //......Other existing content.....
    while (true) {
        //......Other existing content.....
        for (int i = 0; i < page_segments; i++) {
            //......Other existing content.....
               // Check for interruption
            if (fbt_interrupt_playback_) {
                fbt_interrupt_playback_ = false;
                return;
            }

            // Create Opus packet
            auto packet = std::make_unique<AudioStreamPacket>();
            packet->sample_rate = 16000;
            packet->frame_duration = 60;
            packet->payload.resize(segment_size);
            memcpy(packet->payload.data(), buf + segment_offset, segment_size);

            if (!PushPacketToDecodeQueue(std::move(packet), true)) {
                ESP_LOGW(TAG, "Push failed");
                return;
            }
        }
    }
}
```

## 💬 About The Project

-   We believe that AI should not only be a passive Q&A machine, but also a **housekeeper** that facilitates life. This project integrates AI into daily communication scenarios in a real sense, bringing a more natural and efficient interactive experience.
-   This is an ESP32 AI voice communication project open-sourced based on "Brother Xia's Xiaozhi AI Chatbot".
-   If you have any ideas or suggestions, please feel free to raise Issues.

### 🏢 About Starlink Software

We are a team focusing on audio and video communication & streaming media technology, with many years of experience in emergency command and industrial communication fields.

**Our Products Include**:

-   🚨 **Converged Communication Platform** - Interconnection of multi-system voice dispatch
-   📹 **Audio & Video Convergence Platform** - Unified management of large-scale audio and video
-   🎮 **Command and Dispatch Platform** - Visual command and collaborative combat, integrating video conferencing, communication dispatch, video dispatch and other functions

**Application Industries**:

-   Emergency management and fire rescue
-   Judicial supervision and public security
-   Government command and dispatch center
-   Smart city and Internet of Things
