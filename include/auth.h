#ifndef AUTH_H
#define AUTH_H

void init_hardware(void);
void init_led_matrix(void);
void pwm_init_buzzer(uint pin);

void set_led_color(uint index, uint8_t r, uint8_t g, uint8_t b);
void led_reset(void);
void write_led_matrix(void);

void pwm_set_frequency(uint pin, uint frequency);
void beep(uint frequency, uint duration_ms);

void display_reset(void);
uint32_t matrix_to_display_value(int line, int column);
void draw_password_line(uint32_t *password, uint length);

bool is_password_correct(void);
void selection_animation(uint matrix_index);
void access_animation(bool allowed);
uint32_t get_auth_type(void);
void start_joystick_auth(void);
void start_sound_auth(void);

#endif