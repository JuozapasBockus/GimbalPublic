#include "error_handling_api.h"

#include <string.h>
#include "uart_api.h"
#include "gpio.h"

#define UART_FOR_ERRORS eUart_1
#define ERROR_LED_PORT GPIOB
#define ERROR_LED_PIN GPIO_PIN_5
#define HEART_BEAT_LED_PORT GPIOB
#define HEART_BEAT_LED_PIN GPIO_PIN_4

/* TODO: this needs a custom RTC driver to work as intended */
unsigned int GetUpTime (char *Output, unsigned int MaxLength) {
    unsigned int ActualLength = 0;
    if (false) {// for now disabled
        memset(Output, '\0', MaxLength);
        //ActualLength = snprintf(Output, MaxLength, "%d:%d:%d", .Hours, .Minutes, .Seconds);
    }
    return ActualLength;
}

/* TODO: figure out a better place for this function */
GPIO_PinState ToggleGpioPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET) {
        HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
        return GPIO_PIN_SET;
    } else {
        HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
        return GPIO_PIN_RESET;
    }
    /* TODO: figure out an error value */
}

void ReportErrorToUart(const char *Function, int Line, char *File) {
    #define TIME_STRING_LENGTH 10
    /* TODO: make sure this is big enough */
    char Time[TIME_STRING_LENGTH];
    if (!GetUpTime (Time, TIME_STRING_LENGTH)) {
        strncpy(Time, "TST_ERROR", TIME_STRING_LENGTH);
    }
    PrintToUart(UART_FOR_ERRORS, "%s\tERROR in %s at %d of %s\r", Time, Function, Line, File);
    #undef TIME_STRING_LENGTH
}

void RepportErrorByLed(void) {
    /* TODO: GPIO API */
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
}

void ClearErrorLed(void) {
    /* TODO: GPIO API */
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_RESET);
}

void _ReportError(const char *Function, int Line, char *File) {
    RepportErrorByLed();
    ReportErrorToUart(Function, Line, File);
}

void ToggleHeartBeat (void) {
    ToggleGpioPin(HEART_BEAT_LED_PORT, HEART_BEAT_LED_PIN);
}
