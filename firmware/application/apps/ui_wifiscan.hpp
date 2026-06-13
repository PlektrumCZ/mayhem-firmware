#pragma once

/*
 * PortaPack H4M – WiFi Scan Viewer
 * Přijímá data z ESP32 přes I2C (adresa 0x42)
 * Zobrazuje SSID + channel skenovaných WiFi sítí
 *
 * Zapojení PortaPack -> ESP32:
 *   Pin 22 (SCL) -> ESP32 Pin 22 (SCL)
 *   Pin 23 (SDA) -> ESP32 Pin 23 (SDA)
 *   GND          -> ESP32 GND
 *   3.3V         -> ESP32 3.3V
 */

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "i2cdev.hpp"
#include "string_format.hpp"
#include "message.hpp"

namespace ui {

static constexpr size_t  WIFI_MAX_ENTRIES = 16;
static constexpr uint8_t ESP32_I2C_ADDR  = 0x42;
static constexpr size_t  RECORD_SIZE     = 32;  // 1B channel + 31B SSID

struct WifiEntry {
    uint8_t     channel;
    std::string ssid;
};

class WifiScanView : public View {
public:
    explicit WifiScanView(NavigationView& nav);
    ~WifiScanView() override;

    void focus() override;
    std::string title() const override { return "WiFi Scan"; }

private:
    NavigationView& nav_;

    // Widgets
    Labels labels_{
        { { 1, 4 }, "WiFi Scan  ESP32->PortaPack", Color::white() },
    };

    Text text_status_{
        { 1, 16, 30 * 8, 16 },
        "Cekam na ESP32..."
    };

    // 16 řádků pro sítě
    Text entry_text_[WIFI_MAX_ENTRIES];

    Button button_rescan_{
        { 80, 290, 80, 28 },
        "Rescan"
    };

    // Data
    std::vector<WifiEntry> wifi_list_{};
    uint32_t tick_{ 0 };

    // Timer registration
    MessageHandlerRegistration msg_handler_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) { on_tick(); }
    };

    void on_tick();
    void fetch_from_esp32();
    void render_list();
};

}  // namespace ui
