#pragma once

// ── ADS7830 ADC (I2C) ────────────────────────────────────
#define ADS_SDA          17
#define ADS_SCL          18
#define ADS_I2C_FREQ     100000
#define ADS_I2C_ADDR     0x48

// ADC channel assignments
#define ADC_CH_FLAPS     0
#define ADC_CH_AILER     1
#define ADC_CH_ELEV      2

// Flaps ADC range and output scale (maps 0-255 raw to 0-20 degrees)
#define FLAPS_ADC_LO     0
#define FLAPS_ADC_HI     255
#define FLAPS_ADC_SCALE  0.784f

// Trim ADC range and output scale (maps 0-255 raw to 0-100 percent)
#define TRIM_ADC_LO      0
#define TRIM_ADC_HI      255
#define TRIM_ADC_SCALE   0.392f

// ── Fuel tank capacitance sensors (SoftwareSerial) ───────
#define FUEL_SENSOR_PIN_L    5
#define FUEL_SENSOR_PIN_R    6
#define FUEL_SENSOR_BAUD     1200

// Left tank capacitance calibration
#define FUEL_L_CAP_FULL      805
#define FUEL_L_CAP_EMPTY     1590
#define FUEL_L_EMPTY_THRESH  1475

// Right tank capacitance calibration
#define FUEL_R_CAP_FULL      805
#define FUEL_R_CAP_EMPTY     1590
#define FUEL_R_EMPTY_THRESH  1320

// Rolling-average window for capacitance readings
#define SMOOTH_BUF_SIZE      50

// ── Fuel flow sensor (pulse interrupt) ───────────────────
#define FLOW_SENSOR_PIN      7
#define FLOW_PULSES_PER_L    4380.0f
#define FLOW_L_PER_GALLON    3.78541f

// ── Timer periods (ms) ───────────────────────────────────
#define TIMER_FUEL_SENSOR_MS     100
#define TIMER_TRIM_FLAP_MS       100
#define TIMER_FLOW_SENSOR_MS     1000

#define TIMER_GAUGE_FUEL_MS      200
#define TIMER_GAUGE_FLAPS_MS     250
#define TIMER_GAUGE_TRIM_MS      250
#define TIMER_GAUGE_FLOW_MS      250
#define STARTUP_ANIM_INTERVAL    200

// ── Fuel quantity setup UI ───────────────────────────────
#define MAX_FUEL             12
