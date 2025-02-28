# CAA_BitDogLab

# BitDogLab CAA - Comunicação Aumentativa e Alternativa

<div align="center">
  <img src="https://img.shields.io/badge/License-MIT-green" alt="License">
  <img src="https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico-blue" alt="Platform">
  <img src="https://img.shields.io/badge/Status-Production%20Ready-success" alt="Status">
</div>

<br>

<div align="center">
  <img src="docs/demo.gif" width="400" alt="Demonstração do Projeto">
</div>

Sistema embarcado **open-source** para auxiliar pessoas com autismo e dificuldades de fala na comunicação cotidiana. Desenvolvido na plataforma **BitDogLab** com Raspberry Pi Pico, utiliza tecnologias como PIO, DMA e PWM para uma experiência responsiva e acessível.
Video explicativo: https://www.youtube.com/watch?v=lh9ZUCZ9jI0
---

## ✨ Funcionalidades Principais

- **Navegação por Categorias Core Words**  
  📚 11 categorias pré-definidas (Pronomes, Verbos, Perguntas, etc.)  
  🔄 Transição fluida via botões físicos e joystick analógico

- **Feedback Visual Intuitivo**  
  🌈 Matriz de LEDs WS2812B com cores por categoria  
  💡 LED RGB controlado por PWM para reforço visual

- **Interface de Usuário Clara**  
  🖥️ Display OLED 128x64 (I2C) com rolagem de palavras  
  🎮 Joystick analógico para seleção precisa

- **Eficiência Técnica**  
  ⚡ Uso de **PIO** (Programmable I/O) para LEDs endereçáveis  
  🚀 **DMA** (Direct Memory Access) para atualizações sem CPU  
  🔋 Baixo consumo energético (~120mA @5V)

---

## 🛠️ Hardware Necessário

| Componente               | Especificações                          | 
|--------------------------|-----------------------------------------|
| Raspberry Pi Pico        | Microcontrolador RP2040                | 
| Display OLED SSD1306     | 0.96" I2C, 128x64 pixels               |
| Matriz de LEDs WS2812B   | 5x5, endereçável individualmente       |
| Joystick Analógico KY-023| 2 eixos + botão                         |
| Botões Tácteis           | 6x6 mm, normalmente aberto             |

---

## ⚡ Instalação

### Pré-requisitos
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- Compilador ARM: `gcc-arm-none-eabi`

### Passo a Passo
1. Clone o repositório:
   ```bash
   git clone https://github.com/seu-usuario/bitdoglab-caa.git
   cd bitdoglab-caa
Configure o ambiente (Linux):

bash
Copy
mkdir build
cd build
cmake ..
make -j4
Grave o firmware no Raspberry Pi Pico:

bash
Copy
cp bitdoglab_caa.uf2 /media/seu-usuario/RPI-RP2/
🎮 Como Usar
Ligue o dispositivo à fonte de energia (5V via USB ou bateria).

Navegue entre categorias:

Botão ▶ (GPIO 5): Próxima categoria

Botão ◀ (GPIO 6): Categoria anterior

Selecione palavras:

Movimento vertical do joystick: navegação para cima/baixo

Feedback visual:

Cores dos LEDs mudam conforme a categoria selecionada

Destaque ">" no OLED indica a palavra atual

📄 Licença
Distribuído sob a licença MIT. Veja LICENSE para detalhes.

<div align="center"> ✨ Feito com ♥ para tornar a comunicação mais acessível ✨ </div> ```
Recursos Adicionais Sugeridos
