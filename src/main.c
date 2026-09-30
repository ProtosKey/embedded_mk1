#include "main.h"
#include "tm1637.h"
#include "keyboard.h"

volatile uint32_t tickCount;
uint32_t last_display_update;
uint16_t counter;
char lastKey;
uint32_t lastScanTime;

typedef enum { 
  ENTER_A, 
  ENTER_B, 
  RESULT 
} CalcState;

CalcState state = ENTER_A;

uint8_t numbers[4];
uint8_t count = 0;

uint8_t divisionError = 0;

int32_t operandA = 0;
char pendingOp = 0;
int32_t lastResult = 0;

uint16_t toNumber(void) {
  uint16_t number = 0;
  for (uint8_t i = 0; i < count; i++) {
    number = number * 10 + numbers[i];
  }
  return number;
}

void show_display(void) {
  tm1637_display_number(toNumber());
}

void showError(void) {
  tm1637_display_digit(0, 0x40);
  tm1637_display_digit(1, 0x40);
  tm1637_display_digit(2, 0x40);
  tm1637_display_digit(3, 0x40);
}

void displayResult(int32_t value) {
  if (value > 9999 || value < -999) {
    showError();
    printf("Overflow!\n");
    return;
  }
  if (value < 0) {
    tm1637_display_number(-value);
    tm1637_display_digit(0, 0x40);
  } else {
    tm1637_display_number(value);
  }
}

int32_t applyOp(int32_t a, char op, int32_t b) {
  divisionError = 0;
  switch (op) {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '\\':
      if (b == 0) {
        printf("Division by zero\n");
        divisionError = 1;
        return 0;
      }
      return a / b;
    default: return b;
  }
}

void clearAll(void) {
  state = ENTER_A;
  count = 0;
  operandA = 0;
  pendingOp = 0;
  lastResult = 0;
  tm1637_display_number(0);
  printf("Cleared\n");
}

void enterKey(char key) {
  if (key >= '0' && key <= '9') {
    if (state == RESULT) {
      state = ENTER_A;
      count = 0;
      operandA = 0;
      pendingOp = 0;
    }
    if (count < 4) {
      numbers[count++] = key - '0';
      show_display();
    }
    return;
  }

  if (key == '+' || key == '-' || key == '*' || key == '\\') {
    if (state == ENTER_A) {
      operandA = toNumber();
      pendingOp = key;
      count = 0;
      state = ENTER_B;
      tm1637_display_number(0);
      printf("A=%ld, OP=%c\n", (long)operandA, pendingOp);
    } else if (state == ENTER_B) {
      int32_t b = toNumber();
      operandA = applyOp(operandA, pendingOp, b);
      pendingOp = key;
      count = 0;
      tm1637_display_number(0);
      printf("A=%ld, op=%c\n", (long)operandA, pendingOp);
    } else if (state == RESULT) {
      operandA = lastResult;
      pendingOp = key;
      count = 0;
      state = ENTER_B;
      tm1637_display_number(0);
      printf("A=%ld, op=%c\n", (long)operandA, pendingOp);
    }
    return;
  }

  if (key == '=') {
    if (state == ENTER_B && pendingOp != 0) {
      int32_t b = toNumber();
      int32_t result = applyOp(operandA, pendingOp, b);
      if (divisionError) {
        showError();
      } else {
        displayResult(result);
      }
      printf("%ld %c %ld = %ld\n", (long)operandA, pendingOp, (long)b, (long)result);
      lastResult = result;
      pendingOp = 0;
      count = 0;
      state = RESULT;
    }
    return;
  }

  if (key == 'C') {
    clearAll();
    return;
  }
}

void osSystickHandler(void) {
  tickCount++;
}

void initGPIO() {
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

  GPIOA->MODER = (GPIOA->MODER & ~(3 << 10)) | (1 << 10);
  GPIOA->OTYPER &= ~(1 << 5);
  GPIOA->OSPEEDR |= (1 << 10);
}

void initUSART2() {
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  GPIOA->MODER = (GPIOA->MODER & ~(0xF << 4)) | (0xA << 4);
  GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFF << 8)) | (1 << 8) | (1 << 12);
  USART2->BRR = 417;
  USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void initSysTick() {
  SysTick->LOAD = 47999;
  SysTick->VAL = 0;
  SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}

int _write(int file, uint8_t *ptr, int len) {
  for (int i = 0; i < len; i++) {
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = ptr[i];
  }
  return len;
}

void updateCurrentKey(char *key) {
  if (*key != '\0') {
    enterKey(*key);
    *key = '\0';
  }
}

int main(void) {
  initGPIO();
  initUSART2();
  initSysTick();
  initKeyboard();
  tm1637_init();

  while (1) {
    scanKeyboard();
    updateCurrentKey(&keyEvent);
  }

  return 0;
}