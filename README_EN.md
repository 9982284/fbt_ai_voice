# AI Communication System Based on MCP Protocol

([中文](README.md) | English)

## 📖 Project Introduction

- A high-performance AI voice communication solution based on the ESP32 platform, providing HD, low-latency voice calls and multi-party intercom capabilities for smart hardware devices.

- 👉 [An AI voice communication solution that enables AI to make calls, enter intercom rooms, manage groups, with special optimizations for mobile networks【bilibili】](https://www.bilibili.com/video/BV1X2kWB4ET7/?spm_id_from=333.1007.top_right_bar_window_dynamic.content.click&vd_source=f3da0e320c45f62d4876daddf98a6ba6)

## ✨ Core Capabilities

- **AI Intelligent Control**: Achieve AI automatic dialing, contact lookup, group joining and group rule compliance based on the MCP protocol.
- **Network Stability**: Proprietary private protocol that effectively copes with mobile network base station handovers and network fluctuations.
- **Ultra Streamlined Design**: Minimalist protocol architecture reduces bandwidth usage and improves stability.
- **Modular Design**: Adopts business manager and event bus architecture to achieve decoupling between modules and unified interface management, greatly improving code maintainability and scalability.
- **Audio Repeater Design**: Encapsulates system audio services through an audio repeater to provide standardized interfaces, which greatly simplifies code transplantation and multi-platform adaptation processes.

## 🏗️ Design Highlights

### ▸ Event-driven Architecture

- Module communication mechanism based on event bus, reducing coupling degree
- Supports plug-in expansion for rapid integration of new functions

### ▸ Platform-independent Audio Layer

- Audio repeater abstraction layer that shields underlying hardware differences
- Standardized audio interface for quick transplantation to new platforms

### ▸ Dual-channel Network Adaptation

- TCP/UDP hybrid strategy for complex network environments
- Intelligent retransmission and adaptive bit rate ensure call continuity

## ✅ Implemented Features

### 🎤 Voice Communication

- AI automatic dialing for voice calls
- AI automatic joining of intercom rooms for multi-party voice intercom
- Support multi-group management (join, switch)
- AI automatic contact lookup and communication management
- Multi-language support

### 🔊 Audio Processing

- Adopts OPUS single-channel audio codec
- Supports interconnection of devices with different sampling rates
- Echo cancellation and audio noise reduction support on ESP32S3
- Ringtone interruption support

### 📦 Data Transmission

- **AES-128 Encrypted Transmission**: Ensures the security and privacy of voice data
- **Raw Stream Transmission Mode**: Reserves interfaces for expanding more audio protocols in the future
- **Private Lightweight Protocol**: Minimalist design maximizes effective payload with extremely low bandwidth usage
- **UDP Media Stream Transmission**: Implemented based on UDP protocol, providing low-latency and high-efficiency real-time audio transmission
- **TCP/UDP Dual-channel Service Discovery**: Solves the problem of frequent UDP port/IP changes in 4G network environments through dual-protocol channels to ensure stable service reachability

## Future Roadmap

### Cross-platform Expansion

- Support Android/iOS APP, WebRTC web client, WeChat Mini Program
- Achieve full-scenario interconnection between AI devices, mobile terminals and desktop terminals

### Function Enhancement

- Video call and real-time video intercom functions
- Support for more audio codecs (G.711A/B, G.722, Speex, etc.)

## 📦 Update Notes

- **2026-01-18**
    - Restructured the project and released it in component form, removed dependencies on the main project CMakeLists, added Kconfig for project configuration;
    - Reconstructed each component in a modular way with clearer architecture;
    - Added UDP enhanced mode for mobile networks to further ensure reliable signaling transmission;
    - Added touch screen component associated with LVGL and a new touch dispatcher for global listening by other components;
    - Added partial UI components: volume adjustment, buttons, file tabs, images and other general components.

## Quick Integration into Your XiaoZhi Chatbot (`main`)

### Add Ringtone

#### Copy `main/assets/common/phone_in.ogg` to your project directory

#### Add ringtone file mapping in `main/assets/lang_config.h`

```cpp
namespace Lang {

    //.....Other existing content....
    namespace Sounds {

        //.....Other existing content....

        extern const char ogg_phone_in_start[] asm("_binary_phone_in_ogg_start");
        extern const char ogg_phone_in_end[] asm("_binary_phone_in_ogg_end");
        static const std::string_view OGG_PHONE_IN {
        static_cast<const char*>(ogg_phone_in_start),
        static_cast<size_t>(ogg_phone_in_end - ogg_phone_in_start)
        };
    }
}
```

#### Add Language Descriptions `assets/locales/en-US/language.json` (Same for other languages)

```json
{
    "language": {
        "type": "en-US"
    },
    "strings": {
        //.....Other existing content....

        "FBT_PHONE_STARTED": "In a call",
        "FBT_PHONE_AUDIO_IN": "Voice call incoming",
        "FBT_PHONE_VIDEO_IN": "Video call incoming",
        "FBT_PHONE_RINGING": "Waiting for answer",
        "FBT_PHONE_CLOSED": "Call ended",
        "FBT_HOLD_TO_TALK": "Intercom mode",
        "FBT_VOICE_SPEAKING": "Speaking now",
        "FBT_VOICE_IN_SPEECH": "Someone is speaking"
    }
}
```

### Add Device State (`device_state.h`)

```cpp
enum DeviceState {
    //....Other existing states...
    kDeviceStateFbtActive, // FBT service active state
    kDeviceStateFatalError
};
```

### Application Layer Integration

#### `application.h`

- **Define Global Events**

```cpp
#include "fbt_manager.h"

/**
 * Send audio event
 */
#define MAIN_EVENT_FBT_SEND_AUDIO (1 << 10)
/**
 * FBT service activation event, service managed by FBT
 */
#define MAIN_EVENT_FBT_START_SESSION (1 << 11)
/**
 * FBT service deactivation event, switch back to normal mode
 */
#define MAIN_EVENT_FBT_STOP_SESSION (1 << 12)
```

- **Define Manager**

```cpp
class Application {
  private:
    // ... Other existing members ...
    FbtManager &fbt_manager_;
};
```

### `application.cc` Modifications

- **Initialization**

```cpp
// Initialize FBT Manager in constructor
Application::Application() : fbt_manager_(FbtManager::GetInstance()) {
  // ... Other existing initialization ...
}
```

- **Startup Function `Application::Start()`**

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

- **Main Event Loop `Application::MainEventLoop()`**

```cpp
void Application::MainEventLoop() {
    while (true) {
        auto bits = xEventGroupWaitBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO | MAIN_EVENT_FBT_START_SESSION | MAIN_EVENT_FBT_STOP_SESSION | MAIN_EVENT_SCHEDULE | MAIN_EVENT_SEND_AUDIO | MAIN_EVENT_WAKE_WORD_DETECTED | MAIN_EVENT_VAD_CHANGE | MAIN_EVENT_CLOCK_TICK | MAIN_EVENT_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);

        //.....Other existing event handling.....

        // Pass microphone audio to FBT management
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

- **State Machine Processing `Application::SetDeviceState()`**

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

### Add Ringtone Interruption `main/audio/audio_service.cc`

```cpp
// audio_service.h
class AudioService {
  public:
    //......Other existing methods.....
    void FbtInterruptPlayback(); //Interrupt method
  private:
    //......Other existing members.....
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

- We believe that AI should not only be a passive Q&A machine, but also a **personal steward** that brings convenience to life. This project truly integrates AI into daily communication scenarios and delivers a more natural and efficient interactive experience.

- This project is developed based on the latest v2.0 open-source project of Brother Xia's XiaoZhi AI Chatbot for ESP32, [XiaoZhi Open-source Address](https://github.com/78/xiaozhi-esp32).

- Feel free to raise Issues if you have any ideas or suggestions.

### 🏢 About Starlink Software

We are a team with many years of experience focusing on audio and video communication and streaming media technology in emergency command and industrial communication fields.

**Our Products Include**:

- 🚨 **Converged Communication Platform** — Interconnection of multi-system voice dispatch
- 📹 **Audio & Video Convergence Platform** — Unified management of large-scale audio and video streams
- 🎮 **Command and Dispatch Platform** — Visual command and collaborative operations, integrating video conferencing, communication dispatch, video dispatch and other functions

**Application Industries**:

- Emergency management and fire rescue
- Judicial correctional facilities and public security
- Government command and dispatch centers
- Smart cities and the Internet of Things
