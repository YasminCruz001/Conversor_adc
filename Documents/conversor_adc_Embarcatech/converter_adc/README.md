# Projeto de Controle de LEDs e Display com Joystick

Este projeto foi desenvolvido para consolidar os conceitos de uso de conversores analógico-digitais (ADC), PWM (Modulação por Largura de Pulso) e comunicação I2C com um display SSD1306 no microcontrolador RP2040. O objetivo é controlar a intensidade de LEDs RGB com base nos valores de um joystick e exibir a posição do joystick em um display OLED.

## Funcionalidades

- **Controle de LEDs RGB**:
  - O LED azul é controlado pelo eixo Y do joystick.
  - O LED vermelho é controlado pelo eixo X do joystick.
  - O LED verde é alternado ao pressionar o botão do joystick.

- **Display SSD1306**:
  - Um quadrado de 8x8 pixels se move no display de acordo com a posição do joystick.
  - O botão do joystick alterna entre três estilos de borda no display.

- **Botão A**:
  - Liga/desliga os LEDs controlados por PWM (azul e vermelho).

## Componentes Utilizados

- **Microcontrolador**: RP2040 (Raspberry Pi Pico).
- **Display**: SSD1306 128x64 pixels (comunicação I2C).
- **Joystick**: Analógico com botão integrado.
- **LEDs RGB**: Controlados por PWM.
- **Botão A**: Para desativar/ativar os LEDs PWM.

## Pinagem

| Componente         | Pino RP2040 |
|--------------------|-------------|
| LED Verde          | GPIO 11     |
| LED Azul           | GPIO 12     |
| LED Vermelho       | GPIO 13     |
| Botão A            | GPIO 5      |
| Eixo X do Joystick | GPIO 26     |
| Eixo Y do Joystick | GPIO 27     |
| Botão do Joystick  | GPIO 22     |
| SDA (Display)      | GPIO 14     |
| SCL (Display)      | GPIO 15     |

## Como Configurar o Ambiente
Vídeo de demonstração: https://drive.google.com/file/d/1wcrCikwEn_K1HWtNEH6RqH7_OhaDIjcL/view?usp=drive_link
