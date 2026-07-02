/* ============================================================================
 *  Board_Stm32Rc663.h  -  pin / SPI map for STM32F103 + CLRC663.
 *  Selected via -DPHDRIVER_STM32RC663_BOARD (see DAL/cfg/BoardSelection.h).
 *  Provides the PHDRIVER_* macros the NxpNfcRdLib RC663 HAL and phDriver_Stm32.c
 *  consume. Model: DAL/boards/Board_Lpc1769Rc663.h.
 *
 *  Pin encoding used here (decoded by phDriver_Stm32.c): (PORT << 8) | PIN
 *  where PORT: A=0, B=1, C=2, D=3 ; PIN: 0..15. Example wiring (SPI1/GPIOA):
 *      SCK=PA5 MISO=PA6 MOSI=PA7 NSS=PA4 (sw), RESET=PB0, IRQ=PB1(EXTI1)
 *  Adjust to your actual board.
 * ==========================================================================*/
#ifndef BOARD_STM32RC663_H
#define BOARD_STM32RC663_H

#define PORTA   0
#define PORTB   1
#define PORTC   2
#define PORTD   3

/* ---- control pins (generic phDriver encoding: (PORT<<8)|PIN) ---- */
#define PHDRIVER_PIN_RESET     ((PORTB << 8) | 0)    /**< RC663 RESET/PDOWN = PB0 */
#define PHDRIVER_PIN_IRQ       ((PORTB << 8) | 1)    /**< RC663 IRQ -> PB1 (EXTI1) */
#define PHDRIVER_PIN_SSEL      ((PORTA << 8) | 4)    /**< SPI NSS (software) = PA4  */

/* ---- SPI pins ---- */
#define PHDRIVER_PIN_SCK       ((PORTA << 8) | 5)
#define PHDRIVER_PIN_MISO      ((PORTA << 8) | 6)
#define PHDRIVER_PIN_MOSI      ((PORTA << 8) | 7)

/* ---- pull configuration (enums from phDriver_Gpio.h) ---- */
#define PHDRIVER_PIN_RESET_PULL_CFG    PH_DRIVER_PULL_UP
#define PHDRIVER_PIN_IRQ_PULL_CFG      PH_DRIVER_PULL_UP
#define PHDRIVER_PIN_NSS_PULL_CFG      PH_DRIVER_PULL_UP

/* ---- IRQ trigger + NVIC ---- */
#define PIN_IRQ_TRIGGER_TYPE    PH_DRIVER_INTERRUPT_RISINGEDGE  /**< CLRC663 IRQ active-high */
#define EINT_PRIORITY           5
#define EINT_IRQn               EXTI1_IRQn

/* ---- logic levels / reset polarity (CLRC663 PDOWN active-high) ---- */
#define PH_DRIVER_SET_HIGH            1
#define PH_DRIVER_SET_LOW            0
#define RESET_POWERDOWN_LEVEL        PH_DRIVER_SET_HIGH
#define RESET_POWERUP_LEVEL          PH_DRIVER_SET_LOW

/* ---- SPI peripheral (used by phbalReg_Stm32Spi.c via STM32 HAL) ---- */
#define RC663_SPI_INSTANCE      SPI1
#define RC663_SPI_BAUD_HZ       5000000U     /**< <= CLRC663 max (~10 MHz) */

#endif /* BOARD_STM32RC663_H */
