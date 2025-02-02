#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define BTN_A_PIN 5
#define BTN_B_PIN 6
#define LED_R_PIN 11 // GPIO 11 para o vermelho
#define LED_G_PIN 12 // GPIO 12 para o verde
#define LED_B_PIN 13 // GPIO 13 para o azul

// Variável global para armazenar a cor do LED piscando
uint8_t blink_led_r = 74;  // Intensidade do vermelho
uint8_t blink_led_g = 5;   // Intensidade do verde
uint8_t blink_led_b = 176; // Intensidade do azul

// Representação de números 0-9 como matrizes 5x5 (onde 1 é aceso e 0 é apagado)
bool led_buffer[10][5][5] = {
    // ZERO
    {
        {0, 1, 1, 1, 0},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 1, 1, 0}
    },
    // UM
    {
        {1, 1, 1, 1, 1},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 1},
        {0, 1, 1, 0, 0},
        {0, 0, 1, 0, 0}
    },
    // DOIS
    {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    // TRÊS
    {
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    // QUATRO
    {
        {1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1}
    },
    // CINCO
    {
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1}
    },
    // SEIS
    {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1}
    },
    // SETE
    {
        {1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    // OITO
    {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    // NOVE
    {
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    }
};

// Função para enviar pixels para os LEDs WS2812
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função para gerar o valor GRB para o LED
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Função para animar os LEDs na matriz
void animate_leds(int start, int end, int step)
{
    for (int i = start; (step > 0) ? i <= end : i >= end; i += step)
    {
        for (int row = 0; row < 5; row++)
        {
            for (int col = 0; col < 5; col++)
            {
                if (led_buffer[i][row][col])
                {
                    put_pixel(urgb_u32(74, 5, 176)); // Cor roxo da meia-noite
                }
                else
                {
                    put_pixel(0); // Desliga o LED
                }
            }
        }
        sleep_ms(750); // Tempo de espera entre as animações
    }
}

int main()
{
    stdio_init_all();

    // Inicializa o PIO para controlar os LEDs WS2812
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);

    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);

    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);

    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);

    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    bool led_state = false;
    uint64_t last_toggle_time = 0;
    uint64_t blink_duration = 500000; // 500ms em microssegundos

    while (true)
    {
        uint64_t current_time = time_us_64();

        // Controle de piscar o LED nas GPIOs 11, 12 e 13 (RGB)
        if (current_time - last_toggle_time >= blink_duration)
        {
            led_state = !led_state; // Alterna o estado do LED
            gpio_put(LED_R_PIN, led_state ? blink_led_r : 0);
            gpio_put(LED_G_PIN, led_state ? blink_led_g : 0);
            gpio_put(LED_B_PIN, led_state ? blink_led_b : 0);
            last_toggle_time = current_time;
        }

        // Controle da animação dos LEDs na matriz
        if (!gpio_get(BTN_A_PIN))
        {
            animate_leds(0, 9, 1); // Botão A: animação de 0 a 9
        }
        else if (!gpio_get(BTN_B_PIN))
        {
            animate_leds(9, 0, -1); // Botão B: animação de 9 a 0
        }
        else
        {
            // Desliga todos os LEDs
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                put_pixel(0);
            }
        }

        sleep_ms(10); // Pequeno delay para evitar uso excessivo da CPU
    }
}
