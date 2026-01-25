# AI Communication System Based on MCP

( [中文](README.md) | English )

## 📖 Project Introduction

- A high-performance AI voice communication solution based on the **ESP32 platform**, providing HD, low-latency voice calls and multi-party intercom capabilities for smart hardware.

- 👉[Go here to connect devices after flashing **[Backend Management URL]**](https://voice.fbtai.cn/)

- 👉[Synchronized updates on Gitee, **[For domestic users - access via Gitee]**](https://gitee.com/fbtgateway/fbt_ai_voice)

- 👉 [An AI voice communication solution that enables AI to make calls, join intercom rooms, manage groups, with special optimizations for mobile networks **[Bilibili]**](https://www.bilibili.com/video/BV1f6zrBCEVw/?spm_id_from=333.788.recommend_more_video.-1&trackid=web_related_0.router-related-2206419-k4qpm.1769317377521.3)

## 📦 Update Notes

- **2026-01-25**
    - **Added enhanced UDP service**, supporting reliable UDP message delivery;
    - **Added MCP localization feature**: No longer relies on server-side MCP integration—devices directly send MCP tool descriptions to large models (**wider compatibility**);
    - Removed redundant board configurations, added a new touch panel configuration;
    - Service parameter optimization: Optimized configurations required for component startup to be loaded from memory, resolving the issue of being unable to connect to the server at all when there is no network during startup;
    - Additional documentation is being updated....

- **2026-01-18**
    - Project restructuring, released as component-based architecture; removed dependencies on the project's main CMakeLists, added project configuration via Kconfig;
    - Modular refactoring of all components for clearer architecture;
    - Added UDP enhancement mode for mobile networks to further ensure reliable signaling delivery;
    - Added touchscreen component, integrated with LVGL, and implemented a touch dispatcher for global event listening by other components;
    - Added partial UI components: Volume adjustment, buttons, file labels, images, and other general-purpose components;

## ✨ Core Capabilities

- **AI Intelligent Control**: Implements AI automatic dialing, contact lookup, group joining, and group rule compliance based on the MCP protocol.

- **Network Stability**: Proprietary protocol that effectively handles base station handovers and network fluctuations in mobile networks.

- **Extreme Compactness**: Minimalist protocol design reduces bandwidth usage and improves stability.

- **Modular Design**: Adopts a business manager and event bus architecture to achieve decoupling between modules and unified interface management, greatly enhancing code maintainability and scalability.

- **Audio Relay Design**: Encapsulates system audio services via an audio relay to provide standardized interfaces, significantly simplifying code porting and cross-platform adaptation.

## 🏗️ Design Highlights

#### ▸ Event-Driven Architecture

- Module communication mechanism based on an event bus reduces coupling.
- Supports plug-and-play expansion for rapid integration of new features.

#### ▸ Platform-Agnostic Audio Layer

- Audio relay abstraction layer shields underlying hardware differences.
- Standardized audio interfaces enable quick porting to new platforms.

#### ▸ Dual-Channel Network Adaptation

- Hybrid TCP/UDP strategy for complex network environments.
- Intelligent retransmission and adaptive bitrate control ensure call continuity.

## ✅ Implemented Features

#### 🎤 Voice Communication

- AI-automatic dialing for voice calls.
- AI-automatic joining of intercom rooms for multi-party voice intercom.
- Support for multi-group management (join, switch).
- AI-automatic contact lookup and communication management.
- Multi-language support.

#### 🔊 Audio Processing

- Single-channel audio codec using OPUS.
- Interoperability between devices with different sampling rates.
- Echo cancellation and audio noise reduction supported on ESP32S3.
- Ringtone interruption support.

#### 📦 Data Transmission

- **AES-128 Encrypted Transmission**: Ensures voice data security and privacy.
- **Raw Stream Transmission Mode**: Reserves interfaces for future expansion of additional audio protocols.
- **Proprietary Lightweight Protocol**: Minimalist design maximizes effective payload with ultra-low bandwidth consumption.
- **UDP Media Stream Transmission**: Implemented based on the UDP protocol for low-latency, high-efficiency real-time audio transmission.
- **TCP/UDP Dual-Channel Service Discovery**: Resolves issues of frequent UDP port/IP changes in 4G network environments through dual-protocol channels, ensuring stable service reachability.

## Future Roadmap

#### Cross-Platform Expansion

- Support for Android/iOS apps, WebRTC web clients, and WeChat Mini Programs.
- Achieve full-scenario interconnection between AI devices, mobile terminals, and desktop terminals.

#### Feature Enhancement

- Video call and real-time video intercom functionality.
- Support for additional audio codecs (G.711A/B, G.722, Speex, etc.).

## Quick Integration into Your Xiaozhi Chatbot (`main`)

#### Add Ringtones

##### Copy `main/assets/common/phone_in.ogg` to your project directory.

#### Add Language Descriptions `assets/locales/zh-CN/language.json` (similar for other languages)

```json
{
    "language": {
        "type": "zh-CN"
    },
    "strings": {
        //.....other existing entries....

        "FBT_PHONE_STARTED": "In Call",
        "FBT_PHONE_AUDIO_IN": "Voice Call Incoming",
        "FBT_PHONE_VIDEO_IN": "Video Call Incoming",
        "FBT_PHONE_RINGING": "Waiting for Answer",
        "FBT_PHONE_CLOSED": "Call Ended",
        "FBT_HOLD_TO_TALK": "Push-to-Talk Mode",
        "FBT_VOICE_SPEAKING": "Speaking",
        "FBT_VOICE_IN_SPEECH": "Someone is Speaking"
    }
}
```

### Add Device States (`device_state.h`)

```cpp
enum DeviceState {
    //....other existing states...
    kDeviceStateFbtActive, // FBT Service Active State
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
 * FBT service activation event, service handed over to FBT management
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
    // ... other existing members ...
    FbtManager &fbt_manager_;
};
```

### Modifications to `application.cc`

- **Initialization**

```cpp
// Initialize FBT Manager in the constructor
Application::Application() : fbt_manager_(FbtManager::GetInstance()) {
  // ... other existing initialization code ...
}
```

- **Start Function `Application::Start()`**

```cpp
void Application::Start() {
    AudioServiceCallbacks callbacks;
    //**Important: Modify audio queue listener event
    callbacks.on_send_queue_available = [this]() {
        // Send to FBT if FBT service is running
        if (fbt_manager_.IsAudioStarted()) {
            xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO);
        } else {
            // Otherwise send to original protocol
            xEventGroupSetBits(event_group_, MAIN_EVENT_SEND_AUDIO);
        }
    };

    // Start FBT Service Manager
    if (!fbt_manager_.Start(event_group_, audio_service_)) {
        ESP_LOGW(TAG, "FBT services failed to start, continuing...");
    }

    // Modify chat close logic
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

        //.....other existing event handling.....

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

- **State Machine Handling `Application::SetDeviceState()`**

```cpp
void Application::SetDeviceState(DeviceState state) {
  switch (state) {
      // Add FBT state machine
      case kDeviceStateFbtActive:
          protocol_->CloseAudioChannel();
          // Enable audio processing
          audio_service_.EnableVoiceProcessing(true);
          // Disable wake word detection
          audio_service_.EnableWakeWordDetection(false);
          break;
      // Handle other states
  }
}
```

#### Add Ringtone Interruption `main/audio/audio_service.cc`

```cpp
// audio_service.h
class AudioService {
  public:
    //......other existing methods.....
    void FbtInterruptPlayback(); // Interruption method
  private:
    //......other existing members.....
    bool fbt_interrupt_playback_; // Interruption flag
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
// Ringtone playback
void AudioService::PlaySound(const std::string_view &ogg) {
    //......other existing code.....
    while (true) {
        //......other existing code.....
        for (int i = 0; i < page_segments; i++) {
            //......other existing code.....
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
                ESP_LOGW(TAG, "Packet push failed");
                return;
            }
        }
    }
}
```

## 💬 About the Project

- We believe that AI should not be merely a passive question-answering tool, but rather a **smart assistant** that brings convenience to daily life. This project integrates AI into real-world communication scenarios, delivering a more natural and efficient interactive experience.

- This project is developed based on the latest v2.0 open-source version of Brother Xia's Xiaozhi AI Chatbot for ESP32, [Xiaozhi Open Source Repository](https://github.com/78/xiaozhi-esp32).

- If you have any ideas or suggestions, please feel free to submit Issues.

### 🏢 About Starlink Software

We are a team with years of experience in audio/video communication and streaming media technology, focusing on emergency command and professional communication sectors.

**Our Products Include**:

- 🚨 **Converged Communication Platform** — Interoperable voice dispatch across multiple systems
- 📹 **Audio/Video Convergence Platform** — Unified management of large-scale audio/video streams
- 🎮 **Command and Dispatch Platform** — Visual command and collaborative operations, integrating video conferencing, communication dispatch, and video scheduling functions

**Industry Applications**:

- Emergency management and fire rescue
- Judicial correction and public security
- Government command and dispatch centers
- Smart cities and the Internet of Things
