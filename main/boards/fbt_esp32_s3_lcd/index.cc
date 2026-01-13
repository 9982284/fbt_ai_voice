#include "application.h"
#include "assets/lang_config.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "display/lcd_display.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "button.h"
#include "dual_network_board.h"
#include "fbt_manager.h"
#include "lamp_controller.h"
#include "led/single_led.h"
#include "mcp_server.h"
#include "settings.h"
#include "system_reset.h"
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <wifi_station.h>

class CompactWifiBoard : public DualNetworkBoard {
  private:
    LcdDisplay *display_ = nullptr;
    Button boot_button_;
    Button touch_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.mosi_io_num = LCD_SDA_PIN;
        buscfg.sclk_io_num = LCD_SCL_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);

        spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    }

    void InitializeSt7735Display() {

        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = LCD_CS_PIN;
        io_config.dc_gpio_num = LCD_DC_PIN;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = LCD_RST_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);

        esp_lcd_panel_init(panel);

        esp_lcd_panel_invert_color(panel, false);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(panel_io, panel,
                                     DISPLAY_WIDTH,
                                     DISPLAY_HEIGHT,
                                     DISPLAY_OFFSET_X,
                                     DISPLAY_OFFSET_Y,
                                     DISPLAY_MIRROR_X,
                                     DISPLAY_MIRROR_Y,
                                     DISPLAY_SWAP_XY);
    }

    void InitializeButtons() {
        boot_button_.OnLongPress([this]() {
            auto &app = Application::GetInstance();
            app.SetDeviceState(kDeviceStateIdle);
            vTaskDelay(pdMS_TO_TICKS(300));
            esp_restart();
        });

        boot_button_.OnClick([this]() {
            auto &app = Application::GetInstance();
            auto &fbt_manager = FbtManager::GetInstance();
            app.ToggleChatState();
            fbt_manager.OnClickButton();
        });
        touch_button_.OnClick([this]() {
            auto &fbt_manager = FbtManager::GetInstance();
            fbt_manager.OnClickButton();
        });
        touch_button_.OnPressDown([this]() {
            auto &fbt_manager = FbtManager::GetInstance();
            if (fbt_manager.IsIntercomMode()) {
                fbt_manager.OnSpeaking(true);
            } else {
                auto &app = Application::GetInstance();
                app.StartListening();
            }
        });
        touch_button_.OnPressUp([this]() {
            auto &fbt_manager = FbtManager::GetInstance();
            if (fbt_manager.IsIntercomMode()) {
                fbt_manager.OnSpeaking(false);

            } else {
                auto &app = Application::GetInstance();
                app.StopListening();
            }
        });

        volume_up_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_up_button_.OnMultipleClick([this]() {
            Settings settings("network", true);
            int network_type = settings.GetInt("type");
            if (network_type == 0) {
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

        volume_up_button_.OnLongPress([this]() {
            SwitchNetworkType();
            /* GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification(Lang::Strings::MAX_VOLUME); */
        });

        volume_down_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_down_button_.OnLongPress([this]() {
            /* GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification(Lang::Strings::MUTED); */
            auto &app = Application::GetInstance();
            auto &fbt_manager = FbtManager::GetInstance();
            if (app.GetDeviceState() != kDeviceStateFbtActive) {
                if (fbt_manager.OnEnterIntercomMode()) {
                    app.SetDeviceState(kDeviceStateFbtActive);
                }
            } else {
                fbt_manager.OnExitVoiceMode();
                app.SetDeviceState(kDeviceStateIdle);
            }
        });
    }

  public:
    CompactWifiBoard() : DualNetworkBoard(ML307_TX_PIN, ML307_RX_PIN, GPIO_NUM_NC),
                         boot_button_(BOOT_BUTTON_GPIO, false, 1800, 300),
                         touch_button_(TOUCH_BUTTON_GPIO, false, 1800, 300),
                         volume_up_button_(VOLUME_UP_BUTTON_GPIO, false, 1800, 300),
                         volume_down_button_(VOLUME_DOWN_BUTTON_GPIO, false, 1800, 300) {
        InitializeSpi();
        InitializeSt7735Display();
        InitializeButtons();

        if (LCD_BL_PIN != GPIO_NUM_NC) {
            GetBacklight()->RestoreBrightness();
        }
    }

    virtual AudioCodec *GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            16000, 16000,
            GPIO_NUM_15,
            GPIO_NUM_16,
            GPIO_NUM_7,
            GPIO_NUM_5,
            GPIO_NUM_4,
            GPIO_NUM_6);
        return &audio_codec;
    }

    virtual Display *GetDisplay() override {
        return display_;
    }

    /* virtual Display *GetDisplay() override {
        return new NoDisplay();
    } */

    virtual Led *GetLed() override {
        static SingleLed led(GPIO_NUM_2);
        return &led;
    }

    virtual Backlight *GetBacklight() override {
        if (LCD_BL_PIN != GPIO_NUM_NC) {
            static PwmBacklight backlight(LCD_BL_PIN, false);
            return &backlight;
        }
        return nullptr;
    }
};

DECLARE_BOARD(CompactWifiBoard);