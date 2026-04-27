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

    // Flaps scaling (0-255 raw ADC to 0-20 degrees)
    constexpr int16_t FLAPS_LO = 0;
    constexpr int16_t FLAPS_HI = 255;
    constexpr float FLAPS_SCALE = 0.784f;

    // Trim scaling (0-255 raw ADC to 0-100 percent)
    constexpr int16_t TRIM_LO = 0;
    constexpr int16_t TRIM_HI = 255;
    constexpr float TRIM_SCALE = 0.392f;
}

// ── Fuel Tank Capacitance Sensors ─────────────────────
namespace FuelSensors {
    constexpr uint8_t PIN_LEFT = 5;
    constexpr uint8_t PIN_RIGHT = 6;
    constexpr uint32_t BAUD = 1200;

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

    // Maximum fuel tank setting (12 tanks)
    constexpr int MAX_FUEL_TANKS = 12;
}

// ── Fuel Flow Sensor ──────────────────────────────────
namespace FlowSensor {
    constexpr uint8_t PIN = 7;
    constexpr float PULSES_PER_LITER = 4380.0f;
    constexpr float LITERS_PER_GALLON = 3.78541f;
}

// ── Timer Periods (milliseconds) ──────────────────────
namespace Timers {
    constexpr uint16_t FUEL_SENSOR_MS = 100;
    constexpr uint16_t TRIM_FLAP_SENSOR_MS = 100;
    constexpr uint16_t FLOW_SENSOR_MS = 1000;

    constexpr uint16_t GAUGE_FUEL_MS = 200;
    constexpr uint16_t GAUGE_FLAPS_MS = 250;
    constexpr uint16_t GAUGE_TRIM_MS = 250;
    constexpr uint16_t GAUGE_FLOW_MS = 250;

    constexpr uint16_t STARTUP_ANIM_INTERVAL_MS = 200;
    constexpr uint16_t LVGL_HANDLER_PERIOD_MS = 5;
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
