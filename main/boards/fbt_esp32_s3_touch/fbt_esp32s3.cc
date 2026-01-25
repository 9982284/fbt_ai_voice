#include <assets/lang_config.h>
#include <driver/spi_common.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_touch_cst816s.h>
#include <esp_log.h>
#include <fbt_ui_volume_control.h>
#include <wifi_station.h>

#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "device_state.h"
#include "display/lcd_display.h"
#include "driver/i2c_master.h"
#include "dual_network_board.h"
#include "fbt_manager.h"
#include "fbt_touch_device.h"
#include "fbt_touch_dispatcher.h"
#include "led/single_led.h"
#include "settings.h"

#define TAG "CustomBoard"

LV_FONT_DECLARE(font_puhui_20_4);
LV_FONT_DECLARE(font_awesome_20_4);
class CustomBoard : public DualNetworkBoard {
  private:
    Button key_button_;
    LcdDisplay *display_;

    FbtTouchDevice touch_device_;

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_GPIO;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_GPIO;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_GPIO;
        io_config.dc_gpio_num = DISPLAY_DC_GPIO;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_GPIO;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, true);

        // InitializeTouch();
        display_ = new SpiLcdDisplay(panel_io, panel,
                                     DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    };

    void InitializeTouch() {
        TouchParam touch_param = {
            .gpio_sda = GPIO_NUM_14,
            .gpio_scl = GPIO_NUM_38,
            .gpio_rst = GPIO_NUM_13,
            .gpio_int = GPIO_NUM_12,
            .display_width = DISPLAY_WIDTH,
            .display_height = DISPLAY_HEIGHT,
            .offset_x = 40,
            .offset_y = 0,
            .mirror_x = DISPLAY_MIRROR_X,
            .mirror_y = DISPLAY_MIRROR_Y,
            .swap_xy = DISPLAY_SWAP_XY,
        };
        touch_device_.Initialize(touch_param);
        /* FbtTouchDispatcher::Instance().AddListener(
            // 按下回调
            [this](const TouchPoint &point) -> bool {
                this->touchVoice(point, true);
                return false;
            },
            // 松开回调
            [this](const TouchPoint &point) {
                this->touchVoice(point, false);
            },
            nullptr); */
    }

    void SwitchNetwork() {
        xTaskCreate([](void *arg) {
            gpio_config_t io_conf = {};
            io_conf.pin_bit_mask = (1ULL << GPIO_NUM_4);
            io_conf.mode = GPIO_MODE_INPUT;
            gpio_config(&io_conf);

            // 读取初始状态
            int last_state = gpio_get_level(GPIO_NUM_4);
            ESP_LOGI(TAG, "GPIO4 initial: %d", last_state);
            Settings settings("network", true);
            int network_type = settings.GetInt("type");
            if (network_type != last_state) {
                CustomBoard *board = static_cast<CustomBoard *>(arg);
                board->SwitchNetworkType();
            }
            while (1) {
                vTaskDelay(pdMS_TO_TICKS(5000));
                int current_state = gpio_get_level(GPIO_NUM_4);
                if (current_state != last_state) {
                    ESP_LOGI(TAG, "GPIO4 changed: %d -> %d", last_state, current_state);
                    last_state = current_state;
                    Settings settings("network", true);
                    int network_type = settings.GetInt("type");
                    if (network_type != last_state) {
                        CustomBoard *board = static_cast<CustomBoard *>(arg);
                        board->SwitchNetworkType();
                    }
                }
            }
        },
                    "switch_network", 2048, this, 1, NULL);
    }

    void
    InitializeButtons() {
        key_button_.OnClick([this]() {
            auto &app = Application::GetInstance();
            app.ToggleChatState();
        });
        key_button_.OnMultipleClick([this]() {
            Settings settings("network", true);
            int network_type = settings.GetInt("type");
            if (network_type != 1) {
                {
                    Settings settings("wifi", true);
                    settings.SetInt("force_ap", 1);
                }
                GetDisplay()->ShowNotification(Lang::Strings::ENTERING_WIFI_CONFIG_MODE);
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
        },
                                    3);
        key_button_.OnLongPress([this]() {
            auto &app = Application::GetInstance();
            app.SetDeviceState(kDeviceStateIdle);
            vTaskDelay(pdMS_TO_TICKS(300));
            // 复位芯片
            esp_restart();
        });
    };

  public:
    CustomBoard() : DualNetworkBoard(ML307_TX_PIN, ML307_RX_PIN, GPIO_NUM_NC),
                    key_button_(KEY_BUTTON_GPIO, false, 1800, 300) {
        InitializeButtons();
        InitializeSpi();
        InitializeDisplay();
        SwitchNetwork();
        FbtTouchDispatcher::Instance().Init();
        InitializeTouch();

        auto &volumeConfrol = FbtUiVolumeControl::GetInstance();
        VolumeControlParam param = {};
        param.position_x = 10;
        param.position_y = 45;
        param.width = 40;
        param.height = 160;
        param.touch_area.min_x = DISPLAY_WIDTH - 40;
        param.touch_area.max_x = DISPLAY_WIDTH;
        param.touch_area.min_y = 90;
        param.touch_area.max_y = 140;
        volumeConfrol.Initialize(param);

        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            GetBacklight()->RestoreBrightness();
        }
        audio_codec.OnWakeUp([this](const std::string &command) {
            if (command == std::string(vb6824_get_wakeup_word())) {
                if (Application::GetInstance().GetDeviceState() == kDeviceStateIdle) {
                    ESP_LOGI(TAG, "wakeup_word: %s", command.c_str());
                    Application::GetInstance().WakeWordInvoke("hello zibby");
                }
            }
        });
    }

    ~CustomBoard() {
    }

    virtual AudioCodec *GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            16000, 16000,
            GPIO_NUM_15, // spk_bclk
            GPIO_NUM_16, // spk_ws
            GPIO_NUM_7,  // spk_dout
            GPIO_NUM_5,  // mic_sck
            GPIO_NUM_4,  // mic_ws
            GPIO_NUM_6   // mic_din
        );
        return &audio_codec;
    }

    virtual Display *GetDisplay() override {
        return display_;
    }

    virtual Led *GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual Backlight *GetBacklight() override {
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
            return &backlight;
        }
        return nullptr;
    }
};

DECLARE_BOARD(CustomBoard);