#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/adc.h>
#include <driver/gpio.h>

// Keep lichuang-dev audio/display baseline.
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_INPUT_REFERENCE    true

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_38
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_13
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_14
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_12
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_45

#define AUDIO_CODEC_USE_PCA9557
#define AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_1
#define AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_2
#define AUDIO_CODEC_ES8311_ADDR ES8311_CODEC_DEFAULT_ADDR
#define AUDIO_CODEC_ES7210_ADDR 0x82

#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY  true
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_42
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT true

/* Camera pins */
#define CAMERA_PIN_PWDN  GPIO_NUM_NC
#define CAMERA_PIN_RESET GPIO_NUM_NC
#define CAMERA_PIN_XCLK  GPIO_NUM_5
#define CAMERA_PIN_SIOD  GPIO_NUM_1
#define CAMERA_PIN_SIOC  GPIO_NUM_2

#define CAMERA_PIN_D7    GPIO_NUM_9
#define CAMERA_PIN_D6    GPIO_NUM_4
#define CAMERA_PIN_D5    GPIO_NUM_6
#define CAMERA_PIN_D4    GPIO_NUM_15
#define CAMERA_PIN_D3    GPIO_NUM_17
#define CAMERA_PIN_D2    GPIO_NUM_8
#define CAMERA_PIN_D1    GPIO_NUM_18
#define CAMERA_PIN_D0    GPIO_NUM_16
#define CAMERA_PIN_VSYNC GPIO_NUM_3
#define CAMERA_PIN_HREF  GPIO_NUM_46
#define CAMERA_PIN_PCLK  GPIO_NUM_7

#define XCLK_FREQ_HZ 20000000

// PCA9685 on board I2C expansion header.
#define SERVO_PCA9685_I2C_ADDR 0x40
#define SERVO_PCA9685_FREQ_HZ  50

struct HardwareConfig {
    gpio_num_t power_charge_detect_pin;
    adc_unit_t power_adc_unit;
    adc_channel_t power_adc_channel;

    // For PCA9685 backend, these fields store channel index (0..15) instead of GPIO.
    gpio_num_t right_leg_pin;
    gpio_num_t right_foot_pin;
    gpio_num_t left_leg_pin;
    gpio_num_t left_foot_pin;
    gpio_num_t left_hand_pin;
    gpio_num_t right_hand_pin;
};

constexpr HardwareConfig LCS3_OTTO_CONFIG = {
    .power_charge_detect_pin = GPIO_NUM_NC,
    .power_adc_unit = ADC_UNIT_2,
    .power_adc_channel = ADC_CHANNEL_3,

    .right_leg_pin = static_cast<gpio_num_t>(0),
    .right_foot_pin = static_cast<gpio_num_t>(1),
    .left_leg_pin = static_cast<gpio_num_t>(2),
    .left_foot_pin = static_cast<gpio_num_t>(3),
    .left_hand_pin = static_cast<gpio_num_t>(4),
    .right_hand_pin = static_cast<gpio_num_t>(5),
};

#endif // _BOARD_CONFIG_H_
