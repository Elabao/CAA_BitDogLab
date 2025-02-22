#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "inc/ssd1306.h"
#include "ws2818b.pio.h"

// Definições dos pinos
#define JOYSTICK_X 26  // Pino ADC do joystick
#define BUTTON_NEXT_CATEGORY 5
#define BUTTON_PREV_CATEGORY 6
#define LED_COUNT 25
#define LED_PIN 7
#define LEDR 13
#define LEDG 11
#define LEDB 12

// Estrutura para armazenar as palavras do JSON
typedef struct {
    char *categoria;
    uint8_t cor[3]; // Valores RGB
    char *palavras[20];
    int num_palavras;
} Categoria;

Categoria core_words[] = {
    {"Pronomes", {255, 255, 0}, {"eu", "voce", "ele", "ela", "nos", "eles", "meu", "seu"}, 8},
    {"Verbos", {0, 255, 0}, {"querer", "ter", "ir", "gostar", "precisar", "fazer", "estar", "saber", "ver", "dizer", "vir", "poder"}, 12},
    {"Palavras de Acao", {0, 0, 255}, {"abrir", "fechar", "pegar", "dar", "parar", "ajudar", "entrar", "sair", "segurar", "soltar", "empurrar", "puxar"}, 12},
    {"Palavras de Negacao e Afirmacao", {0, 0, 0}, {"sim", "nao", "pode", "nao pode", "pronto", "mais", "menos", "acabou"}, 8},
    {"Perguntas", {255, 0, 0}, {"o que", "onde", "quando", "quem", "por que", "como"}, 6},
    {"Sentimentos e Estados", {128, 0, 128}, {"feliz", "triste", "bravo", "cansado", "com fome", "com sede", "doente", "bem", "machucado"}, 9},
    {"Objetos do Dia a Dia", {0, 255, 0}, {"casa", "comida", "agua", "banheiro", "brinquedo", "roupa", "cama", "cadeira", "escola", "livro", "telefone"}, 11},
    {"Locais Comuns", {135, 206, 235}, {"casa", "escola", "parque", "loja", "hospital", "rua", "quarto", "cozinha"}, 8},
    {"Pessoas e Relacoes", {255, 127, 80}, {"mae", "pai", "irmao", "irma", "amigo", "professora", "medico"}, 7},
    {"Palavras de Tempo", {222, 184, 135}, {"agora", "depois", "antes", "sempre", "nunca", "ja", "amanha", "ontem", "hoje"}, 9},
    {"Palavras de Localizacao", {255, 192, 203}, {"dentro", "fora", "perto", "longe", "acima", "abaixo", "ao lado", "entre"}, 8}
};

int current_category = 0;
int current_word = 0;
int display_start_word = 0;

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

// Variáveis para uso do PWM e DMA
uint slice_num_r, slice_num_g, slice_num_b;
uint channel_r, channel_g, channel_b;
int dma_chan;
dma_channel_config dma_cfg;
uint16_t pwm_values[3];

// Função para inicializar o PWM
void iniciar_pwm(uint gpio, uint slice_num, uint channel) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    pwm_set_clkdiv(slice_num, 255);
    pwm_set_wrap(slice_num, 255);
    pwm_set_chan_level(slice_num, channel, 0);
    pwm_set_enabled(slice_num, true);
}

// Função para inicializar o DMA
void dma_init() {
    dma_chan = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);
    channel_config_set_dreq(&dma_cfg, DREQ_PWM_WRAP0 + slice_num_r);
    dma_channel_configure(
        dma_chan,
        &dma_cfg,
        &pwm_hw->slice[slice_num_r].cc, // Endereço de destino
        pwm_values,                     // Endereço de origem
        3,                              // Número de transferências
        true                            // Iniciar imediatamente
    );
}


// Inicializa a máquina PIO para controle da matriz de LEDs.
void npInit(uint pin) {
  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

// Atribui uma cor RGB a um LED.
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

// Limpa o buffer de pixels.

void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}


// Escreve os dados do buffer nos LEDs.

void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}


///Atualiza a cor da matriz de LEDs com base na cor da categoria.

void update_led_color(const uint8_t r, const uint8_t g, const uint8_t b) {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, r, g, b);
    }
    npWrite();

    // Atualiza os valores PWM para o LED RGB
    pwm_set_gpio_level(LEDR, r);
    pwm_set_gpio_level(LEDG, g);
    pwm_set_gpio_level(LEDB, b);
}

void display_category(Categoria *category, int selected_word) {
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);

    // Dividir o nome da categoria em duas linhas, se necessário
    char categoria_linha1[21] = {0};
    char categoria_linha2[21] = {0};
    strncpy(categoria_linha1, category->categoria, 16);
    if (strlen(category->categoria) > 16) {
        strncpy(categoria_linha2, category->categoria + 16, 16);
    }

    ssd1306_draw_string(ssd, 0, 0, categoria_linha1);
    ssd1306_draw_string(ssd, 0, 8, categoria_linha2);

    // Exibir as palavras da categoria com rolagem
    int max_display_words = 6; // Número máximo de palavras que podem ser exibidas (8 linhas - 2 linhas para o nome da categoria)
    for (int i = 0; i < max_display_words && (i + display_start_word) < category->num_palavras; i++) {
        int word_index = i + display_start_word;
        if (word_index == selected_word) {
            ssd1306_draw_string(ssd, 0, (i + 2) * 8, "> ");
            ssd1306_draw_string(ssd, 16, (i + 2) * 8, category->palavras[word_index]);
        } else {
            ssd1306_draw_string(ssd, 0, (i + 2) * 8, category->palavras[word_index]);
        }
    }

    render_on_display(ssd, &frame_area);
    update_led_color(category->cor[0], category->cor[1], category->cor[2]); // Atualiza a cor da matriz de LEDs e do LED RGB
}

void next_category() {
    current_category = (current_category + 1) % (sizeof(core_words) / sizeof(core_words[0]));
    current_word = 0;
    display_start_word = 0;
    display_category(&core_words[current_category], current_word);
}

void prev_category() {
    current_category = (current_category - 1 + (sizeof(core_words) / sizeof(core_words[0]))) % (sizeof(core_words) / sizeof(core_words[0]));
    current_word = 0;
    display_start_word = 0;
    display_category(&core_words[current_category], current_word);
}

void next_word() {
    current_word = (current_word + 1) % core_words[current_category].num_palavras;
    if (current_word >= display_start_word + 6) {
        display_start_word++;
    }
    display_category(&core_words[current_category], current_word);
}

void prev_word() {
    current_word = (current_word - 1 + core_words[current_category].num_palavras) % core_words[current_category].num_palavras;
    if (current_word < display_start_word) {
        display_start_word--;
    }
    display_category(&core_words[current_category], current_word);
}

int main() {
    stdio_init_all();

    // Inicialização do I2C e OLED
    i2c_init(i2c1, 100000);  // Ajuste o valor conforme necessário
    gpio_set_function(14, GPIO_FUNC_I2C);  // SDA
    gpio_set_function(15, GPIO_FUNC_I2C);  // SCL
    gpio_pull_up(14);
    gpio_pull_up(15);

    ssd1306_init();

    // Inicialização dos botões
    gpio_init(BUTTON_NEXT_CATEGORY);
    gpio_set_dir(BUTTON_NEXT_CATEGORY, GPIO_IN);
    gpio_pull_up(BUTTON_NEXT_CATEGORY);

    gpio_init(BUTTON_PREV_CATEGORY);
    gpio_set_dir(BUTTON_PREV_CATEGORY, GPIO_IN);
    gpio_pull_up(BUTTON_PREV_CATEGORY);

    // Inicialização do joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_select_input(0);

    // Inicialização da matriz de LEDs
    npInit(LED_PIN);
    npClear();
    npWrite();

    // Inicialização do PWM para o LED RGB
    slice_num_r = pwm_gpio_to_slice_num(LEDR);
    slice_num_g = pwm_gpio_to_slice_num(LEDG);
    slice_num_b = pwm_gpio_to_slice_num(LEDB);
    channel_r = pwm_gpio_to_channel(LEDR);
    channel_g = pwm_gpio_to_channel(LEDG);
    channel_b = pwm_gpio_to_channel(LEDB);
    iniciar_pwm(LEDR, slice_num_r, channel_r);
    iniciar_pwm(LEDG, slice_num_g, channel_g);
    iniciar_pwm(LEDB, slice_num_b, channel_b);

    // Inicialização do DMA para o PWM
    dma_init();

    display_category(&core_words[current_category], current_word);

    uint16_t last_adc_value = adc_read();
    while (true) {
        if (!gpio_get(BUTTON_NEXT_CATEGORY)) {
            next_category();
            sleep_ms(200);
        }
        if (!gpio_get(BUTTON_PREV_CATEGORY)) {
            prev_category();
            sleep_ms(200);
        }

        uint16_t adc_value = adc_read();
        printf("ADC Value: %d\n", adc_value);  // Adiciona print para verificar o valor do joystick
        if (adc_value > 3000 && last_adc_value <= 3000) {  // Corrigido para a direção correta
            prev_word();
            sleep_ms(200);
        } else if (adc_value < 1000 && last_adc_value >= 1000) {  // Corrigido para a direção correta
            next_word();
            sleep_ms(200);
        }
        last_adc_value = adc_value;

        sleep_ms(100);
    }

    return 0;
}