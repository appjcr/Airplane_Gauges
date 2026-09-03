#pragma once

#include <cstdint>

// ── Display Configuration (TFT) ───────────────────────
namespace Display {
    constexpr uint16_t WIDTH = 320;
    constexpr uint16_t HEIGHT = 480;
    constexpr uint16_t ROTATION = 1;
    constexpr uint32_t SPI_SPEED = 40000000UL;
}

// ── Display Backlight ─────────────────────────────────
namespace TFT {
    constexpr uint8_t BL_PIN = 1;
}

// ── SPI Pins ──────────────────────────────────────────
namespace SPII{
    constexpr uint8_t CS = 45;
    constexpr uint8_t SCK = 47;
    constexpr uint8_t SDA0 = 21;
    constexpr uint8_t SDA1 = 48;
    constexpr uint8_t SDA2 = 40;
    constexpr uint8_t SDA3 = 39;
}

// ── Touch Controller ──────────────────────────────────
namespace Touch {
    constexpr uint8_t SCL = 8;
    constexpr uint8_t SDA = 4;
    constexpr uint8_t INT = 3;
    constexpr uint8_t ADDR = 0x3B;
    constexpr uint16_t X_MIN = 12;
    constexpr uint16_t X_MAX = 310;
    constexpr uint16_t Y_MIN = 14;
    constexpr uint16_t Y_MAX = 461;
}

// ── ADS7830 ADC (I2C) ─────────────────────────────────
namespace ADC {
    constexpr uint8_t SDA = 17;
    constexpr uint8_t SCL = 18;
    constexpr uint32_t I2C_FREQ = 100000;
    constexpr uint8_t I2C_ADDR = 0x48;

    // Channel assignments
    constexpr uint8_t CH_FLAPS = 0;
    constexpr uint8_t CH_AILERON = 1;
    constexpr uint8_t CH_ELEVATOR = 2;

    // Flaps scaling: raw ADC FLAPS_LO..FLAPS_HI maps to 0..11 bar units on the
    // flaps gauge — not degrees. The scale's degree labels are decorative.
    constexpr int16_t FLAPS_LO = 30;
    constexpr int16_t FLAPS_HI = 242;
    constexpr float FLAPS_SCALE = 0.05188f;

    // Trim scaling (0-255 raw ADC to 0-100 percent)
    constexpr int16_t TRIM_LO = 0;
    constexpr int16_t TRIM_HI = 255;
    constexpr float TRIM_SCALE = 0.39215f;
}

// ── Fuel Tank Capacitance Sensors ─────────────────────
namespace FuelSensors {
    constexpr uint8_t PIN_LEFT = 5;
    constexpr uint8_t PIN_RIGHT = 6;
    constexpr uint32_t BAUD = 1200;

    // Bound on readBytesUntil(). Stream's 1000 ms default would let a single dropped
    // frame terminator block the sensor callback — and with it lv_timer_handler() —
    // for a full second. A 2-byte frame plus terminator is ~25 ms at 1200 baud.
    constexpr uint32_t READ_TIMEOUT_MS = 50;

    // Left tank calibration
    constexpr int16_t LEFT_CAP_FULL = 805;
    constexpr int16_t LEFT_CAP_EMPTY = 1590;
    constexpr int16_t LEFT_EMPTY_THRESH = 1475;

    // Right tank calibration
    constexpr int16_t RIGHT_CAP_FULL = 805;
    constexpr int16_t RIGHT_CAP_EMPTY = 1590;
    constexpr int16_t RIGHT_EMPTY_THRESH = 1320;

    // Rolling-average window
    constexpr int SMOOTH_BUFFER_SIZE = 50;

    // Sanity window for a decoded frame — deliberately far wider than the calibrated
    // span. This rejects nonsense from a mid-frame read; it is NOT a calibration
    // clamp. Readings inside this window but outside CAP_FULL..CAP_EMPTY are still
    // clamped to the rails as before.
    constexpr int32_t RAW_PLAUSIBLE_MIN = 400;
    constexpr int32_t RAW_PLAUSIBLE_MAX = 2000;

    // Usable gallons in one tank — the top of the setup roller, not a tank count.
    // The roller's options are generated from this, so the two can't drift.
    constexpr int MAX_TANK_GALLONS = 12;
}

// ── Fuel Flow Sensor ──────────────────────────────────
// Electronics International FT-60 (Red Cube). Open-collector output —
// requires a pull-up (sensor spec calls for ~1 mA pull-up current).
namespace FlowSensor {
    constexpr uint8_t PIN = 7;

    // K-factor is user-settable on the K-FAC screen and persisted to NVS, so it is
    // held at runtime in AppState rather than fixed here. Stored and displayed in
    // thousands of pulses per gallon: 68 means 68,000. The FT-60 ships at 68, but
    // installation plumbing shifts it, which is why it is adjustable.
    constexpr int DEFAULT_K_FACTOR_THOUSANDS = 68;
    constexpr int MIN_K_FACTOR_THOUSANDS = 10;
    constexpr int MAX_K_FACTOR_THOUSANDS = 100;
    // Damping window is SMOOTH_BUFFER_SIZE * Timers::FLOW_SENSOR_MS — 4 * 1000 ms = 4 s.
    // These two constants multiply, so changing the tick rate changes the damping.
    constexpr int SMOOTH_BUFFER_SIZE = 4;

    // Rate gate, as a Schmitt trigger. The reading must reach MIN_GPH to start being
    // displayed, then fall MIN_GPH_HYSTERESIS below that to stop. At K=68,000 and a
    // 1000 ms tick one pulse is worth ~0.053 GPH, so a bare threshold sits between two
    // adjacent pulse counts and the display flickers as the count alternates. The band
    // is about two pulses wide, which comfortably straddles that.
    constexpr float MIN_GPH = 0.2f;
    constexpr float MIN_GPH_HYSTERESIS = 0.1f;
    constexpr float MAX_GPH = 60.0f;
    constexpr uint32_t STALE_TIMEOUT_MS = 2500;
}

// ── Timer Periods (milliseconds) ──────────────────────
namespace Timers {
    constexpr uint16_t FUEL_SENSOR_MS = 2000;
    constexpr uint16_t TRIM_FLAP_SENSOR_MS = 500;
    // Pairs with FlowSensor::SMOOTH_BUFFER_SIZE to set the damping window.
    constexpr uint16_t FLOW_SENSOR_MS = 1000;

    constexpr uint16_t GAUGE_FUEL_MS = 200;
    constexpr uint16_t GAUGE_FLAPS_MS = 250;
    constexpr uint16_t GAUGE_TRIM_MS = 250;
    constexpr uint16_t GAUGE_FLOW_MS = 250;

    constexpr uint16_t STARTUP_ANIM_INTERVAL_MS = 200;
    constexpr uint16_t LVGL_HANDLER_PERIOD_MS = 5;

    // How often the flow totals are committed to NVS. Every flow tick would be
    // ~4500 flash writes/hour; at 30 s a power cut costs well under a tenth of a
    // gallon of totalizer accuracy.
    constexpr uint32_t FLOW_SAVE_INTERVAL_MS = 30000;

    // Flow debug line cadence. Kept independent of FLOW_SENSOR_MS so the sample rate
    // can be tuned without turning the serial log into a firehose.
    constexpr uint32_t FLOW_LOG_INTERVAL_MS = 1000;
}

// ── Startup Configuration ─────────────────────────────
namespace Startup {
    constexpr uint16_t SERIAL_WAIT_MS = 5000;
    constexpr uint16_t USB_CDC_WAIT_MS = 2000;
    constexpr uint32_t SERIAL_BAUD = 115200;
    constexpr uint16_t LVGL_BUFFER_DIVISOR = 10;
}

// ── Trim Gauge UI Constants ───────────────────────────
namespace TrimGauge {
    constexpr float POSITION_SCALE = 1.4f;
    constexpr int32_t CENTER_X = 55;
    constexpr int32_t CENTER_Y = 55;
}
