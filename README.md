# 基于 MCP 的 AI 通讯系统

（中文 | [English](README_EN.md) ）

## 📖 项目介绍

- 一款基于 ESP32 平台 的高性能 AI 语音通讯解决方案，为智能硬件提供高清、低延迟的语音通话与多方对讲能力。

- 👉 [一套 AI 语音通讯方案，可以让ai帮你拔打电话，进入对讲室，管理群组，针对移动网络的特殊优化【bilibili】](https://www.bilibili.com/video/BV1X2kWB4ET7/?spm_id_from=333.1007.top_right_bar_window_dynamic.content.click&vd_source=f3da0e320c45f62d4876daddf98a6ba6)

## ✨ 核心能力

- **AI 智能控制：** 基于 MCP 协议实现 AI 自动拨号、查找联系人、加入群组、遵守群规则

- **网络稳定性：** 独特的私有协议，有效应对移动网络基站切换和网络波动

- **极致精简：** 协议设计极简，减少带宽占用，提高稳定性

- **模块化设计：** 采用业务管理器与事件总线设计，实现模块间解耦与统一接口管理，大幅提升代码可维护性与扩展性。

- **音频中继器设计：** 通过音频中继器封装系统音频服务，提供标准化接口，极大简化代码移植与多平台适配过程;

## 🏗️ 设计亮点

#### ▸ 事件驱动架构

- 基于事件总线的模块通信机制，降低耦合度

- 支持插件化扩展，新功能可快速集成

#### ▸ 平台无关音频层

- 音频中继器抽象层，屏蔽底层硬件差异

- 标准化音频接口，支持快速移植到新平台

#### ▸ 双通道网络适配

- TCP/UDP 混合策略应对复杂网络环境

- 智能重传与码率自适应，保障通话连续性

## ✅ 已实现功能

#### 🎤 语音通讯

- AI 自动拨号进行语音通话

- AI 自动加入对讲室进行多方语音对讲

- 支持多群组管理（加入、切换）

- AI 自动查找联系人并管理通讯

- 支持多语言;

#### 🔊 音频处理

- 采用 OPUS 单通道音频编解码

- 支持不同采样率设备互通

- ESP32S3 支持回声消除和音频降噪

- 支持铃声中断

#### 📦 数据传输

- **AES-128 加密传输**：保障语音数据的安全性和隐私性

- **裸流传输模式**：为未来扩展更多音频协议预留接口

- **私有轻量协议**：极简设计，最大化有效载荷，带宽占用极低

- **UDP 媒体流传输**：基于 UDP 协议实现，提供低延迟、高效率的实时音频传输

- **TCP/UDP 双通道服务发现**：通过双协议通道解决在 4G 网络环境下 UDP 端口/IP 频繁变动的问题，确保服务稳定可达

## 未来规划

#### 跨平台扩展

- 支持 Android/iOS APP、WebRTC 网页端、微信小程序

- 实现 AI 设备与移动端、桌面端的全场景互联

#### 功能增强

- 视频通话与实时视频对讲功能

- 更多音频编解码器支持（G.711A/B、G.722、Speex 等）

## 📦 更新说明

- **2026-01-18**
    - 重构项目，以组件的形式发布, 去除项目主 CMakeLists 的依懒，新增 使用 Kconig 对项目进行配置;
    - 模块化重构各组件，架构更清晰；
    - 新增针对移动网络udp增强模式，进一步确保信令必达;
      = 新增触模屏组件，关联lvgl,并新增触模分发器,代其他组件全局监听;
    - 新增部份ui组件, 音量调节、按钮、文件标签、图片等通用组件;

## 快速集成到你的小智聊天机器人上（`main`）

#### 添加铃声

##### 复制 main/assets/common/phone_in.ogg 到你的项目上

##### 在 `main/assets/lang_config.h` 上添加铃声文件映射

```cpp

namespace Lang {

    //.....其他已有....
    namespace Sounds {

        //.....其他已有....

        extern const char ogg_phone_in_start[] asm("_binary_phone_in_ogg_start");
        extern const char ogg_phone_in_end[] asm("_binary_phone_in_ogg_end");
        static const std::string_view OGG_PHONE_IN {
        static_cast<const char*>(ogg_phone_in_start),
        static_cast<size_t>(ogg_phone_in_end - ogg_phone_in_start)
        };
    }
}
```

#### 添加语言描述 `assets/locales/zh-CN/language.json` （其他语言类似）

```json
{
    "language": {
        "type": "zh-CN"
    },
    "strings": {
        //.....其他已有....

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

### 新增设备状态 (`device_state.h`)

```cpp
enum DeviceState {
    //....其他已有...
    kDeviceStateFbtActive, // FBT 服务活跃状态
    kDeviceStateFatalError
};

```

### 应用层集成

#### `application.h`

- **定义全局事件**

```cpp

#include "fbt_manager.h"

/**
 * 发送音频事件
 */
#define MAIN_EVENT_FBT_SEND_AUDIO (1 << 10)
/**
 * 开始fbt 事件活跃, 服务交给 fbt 管理
 */
#define MAIN_EVENT_FBT_START_SESSION (1 << 11)
/**
 * fbt 事件活跃已停止，切换回普通模式
 */
#define MAIN_EVENT_FBT_STOP_SESSION (1 << 12)
```

- **定义管理器**

```cpp
class Application {
  private:
    // ... 其他已有 ...
    FbtManager &fbt_manager_;
};
```

### `application.cc` 修改

- **初始化**

```cpp
// 构造函数中初始化 FBT 管理器
Application::Application() : fbt_manager_(FbtManager::GetInstance()) {
  // ... 其他已有 ...
}

```

- **启动函数 `Application::Start()`**

```cpp
void Application::Start() {

    AudioServiceCallbacks callbacks;
    //**重要: 修改音频队列监听事件
    callbacks.on_send_queue_available = [this]() {
        // 如果 FBT 服务正在运行，发送到 FBT
        if (fbt_manager_.IsAudioStarted()) {
            xEventGroupSetBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO);
        } else {
            // 否则发送到原来的协议
            xEventGroupSetBits(event_group_, MAIN_EVENT_SEND_AUDIO);
        }
    };

      // 启动 FBT 服务管理器
    if (!fbt_manager_.Start(event_group_, audio_service_)) {
        ESP_LOGW(TAG, "FBT services failed to start, continuing...");
    }

    //修改聊天的关闭逻辑
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

- **主事件循环 `Application::MainEventLoop()`**

```cpp
void Application::MainEventLoop() {
    while (true) {
        auto bits = xEventGroupWaitBits(event_group_, MAIN_EVENT_FBT_SEND_AUDIO | MAIN_EVENT_FBT_START_SESSION | MAIN_EVENT_FBT_STOP_SESSION | MAIN_EVENT_SCHEDULE | MAIN_EVENT_SEND_AUDIO | MAIN_EVENT_WAKE_WORD_DETECTED | MAIN_EVENT_VAD_CHANGE | MAIN_EVENT_CLOCK_TICK | MAIN_EVENT_ERROR, pdTRUE, pdFALSE, portMAX_DELAY);

        //.....其他已有.....


         // 把麦克风的音频交给FBT管理
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

- **状态机处理 `Application::SetDeviceState()`**

```cpp
void Application::SetDeviceState(DeviceState state) {
  switch (state) {
      //添加fbt状态机
      case kDeviceStateFbtActive:
          protocol_->CloseAudioChannel();
          //启用音频处理
          audio_service_.EnableVoiceProcessing(true);
          //关闭语音唤醒
          audio_service_.EnableWakeWordDetection(false);
          break;
  }
}

```

#### 添加铃声中断 `main/audio/audio_service.cc`

```cpp

// audio_service.h

class AudioService {
  public:
    //......其他已有.....
    void FbtInterruptPlayback(); //中断的方法
  private:
    //......其他已有.....
    bool fbt_interrupt_playback_; //中断标识
}
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
//铃声播放
void AudioService::PlaySound(const std::string_view &ogg) {

    //......其他已有.....
    while (true) {
        //......其他已有.....
        for (int i = 0; i < page_segments; i++) {
            //......其他已有.....
               // 检查中断
            if (fbt_interrupt_playback_) {
                fbt_interrupt_playback_ = false;
                return;
            }

            // 创建Opus包
            auto packet = std::make_unique<AudioStreamPacket>();
            packet->sample_rate = 16000;
            packet->frame_duration = 60;
            packet->payload.resize(segment_size);
            memcpy(packet->payload.data(), buf + segment_offset, segment_size);

            if (!PushPacketToDecodeQueue(std::move(packet), true)) {
                ESP_LOGW(TAG, "推送失败");
                return;
            }
        }
    }
}
```

## 💬 关于项目

- 我们觉得，AI 不应只是被动的问答机器，更应是为生活提供便利的**小管家**。这个项目让 AI 真正融入日常通讯场景，带来更自然、高效的交互体验。

- 本项目基于虾哥的 小智 AI 聊天机器人 esp32 最新的 2.0 版本开源项目进行开发，[小智开源地址](https://github.com/78/xiaozhi-esp32)。

- 如果你有任何想法或建议，请随时提出 Issues

### 🏢 关于星链软件

我们是一个专注于音视频通讯与流媒体技术在应急指挥、行业通讯领域深耕多年的团队。

**产品包括：**

- 🚨 **融合通讯平台**——多系统语音调度互通
- 📹 **音视频汇聚平台**——大规模音视频统一管理
- 🎮 **指挥调度平台**——可视化指挥与协同作战，集成视频会议、通讯调度、视频调度等功能

**应用行业：**

- 应急管理与消防救援
- 司法监所与公共安全
- 政府指挥调度中心
- 智慧城市与物联网
