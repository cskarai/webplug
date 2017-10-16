#ifndef __ATTINY_CMD_H__
#define __ATTINY_CMD_H__

#define RES_ADC_RESULT           0x0000        /* 0x0000 - 0x3FFF -> ADC */

#define RES_PIN_A                0x4000        /* 0x4000 - 0x40FF -> content of PIN A */
#define RES_PIN_B                0x4100        /* 0x4100 - 0x41FF -> content of PIN B */
#define RES_DDR_A                0x4200        /* 0x4200 - 0x42FF -> content of DDR A */
#define RES_DDR_B                0x4300        /* 0x4300 - 0x43FF -> content of DDR B */
#define RES_PORT_A               0x4400        /* 0x4400 - 0x44FF -> content of PORT A */
#define RES_PORT_B               0x4500        /* 0x4500 - 0x45FF -> content of PORT B */
#define RES_ADC_20X_CALIBRATION  0x4C00        /* 0x4C00 - 0x4FFF -> adc calibration result */

#define RES_HEARTBEAT_OK         0xFFAA        /* 0xFFAA          -> heartbeat ok */
#define RES_SYSTEM_STARTED       0xFFAB        /* 0xFFAB          -> system started */
#define RES_OVERFLOW_ERROR       0xFFEE        /* 0xFFEE          -> overflow */
#define RES_NOP                  0xFDEB        /* 0xFDEB          -> no operation */


#define CMD_SET_DDR_A            0x1000        /* 0x1000 - 0x10FF -> set DDR A */
#define CMD_SET_DDR_A_BITS       0x1100        /* 0x1100 - 0x11FF -> set DDR A bits */
#define CMD_RESET_DDR_A_BITS     0x1200        /* 0x1200 - 0x12FF -> reset DDR A bits */
#define CMD_SET_DDR_B            0x1400        /* 0x1400 - 0x14FF -> set DDR B */
#define CMD_SET_DDR_B_BITS       0x1500        /* 0x1500 - 0x15FF -> set DDR B bits */
#define CMD_RESET_DDR_B_BITS     0x1600        /* 0x1600 - 0x16FF -> reset DDR B bits */
#define CMD_SET_PORT_A           0x1800        /* 0x1800 - 0x18FF -> set PORT A */
#define CMD_SET_PORT_A_BITS      0x1900        /* 0x1900 - 0x19FF -> set PORT A bits */
#define CMD_RESET_PORT_A_BITS    0x1A00        /* 0x1A00 - 0x1AFF -> reset PORT A bits */
#define CMD_TOGGLE_PORT_A_BITS   0x1B00        /* 0x1B00 - 0x1BFF -> toggle PORT A bits */
#define CMD_SET_PORT_B           0x1C00        /* 0x1C00 - 0x1CFF -> set PORT B */
#define CMD_SET_PORT_B_BITS      0x1D00        /* 0x1D00 - 0x1DFF -> set PORT B bits */
#define CMD_RESET_PORT_B_BITS    0x1E00        /* 0x1E00 - 0x1EFF -> reset PORT B bits */
#define CMD_TOGGLE_PORT_B_BITS   0x1F00        /* 0x1F00 - 0x1FFF -> toggle PORT B bits */

#define CMD_REQUEST_HEARTBEAT    0xFE00        /* 0xFE00          -> request for heartbeat */
#define CMD_ADC_START            0xFE01        /* 0xFE01          -> start ADC */
#define CMD_ADC_START_20X        0xFE02        /* 0xFE02          -> start ADC 20X */
#define CMD_ADC_STOP             0xFE03        /* 0xFE03          -> stop ADC */
#define CMD_ESP_RESET            0xFE04        /* 0xFE04          -> request ESP reset */

#define CMD_READ_PIN_A           0xFE20        /* 0xFE20          -> read out PIN A */
#define CMD_READ_PIN_B           0xFE21        /* 0xFE21          -> read out PIN B */
#define CMD_READ_DDR_A           0xFE22        /* 0xFE22          -> read out DDR A */
#define CMD_READ_DDR_B           0xFE23        /* 0xFE23          -> read out DDR B */
#define CMD_READ_PORT_A          0xFE24        /* 0xFE24          -> read out PORT A */
#define CMD_READ_PORT_B          0xFE25        /* 0xFE25          -> read out PORT B */

#define CMD_NONE                 0xFFFE        /* 0xFFFE          -> no command */
#define CMD_NOP                  0xFFFF        /* 0xFFFF          -> no operation, just collect result */


#endif /* __ATTINY_CMD_H__ */
