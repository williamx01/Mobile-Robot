#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/pulse_cnt.h"
#include "bdc_motor.h"
#include "pid_ctrl.h"

#include "include/mobile_robot_pins.h"

static const char *TAG_MOTOR = "Motor_Control";

// Motor Configuration
#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000 // 10MHz, 1 tick = 0.1us
#define BDC_MCPWM_FREQ_HZ             10000    // 25KHz PWM
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ) // maximum value we can set for the duty cycle, in ticks

// Motor Handles
bdc_motor_handle_t left_motor = NULL;
bdc_motor_handle_t right_motor = NULL;

// Encoder Handles
pcnt_unit_handle_t pcnt_unit_left = NULL;
pcnt_unit_handle_t pcnt_unit_right = NULL;

// PID Controller Handles
pid_ctrl_block_handle_t left_pid_ctrl = NULL;
pid_ctrl_block_handle_t right_pid_ctrl = NULL;

// Encoder Configuration
#define MOTOR_PCNT_HIGH_LIMIT 10000
#define MOTOR_PCNT_LOW_LIMIT  -10000

// PID Configuration
#define BDC_PID_LOOP_PERIOD_MS        10   // calculate the motor speed every 10ms
#define BDC_PID_EXPECT_SPEED          20  // expected motor speed, in the pulses counted by the rotary encoder

typedef struct {
    bdc_motor_handle_t motor;
    pcnt_unit_handle_t pcnt_encoder;
    pid_ctrl_block_handle_t pid_ctrl;
    int report_pulses;
} motor_control_context_t;

void motor_setup()
{
    // Create Right Motor Object
    bdc_motor_config_t motor_config_right = {
        .pwm_freq_hz = BDC_MCPWM_FREQ_HZ,
        .pwma_gpio_num = MOTOR_R_A,
        .pwmb_gpio_num = MOTOR_R_B,
    };

    // Create Left Motor Object
    bdc_motor_config_t motor_config_left = {
        .pwm_freq_hz = BDC_MCPWM_FREQ_HZ,
        .pwma_gpio_num = MOTOR_L_B, // NOTE: A and B Phases are flipped intentionally! This means that the motor will rotate forward in the context of the robot
        .pwmb_gpio_num = MOTOR_L_A,
    };

    // Configure mcpwm
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = 0,
        .resolution_hz = BDC_MCPWM_TIMER_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config_left, &mcpwm_config, &left_motor));
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config_right, &mcpwm_config, &right_motor));
    ESP_LOGI(TAG_MOTOR, "Left and Right Motors setup successful");

    ESP_ERROR_CHECK(bdc_motor_enable(left_motor));
    ESP_ERROR_CHECK(bdc_motor_enable(right_motor));
    ESP_LOGI(TAG_MOTOR, "Motors Enabled Successfully");
}

void encoder_setup()
{
    // Sets the Pulse Counter config
    pcnt_unit_config_t unit_config = {
        .high_limit = MOTOR_PCNT_HIGH_LIMIT,
        .low_limit = MOTOR_PCNT_LOW_LIMIT,
        .flags.accum_count = 1,
    };

    // Creates new pulse counters for the left and right
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit_left));
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit_right));

    // Setup left channel (NOTE: A and B are fipped intentionally to account for the way the motor is connected on the robot!)
    pcnt_chan_config_t chan_a_left_config = {
        .edge_gpio_num = ENC_L_B,
        .level_gpio_num = ENC_L_A,
    };

    pcnt_channel_handle_t pcnt_chan_a_left = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit_left, &chan_a_left_config, &pcnt_chan_a_left));
    pcnt_chan_config_t chan_b_left_config = {
        .edge_gpio_num = ENC_L_A,
        .level_gpio_num = ENC_L_B,
    };

    pcnt_channel_handle_t pcnt_chan_b_left = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit_left, &chan_b_left_config, &pcnt_chan_b_left));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a_left, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a_left, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b_left, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b_left, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    // Setup right channel
    pcnt_chan_config_t chan_a_right_config = {
        .edge_gpio_num = ENC_R_A,
        .level_gpio_num = ENC_R_B,
    };

    pcnt_channel_handle_t pcnt_chan_a_right = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit_right, &chan_a_right_config, &pcnt_chan_a_right));
    pcnt_chan_config_t chan_b_right_config = {
        .edge_gpio_num = ENC_R_B,
        .level_gpio_num = ENC_R_A,
    };

    pcnt_channel_handle_t pcnt_chan_b_right = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit_right, &chan_b_right_config, &pcnt_chan_b_right));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a_right, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a_right, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b_right, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b_right, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    // Enable Counters
    // Left
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit_left));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit_left));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit_left));

    // Right
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit_right));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit_right));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit_right));
}

void vMotor_Routine()
{
    motor_setup();
    bdc_motor_set_speed(left_motor, 398);
    bdc_motor_set_speed(right_motor, 398);
    while (1)
    {
        ESP_LOGI(TAG_MOTOR, "Motors forward");
        bdc_motor_forward(left_motor);
        bdc_motor_forward(right_motor);

        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG_MOTOR, "Motors coasting");
        bdc_motor_coast(left_motor);
        bdc_motor_coast(right_motor);

        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG_MOTOR, "Motors reverse");
        bdc_motor_reverse(left_motor);
        bdc_motor_reverse(right_motor);

        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG_MOTOR, "Motors braking");
        bdc_motor_brake(left_motor);
        bdc_motor_brake(right_motor);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
}

void vMotor_Ramp()
{
    motor_setup();
    ESP_LOGI(TAG_MOTOR, "Motors forward");
    bdc_motor_forward(left_motor);
    bdc_motor_forward(right_motor);
    int count = BDC_MCPWM_DUTY_TICK_MAX - 1;
    while (1)
    {
        for (size_t i = 0; i < count; i++)
        {
            bdc_motor_set_speed(left_motor,i);
            bdc_motor_set_speed(right_motor,i);
            ESP_LOGI(TAG_MOTOR, "Speed = %i", i);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}