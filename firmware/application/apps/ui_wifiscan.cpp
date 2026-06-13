/*
 * PortaPack H4M – WiFi Scan Viewer
 * Implementace
 */

#include "ui_wifiscan.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui {

// ---------------------------------------------------------------
// Konstruktor
// ---------------------------------------------------------------
WifiScanView::WifiScanView(NavigationView& nav)
    : nav_{ nav }
{
    add_children({
        &labels_,
        &text_status_,
        &button_rescan_,
    });

    // Inicializuj řádky seznamu
    for (size_t i = 0; i < WIFI_MAX_ENTRIES; i++) {
        entry_text_[i].set_parent_rect({
            1,
            static_cast<int>(32 + i * 16),
            30 * 8,
            16
        });
        entry_text_[i].set("");
        add_child(&entry_text_[i]);
    }

    button_rescan_.on_select = [this](Button&) {
        wifi_list_.clear();
        render_list();
        text_status_.set("Rescanuji...");
        tick_ = 0;
        fetch_from_esp32();
    };

    // První načtení hned při spuštění
    fetch_from_esp32();
}

WifiScanView::~WifiScanView() = default;

void WifiScanView::focus() {
    button_rescan_.focus();
}

// ---------------------------------------------------------------
// Voláno každý frame sync (~každých 500 ms → 10 tiků = 5 sekund)
// ---------------------------------------------------------------
void WifiScanView::on_tick() {
    tick_++;
    // Obnov každých ~10 tiků (cca 5 s)
    if (tick_ % 10 == 0) {
        fetch_from_esp32();
    }
}

// ---------------------------------------------------------------
// Čte záznamy z ESP32 přes I2C
// Formát: byte[0] = channel (0 = konec), byte[1..31] = SSID
// ---------------------------------------------------------------
void WifiScanView::fetch_from_esp32() {
    std::vector<WifiEntry> new_list;

    for (size_t i = 0; i < WIFI_MAX_ENTRIES + 1; i++) {
        uint8_t buf[RECORD_SIZE] = { 0 };

        const bool ok = i2c0.receive(
            ESP32_I2C_ADDR,
            buf,
            RECORD_SIZE,
            10  // timeout ms
        );

        if (!ok) {
            text_status_.set("I2C chyba! Zkontroluj draty.");
            return;
        }

        uint8_t ch = buf[0];
        if (ch == 0) break;  // Konec seznamu

        char ssid_buf[31] = { 0 };
        memcpy(ssid_buf, buf + 1, 30);

        // Detekce smyčky (ESP32 točí záznamy dokola)
        bool dup = false;
        for (auto& e : new_list) {
            if (e.channel == ch && e.ssid == ssid_buf) {
                dup = true;
                break;
            }
        }
        if (dup) break;

        WifiEntry entry;
        entry.channel = ch;
        entry.ssid    = std::string(ssid_buf);
        new_list.push_back(entry);

        if (new_list.size() >= WIFI_MAX_ENTRIES) break;
    }

    wifi_list_ = std::move(new_list);

    std::string status = "Siti: " + to_string_dec_uint(wifi_list_.size());
    text_status_.set(status);

    render_list();
}

// ---------------------------------------------------------------
// Vykreslí seznam do textových widgetů
// Formát řádku: "CH 6  MojeWifi"
// ---------------------------------------------------------------
void WifiScanView::render_list() {
    for (size_t i = 0; i < WIFI_MAX_ENTRIES; i++) {
        if (i < wifi_list_.size()) {
            const auto& e = wifi_list_[i];

            std::string line = "CH";
            if (e.channel < 10) line += " ";
            line += to_string_dec_uint(e.channel);
            line += "  ";

            std::string ssid = e.ssid;
            if (ssid.size() > 24) ssid = ssid.substr(0, 24);
            line += ssid;

            while (line.size() < 30) line += " ";

            entry_text_[i].set(line);
        } else {
            entry_text_[i].set("");
        }
    }
}

}  // namespace ui
