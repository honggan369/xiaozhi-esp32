//--------------------------------------------------------------
//-- Oscillator for PCA9685 backend
//--------------------------------------------------------------
#include "oscillator.h"

#include <esp_timer.h>

#include <algorithm>
#include <cmath>

static const char* TAG = "OscillatorPCA9685";

extern unsigned long IRAM_ATTR millis();

namespace {
constexpr uint8_t kRegMode1 = 0x00;
constexpr uint8_t kRegMode2 = 0x01;
constexpr uint8_t kRegPrescale = 0xFE;
constexpr uint8_t kRegLed0OnL = 0x06;

constexpr uint8_t kMode1Sleep = 0x10;
constexpr uint8_t kMode1Ai = 0x20;
constexpr uint8_t kMode1Restart = 0x80;
constexpr uint8_t kMode2Outdrv = 0x04;

constexpr uint32_t kPcaInternalClockHz = 25000000;
constexpr uint16_t kPcaResolution = 4096;

class Pca9685Device : public I2cDevice {
public:
    Pca9685Device(i2c_master_bus_handle_t i2c_bus, uint8_t addr)
        : I2cDevice(i2c_bus, addr) {}

    void Initialize(uint16_t pwm_hz) {
        WriteReg(kRegMode1, kMode1Sleep);
        WriteReg(kRegMode2, kMode2Outdrv);

        uint8_t prescale = static_cast<uint8_t>(
            std::clamp<int>((kPcaInternalClockHz / (kPcaResolution * pwm_hz)) - 1, 3, 255));
        WriteReg(kRegPrescale, prescale);

        WriteReg(kRegMode1, kMode1Ai);
        vTaskDelay(pdMS_TO_TICKS(1));
        WriteReg(kRegMode1, kMode1Ai | kMode1Restart);
    }

    void SetPulseUs(uint8_t channel, uint16_t pulse_us) {
        if (channel > 15) {
            return;
        }
        const uint16_t ticks = static_cast<uint16_t>(
            std::clamp<int>((pulse_us * kPcaResolution) / SERVO_TIMEBASE_PERIOD, 0, kPcaResolution - 1));

        const uint8_t base = static_cast<uint8_t>(kRegLed0OnL + channel * 4);
        WriteReg(base + 0, 0x00);                     // ON_L
        WriteReg(base + 1, 0x00);                     // ON_H
        WriteReg(base + 2, static_cast<uint8_t>(ticks & 0xFF));        // OFF_L
        WriteReg(base + 3, static_cast<uint8_t>((ticks >> 8) & 0x0F)); // OFF_H
    }
};

Pca9685Device* g_pca9685 = nullptr;
bool g_pca9685_ready = false;
} // namespace

void InitializeOttoPca9685(i2c_master_bus_handle_t i2c_bus, uint8_t i2c_addr, uint16_t pwm_hz) {
    if (g_pca9685_ready) {
        return;
    }
    g_pca9685 = new Pca9685Device(i2c_bus, i2c_addr);
    g_pca9685->Initialize(pwm_hz);
    g_pca9685_ready = true;
    ESP_LOGI(TAG, "PCA9685 initialized addr=0x%02x freq=%uHz", i2c_addr, pwm_hz);
}

Oscillator::Oscillator(int trim) {
    trim_ = trim;
    diff_limit_ = 0;
    is_attached_ = false;

    sampling_period_ = 30;
    period_ = 2000;
    number_samples_ = period_ / sampling_period_;
    inc_ = 2 * M_PI / number_samples_;

    amplitude_ = 45;
    phase_ = 0;
    phase0_ = 0;
    offset_ = 0;
    stop_ = false;
    rev_ = false;

    pos_ = 90;
    previous_millis_ = 0;
}

Oscillator::~Oscillator() {
    Detach();
}

uint32_t Oscillator::AngleToCompare(int angle) {
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) /
               (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) +
           SERVO_MIN_PULSEWIDTH_US;
}

bool Oscillator::NextSample() {
    current_millis_ = millis();

    if (current_millis_ - previous_millis_ > sampling_period_) {
        previous_millis_ = current_millis_;
        return true;
    }

    return false;
}

void Oscillator::Attach(int pin, bool rev) {
    if (is_attached_) {
        Detach();
    }

    if (!g_pca9685_ready || g_pca9685 == nullptr) {
        ESP_LOGE(TAG, "PCA9685 is not initialized before attaching channel=%d", pin);
        return;
    }

    pin_ = pin; // for this backend, pin_ stores PCA9685 channel index.
    rev_ = rev;
    previous_servo_command_millis_ = millis();
    is_attached_ = true;
}

void Oscillator::Detach() {
    if (!is_attached_) {
        return;
    }
    is_attached_ = false;
}

void Oscillator::SetT(unsigned int T) {
    period_ = T;

    number_samples_ = period_ / sampling_period_;
    inc_ = 2 * M_PI / number_samples_;
}

void Oscillator::SetPosition(int position) {
    Write(position);
}

void Oscillator::Refresh() {
    if (NextSample()) {
        if (!stop_) {
            int pos = std::round(amplitude_ * std::sin(phase_ + phase0_) + offset_);
            if (rev_) {
                pos = -pos;
            }
            Write(pos + 90);
        }

        phase_ = phase_ + inc_;
    }
}

void Oscillator::Write(int position) {
    if (!is_attached_ || g_pca9685 == nullptr) {
        return;
    }

    long currentMillis = millis();
    if (diff_limit_ > 0) {
        int limit = std::max(1, (((int)(currentMillis - previous_servo_command_millis_)) * diff_limit_) / 1000);
        if (abs(position - pos_) > limit) {
            pos_ += position < pos_ ? -limit : limit;
        } else {
            pos_ = position;
        }
    } else {
        pos_ = position;
    }
    previous_servo_command_millis_ = currentMillis;

    int angle = pos_ + trim_;
    angle = std::min(std::max(angle, 0), 180);

    uint16_t pulse_us = static_cast<uint16_t>(SERVO_MIN_PULSEWIDTH_US +
        ((SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) * angle) / 180);
    g_pca9685->SetPulseUs(static_cast<uint8_t>(pin_), pulse_us);
}
