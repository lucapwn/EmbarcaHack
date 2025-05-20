#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"

#include "include/ws2818b.pio.h"
#include "include/ssd1306.h"
#include "include/auth.h"

#define MIC_PIN      28
#define VRX_PIN      27
#define VRY_PIN      26
#define SW_PIN       22
#define BUZZER_PIN   21
#define I2C_SCL_PIN  15
#define I2C_SDA_PIN  14
#define MATRIZ_PIN    7
#define BTN_B_PIN     6
#define BTN_A_PIN     5

#define MATRIX_COLUMNS 5
#define MATRIX_ROWS    5
#define LED_COUNT      (MATRIX_ROWS * MATRIX_COLUMNS)

#define MIC_THRESHOLD      2180
#define MIC_NOISE_MARGIN   50
#define DOUBLE_CLAP_TIME   500
#define MIN_CLAP_INTERVAL  100

#define LOWER_JOYSTICK_LIMIT 1024
#define UPPER_JOYSTICK_LIMIT 3072

#define JOYSTICK_AUTH 0
#define SOUND_AUTH    1

#define MAX_PASSWORD_LENGTH 4

typedef struct {
    uint8_t G;
    uint8_t R;
    uint8_t B;
} npLED_t;

uint32_t auth_type;
uint32_t user_password[MAX_PASSWORD_LENGTH];
uint32_t masked_password[MAX_PASSWORD_LENGTH];

/*
    O sistema é composto por uma matriz de LEDs 5x5, organizada da seguinte forma:

    40 41 42 43 44
    30 31 32 33 34
    20 21 22 23 24
    10 11 12 13 14
    00 01 02 03 04

    Abaixo está uma representação da senha do sistema:

    40 -- 42 -- 44
    -- -- 32 -- --
    -- -- -- -- --
    -- -- -- -- --
    -- -- -- -- --

    A senha deve possuir a seguinte ordem:

    1 - 2 - 3
    - - 4 - -
    - - - - -
    - - - - -
    - - - - -
*/

const uint32_t system_password[MAX_PASSWORD_LENGTH] = { 24, 22, 20, 17 };

const uint led_matrix[MATRIX_ROWS][MATRIX_COLUMNS] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    { 5,  6,  7,  8,  9},
    { 4,  3,  2,  1,  0}
};

static const uint32_t central_square[] = { 6, 7, 8, 11, 13, 16, 17, 18 };
static const uint32_t outer_square[] = { 0, 1, 2, 3, 4, 5, 9, 10, 14, 15, 19, 20, 21, 22, 23, 24 };

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

struct render_area frame_area = {
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1
};

uint8_t ssd[ssd1306_buffer_length];

void init_hardware(void) {
    stdio_init_all();
    adc_init();

    adc_gpio_init(VRY_PIN);
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(MIC_PIN);

    gpio_init(SW_PIN);
    gpio_init(BTN_A_PIN);
    gpio_init(BTN_B_PIN);
    gpio_init(BUZZER_PIN);
    
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    
    gpio_pull_up(SW_PIN);
    gpio_pull_up(BTN_A_PIN);
    gpio_pull_up(BTN_B_PIN);

    pwm_init_buzzer(BUZZER_PIN);

    init_led_matrix();
    led_reset();
    write_led_matrix();

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
}

void init_led_matrix(void) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    ws2818b_program_init(np_pio, sm, offset, MATRIZ_PIN, 800000.0f);
}

void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();

    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

void set_led_color(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void led_reset(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        set_led_color(i, 0, 0, 0);
    }
}

void write_led_matrix(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }

    sleep_us(100);
}

void pwm_set_frequency(uint pin, uint frequency) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t top = 4095;

    float divider = (float)clock / (frequency * (top + 1));
    
    divider = (divider < 1.0f) ? 1.0f : divider;
    divider = (divider > 255.0f) ? 255.0f : divider;

    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, top);
}

void beep(uint frequency, uint duration_ms) {
    pwm_set_frequency(BUZZER_PIN, frequency);
    pwm_set_gpio_level(BUZZER_PIN, 2048);
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    sleep_ms(100);
}

void display_reset(void) {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

uint32_t matrix_to_display_value(int line, int column) {
    return (40 + column - (line * 10));
}

void draw_password_line(uint32_t *password, uint length) {
    char buffer[20];
    memset(buffer, 0, sizeof(buffer));

    for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
        if (i < length) {
            sprintf(buffer + strlen(buffer), "%02d ", password[i]);
        } else {
            strcat(buffer, "-- ");
        }
    }

    const char *auth_text = (auth_type == JOYSTICK_AUTH) ? "Joystick" : "Sound";

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 8, 16, (char*)auth_text);
    ssd1306_draw_string(ssd, 8, 40, buffer);
    render_on_display(ssd, &frame_area);
}

bool is_password_correct(void) {
    for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
        if (user_password[i] != system_password[i]) {
            return false;
        }
    }

    return true;
}

void selection_animation(uint matrix_index) {
    for (int i = 0; i < 2; i++) {
        led_reset();
        set_led_color(matrix_index, 0, 0, 4);
        write_led_matrix();

        if (!i) {
            beep(1320, 35);
            sleep_ms(65);
        } else {
            sleep_ms(100);
        }

        led_reset();
        write_led_matrix();
        sleep_ms(100);
    }

    led_reset();
}

void access_animation(bool allowed) {
    char buffer[20];
    memset(buffer, 0, sizeof(buffer));

    for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
        sprintf(buffer + strlen(buffer), "%02d ", masked_password[i]);
    }

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 8, 16, allowed ? "Access Allowed" : "Access Denied");
    ssd1306_draw_string(ssd, 8, 40, buffer);
    sleep_ms(80);
    render_on_display(ssd, &frame_area);

    uint8_t r = allowed ? 0 : 4;
    uint8_t g = allowed ? 4 : 0;
    uint8_t b = 0;

    uint16_t freq1 = allowed ? 880 : 988;
    uint16_t freq2 = allowed ? 1175 : 784;
    uint16_t freq3 = allowed ? 1567 : 0;
    
    for (int i = 0; i < 2; i++) {
        led_reset();

        for (int j = 0; j < 16; j++) {
            set_led_color(outer_square[j], r, g, b);
        }

        write_led_matrix();

        if (!i) {
            beep(freq1, 50);
            sleep_ms(40);
        } else {
            sleep_ms(90);
        }

        led_reset();

        for (int j = 0; j < 8; j++) {
            set_led_color(central_square[j], r, g, b);
        }

        write_led_matrix();
        
        if (!i) {
            beep(freq2, 50);
            sleep_ms(40);
        } else {
            sleep_ms(90);
        }

        led_reset();
        set_led_color(12, r, g, b);
        write_led_matrix();
        
        if (!i && allowed) {
            beep(freq3, 80);
        } else {
            sleep_ms(80);
        }

        led_reset();

        for (int j = 0; j < 8; j++) {
            set_led_color(central_square[j], r, g, b);
        }

        write_led_matrix();
        sleep_ms(100);
        led_reset();

        for (int j = 0; j < 16; j++) {
            set_led_color(outer_square[j], r, g, b);
        }

        write_led_matrix();
        sleep_ms(340);
    }

    led_reset();
}

uint32_t get_auth_type(void) {
    memset(ssd, 0, ssd1306_buffer_length);

    ssd1306_draw_string(ssd, 8, 16, "Select");
    ssd1306_draw_string(ssd, 8, 32, "A - Joystick");
    ssd1306_draw_string(ssd, 8, 40, "B - Sound");

    render_on_display(ssd, &frame_area);

    while (true) {
        bool button_a_pressed = !gpio_get(BTN_A_PIN);
        bool button_b_pressed = !gpio_get(BTN_B_PIN);

        if (button_a_pressed) {
            beep(1320, 35);
            return JOYSTICK_AUTH;
        }

        if (button_b_pressed) {
            beep(1320, 35);
            return SOUND_AUTH;
        }

        sleep_ms(50);
    }
}

void start_joystick_auth(void) {
    uint line = 0;
    uint column = 0;
    uint matrix_index = 0;
    uint password_length = 0;

    memset(user_password, 0, sizeof(user_password));
    memset(masked_password, 0, sizeof(masked_password));
    draw_password_line(user_password, password_length);

    while (password_length < MAX_PASSWORD_LENGTH) {
        bool sw_button_pressed = !gpio_get(SW_PIN);

        adc_select_input(0);
        uint16_t vry_joystick = adc_read();

        adc_select_input(1);
        uint16_t vrx_joystick = adc_read();

        static absolute_time_t last_move = { 0 };
        int64_t time_difference = absolute_time_diff_us(last_move, get_absolute_time());

        if (time_difference > 200000) {
            if (vry_joystick < LOWER_JOYSTICK_LIMIT && line < 4) {
                line++;
            } else if (vry_joystick > UPPER_JOYSTICK_LIMIT && line > 0) {
                line--;
            }

            if (vrx_joystick < LOWER_JOYSTICK_LIMIT && column > 0) {
                column--;
            } else if (vrx_joystick > UPPER_JOYSTICK_LIMIT && column < 4) {
                column++;
            }

            last_move = get_absolute_time();
            matrix_index = led_matrix[line][column];

            if (sw_button_pressed) {
                user_password[password_length] = matrix_index;
                masked_password[password_length] = matrix_to_display_value(line, column);
                password_length++;

                selection_animation(matrix_index);
                draw_password_line(masked_password, password_length);
            }
        }

        led_reset();
        set_led_color(matrix_index, 0, 0, 4);
        write_led_matrix();
        sleep_ms(10);
    }

    access_animation(is_password_correct());
    led_reset();
    write_led_matrix();
}

void start_sound_auth(void) {
    uint line = 0;
    uint column = 0;
    uint password_length = 0;
    uint matrix_index = led_matrix[line][column];

    bool waiting_clap = false;
    bool waiting_fall = false;

    absolute_time_t last_peak_time = {0};
    absolute_time_t clap_start_time = {0};

    memset(user_password, 0, sizeof(user_password));
    memset(masked_password, 0, sizeof(masked_password));
    draw_password_line(user_password, password_length);

    led_reset();
    set_led_color(matrix_index, 0, 0, 4);
    write_led_matrix();

    adc_select_input(2);

    while (password_length < MAX_PASSWORD_LENGTH) {
        uint16_t mic_value = adc_read();
        absolute_time_t now = get_absolute_time();

        if (mic_value > MIC_THRESHOLD && !waiting_fall) {
            int64_t since_last = absolute_time_diff_us(last_peak_time, now) / 1000;
            
            if (since_last > MIN_CLAP_INTERVAL) {
                last_peak_time = now;
                waiting_fall = true;

                if (!waiting_clap) {
                    waiting_clap = true;
                    clap_start_time = now;
                } else {
                    int64_t time_difference = absolute_time_diff_us(clap_start_time, now) / 1000;

                    if (time_difference < DOUBLE_CLAP_TIME) {
                        matrix_index = led_matrix[line][column];
                        user_password[password_length] = matrix_index;
                        masked_password[password_length] = matrix_to_display_value(line, column);
                        password_length++;

                        selection_animation(matrix_index);
                        draw_password_line(masked_password, password_length);

                        clap_start_time = get_absolute_time();
                        waiting_clap = false;

                        set_led_color(matrix_index, 0, 0, 4);
                        write_led_matrix();
                    }
                }
            }
        }

        if (waiting_fall && mic_value < (MIC_THRESHOLD - MIC_NOISE_MARGIN)) {
            waiting_fall = false;
        }

        int64_t time_difference = absolute_time_diff_us(clap_start_time, now);

        if (waiting_clap && time_difference > DOUBLE_CLAP_TIME * 1000) {
            waiting_clap = false;
            column++;
            
            if (column >= MATRIX_COLUMNS) {
                column = 0;
                line++;

                if (line >= MATRIX_ROWS) {
                    line = 0;
                }
            }

            led_reset();
            matrix_index = led_matrix[line][column];
            set_led_color(matrix_index, 0, 0, 4);
            write_led_matrix();
        }

        sleep_ms(10);
    }

    access_animation(is_password_correct());
    led_reset();
    write_led_matrix();
}

int main(int argc, char *argv[]) {
    init_hardware();

    while (true) {
        auth_type = get_auth_type();

        switch (auth_type) {
            case JOYSTICK_AUTH:
                start_joystick_auth();
                break;

            case SOUND_AUTH:
                start_sound_auth();
                break;
            
            default:
                break;
        }
    }

    return 0;
}
