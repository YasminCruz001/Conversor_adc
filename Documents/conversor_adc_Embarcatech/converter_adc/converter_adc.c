#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

// Definições dos pinos
#define PIN_BUTTON_A 5
#define PIN_JOY_X 26
#define PIN_JOY_Y 27
#define PIN_JOY_BUTTON 22
#define PIN_GREEN_LED 11
#define PIN_BLUE_LED 12
#define PIN_RED_LED 13
#define MESSAGE_DURATION 2000

// Definições do I2C
#define I2C_PORT_ID i2c1
#define PIN_SDA 14
#define PIN_SCL 15
#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// Variáveis globais
volatile bool green_led_status = false;
uint32_t button_debounce_time = 0;
volatile uint8_t frame_border_style = 0;
volatile bool leds_pwm_enabled = true;
uint32_t button_a_debounce_time = 0;
volatile bool show_led_status_message = false;
volatile uint32_t message_time = 0;
volatile bool previous_button_a_state = true;

// Função de debouncing
bool debounce(uint32_t *last_time)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *last_time > 500)
    {
        *last_time = current_time;
        return true;
    }
    return false;
}

// Função de callback para interrupção do botão do joystick
void joystick_button_callback(uint gpio, uint32_t events)
{
    if (gpio == PIN_JOY_BUTTON)
    {
        if (debounce(&button_debounce_time))
        {
            green_led_status = !green_led_status;
            gpio_put(PIN_GREEN_LED, green_led_status);
            frame_border_style = (frame_border_style + 1) % 3;
        }
    }
    else if (gpio == PIN_BUTTON_A)
    {
        bool current_state = gpio_get(PIN_BUTTON_A);

        while (!current_state && debounce(&button_a_debounce_time))
        {
            leds_pwm_enabled = !leds_pwm_enabled;

            if (!leds_pwm_enabled)
            {
                printf("LEDs OFF\n");
                pwm_set_chan_level(pwm_gpio_to_slice_num(PIN_BLUE_LED), PWM_CHAN_A, 0);
                pwm_set_chan_level(pwm_gpio_to_slice_num(PIN_RED_LED), PWM_CHAN_B, 0);
            }
            else
            {
                printf("LEDs ON\n");
            }
            show_led_status_message = true;
            message_time = to_ms_since_boot(get_absolute_time());
        }
    }
}

// Inicialização do ADC
void initialize_adc()
{
    adc_init();
    adc_gpio_init(PIN_JOY_X);
    adc_gpio_init(PIN_JOY_Y);
}

// Inicialização dos LEDs
void setup_leds()
{
    gpio_init(PIN_GREEN_LED);
    gpio_set_dir(PIN_GREEN_LED, GPIO_OUT);
    gpio_put(PIN_GREEN_LED, false);

    gpio_set_function(PIN_BLUE_LED, GPIO_FUNC_PWM);
    uint slice_num_blue = pwm_gpio_to_slice_num(PIN_BLUE_LED);
    pwm_set_wrap(slice_num_blue, 4095);
    pwm_set_chan_level(slice_num_blue, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_num_blue, true);

    gpio_set_function(PIN_RED_LED, GPIO_FUNC_PWM);
    uint slice_num_red = pwm_gpio_to_slice_num(PIN_RED_LED);
    pwm_set_wrap(slice_num_red, 4095);
    pwm_set_chan_level(slice_num_red, PWM_CHAN_B, 0);
    pwm_set_enabled(slice_num_red, true);
}

// Inicialização do botão do joystick
void setup_joystick_button()
{
    gpio_init(PIN_JOY_BUTTON);
    gpio_set_dir(PIN_JOY_BUTTON, GPIO_IN);
    gpio_pull_up(PIN_JOY_BUTTON);
    gpio_set_irq_enabled_with_callback(PIN_JOY_BUTTON, GPIO_IRQ_EDGE_FALL, true, &joystick_button_callback);
}

// Inicialização do Botão A
void setup_button_a()
{
    gpio_init(PIN_BUTTON_A);
    gpio_set_dir(PIN_BUTTON_A, GPIO_IN);
    gpio_pull_up(PIN_BUTTON_A);
    gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &joystick_button_callback);
}

// Ajuste de brilho do LED azul
void adjust_blue_led_brightness(uint16_t y_value)
{
    if (!leds_pwm_enabled)
        return;

    uint16_t brightness = 0;
    if (y_value < 1980 || y_value > 2100)
    {
        brightness = (y_value > 2048) ? (y_value - 2048) * 2 : (2048 - y_value) * 2;
    }
    pwm_set_chan_level(pwm_gpio_to_slice_num(PIN_BLUE_LED), PWM_CHAN_A, brightness);
}

// Ajuste de brilho do LED vermelho
void adjust_red_led_brightness(uint16_t x_value)
{
    if (!leds_pwm_enabled)
        return;

    uint16_t brightness = 0;
    if (x_value < 1980 || x_value > 2100)
    {
        brightness = (x_value > 2048) ? (x_value - 2048) * 2 : (2048 - x_value) * 2;
        if (brightness > 4095)
        {
            brightness = 4095;
        }
    }
    pwm_set_chan_level(pwm_gpio_to_slice_num(PIN_RED_LED), PWM_CHAN_B, brightness);
}

// Inicialização do display OLED
void setup_display(ssd1306_t *ssd)
{
    i2c_init(I2C_PORT_ID, 400000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    ssd1306_init(ssd, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, DISPLAY_I2C_ADDR, I2C_PORT_ID);
    ssd1306_config(ssd);
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}

// Exibição de mensagens no display OLED
void show_message_on_display(ssd1306_t *ssd, const char *line1, const char *line2)
{
    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, line1, 0, 0);  
    ssd1306_draw_string(ssd, line2, 0, 20); 
    ssd1306_send_data(ssd);
}

// Função para exibir o status dos LEDs
void display_led_status(ssd1306_t *ssd)
{
    ssd1306_fill(ssd, false);

    if (leds_pwm_enabled)
    {
        ssd1306_draw_string(ssd, "LED ON", 20, 24);
    }
    else
    {
        ssd1306_draw_string(ssd, "LED OFF", 8, 24);
    }

    ssd1306_send_data(ssd);
}

// Mover quadrado no display
void move_square_on_display(ssd1306_t *ssd, uint16_t x_value, uint16_t y_value)
{
    if (show_led_status_message)
    {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - message_time < MESSAGE_DURATION)
        {
            display_led_status(ssd);
            return;
        }
        else
        {
            show_led_status_message = false;
        }
    }

    static uint8_t square_x = DISPLAY_WIDTH / 2 - 4;
    static uint8_t square_y = DISPLAY_HEIGHT / 2 - 4;

    square_x = (y_value * DISPLAY_WIDTH) / 4095;
    square_y = DISPLAY_HEIGHT - ((x_value * DISPLAY_HEIGHT) / 4095);

    if (square_x < 0) square_x = 0;
    if (square_x > DISPLAY_WIDTH - 8) square_x = DISPLAY_WIDTH - 8;
    if (square_y < 0) square_y = 0;
    if (square_y > DISPLAY_HEIGHT - 8) square_y = DISPLAY_HEIGHT - 8;

    ssd1306_fill(ssd, false);
    ssd1306_rect(ssd, square_y, square_x, 8, 8, true, true);

    switch (frame_border_style)
    {
    case 0:
        ssd1306_line(ssd, 0, 0, DISPLAY_WIDTH - 1, 0, true);
        ssd1306_line(ssd, 0, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, true);
        break;
    case 1:
        ssd1306_line(ssd, 0, 0, 0, DISPLAY_HEIGHT - 1, true);
        ssd1306_line(ssd, DISPLAY_WIDTH - 1, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, true);
        break;
    case 2:
        ssd1306_line(ssd, 0, 0, DISPLAY_WIDTH - 1, 0, true);
        ssd1306_line(ssd, 0, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, true);
        ssd1306_line(ssd, 0, 0, 0, DISPLAY_HEIGHT - 1, true);
        ssd1306_line(ssd, DISPLAY_WIDTH - 1, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, true);
        break;
    }

    ssd1306_send_data(ssd);
}

int main()
{
    stdio_init_all();
    ssd1306_t display;
    initialize_adc();
    setup_leds();
    setup_joystick_button();
    setup_button_a();

    setup_display(&display);

    while (true)
    {
        uint16_t x_value = adc_read_channel(0);
        uint16_t y_value = adc_read_channel(1);
        adjust_blue_led_brightness(y_value);
        adjust_red_led_brightness(x_value);

        move_square_on_display(&display, x_value, y_value);

        sleep_ms(10); 
    }
}
