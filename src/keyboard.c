#include "keyboard.h"

volatile char keyEvent = '\0';

void initKeyboard() {
  // Настройка строк (R1-R4) как выходы с открытым стоком
  // PA0, PA1, PA10, PA4
  GPIOA->MODER = (GPIOA->MODER & ~(3U << (0 * 2))) | (1U << (0 * 2));
  GPIOA->MODER = (GPIOA->MODER & ~(3U << (1 * 2))) | (1U << (1 * 2));
  GPIOA->MODER = (GPIOA->MODER & ~(3U << (10 * 2))) | (1U << (10 * 2));
  GPIOA->MODER = (GPIOA->MODER & ~(3U << (4 * 2))) | (1U << (4 * 2));
  GPIOA->OTYPER |= (1 << 0) | (1 << 1) | (1 << 10) | (1 << 4);

  // Настройка столбцов (C1-C3) как входы с подтяжкой к питанию
  // PA8, PA9, PA11, PA15
  GPIOA->MODER &= ~(3U << (8 * 2) | 3U << (9 * 2) | 3U << (11 * 2) | 3U << (15 * 2));
  GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (8 * 2))) | (1U << (8 * 2));
  GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (9 * 2))) | (1U << (9 * 2));
  GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (11 * 2))) | (1U << (11 * 2));
  GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (15 * 2))) | (1U << (15 * 2));

  lastKey = '\0';
  lastScanTime = 0;
}

char readKey() {
  const uint8_t rows[] = {0, 1, 10, 4};    // Пины строк
  const uint8_t cols[] = {8, 9, 11, 15};       // Пины столбцов
  const char keymap[4][4] = {
    {'1', '2', '3', '\\'},
    {'4', '5', '6', '*'},
    {'7', '8', '9', '-'},
    {'C', '0', '=', '+'}
  };

  for (uint8_t i = 0; i < 4; i++) {
    // Активируем текущую строку
    GPIOA->BSRR = (1 << (rows[i] + 16));

    // Небольшая задержка для стабилизации
    for (volatile int d = 0; d < 50; d++);

    // Проверяем столбцы
    for (uint8_t j = 0; j < 4; j++) {
      if ((GPIOA->IDR & (1 << cols[j])) == 0) {
        // Деактивируем строку перед возвратом
        GPIOA->BSRR = (1 << rows[i]);
        return keymap[i][j];
      }
    }

    // Деактивируем строку
    GPIOA->BSRR = (1 << rows[i]);
  }

  return '\0';
}

void scanKeyboard() {
  // Сканируем клавиатуру каждые 100мс
  if (tickCount - lastScanTime > 100) {
    lastScanTime = tickCount;
    char currentKey = readKey();

    if (currentKey != '\0' && currentKey != lastKey) {
      keyEvent = currentKey;
      printf("Pressed key: %c\n", keyEvent);
      lastKey = currentKey;
    } else if (currentKey == '\0') {
      lastKey = '\0';
    }
  }
}
