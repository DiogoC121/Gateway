  
#define WIFI_Kit_32
//#include ".pio\libdeps\heltec_wifi_lora_32_V2\Heltec ESP32 Dev-Boards\src\heltec.h"
#include <heltec.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <string.h>  
#include <stdint.h>
#include <stdbool.h>

#define button                      13          // Pino do botÃ£o
#define ledAux                      LED_BUILTIN // LED embutido no ESP32 Heltec V2

void IRAM_ATTR handleIRQ();
void sendCommand(uint8_t command);

#define VIN                         2.5     // Tens o de alimenta  o
#define HVIN                        1.25    // Metade da tens o de alimenta  o

#define R_ntc                       10000.0 // Resistor de 10kOhm no divisor de tens o
#define BETA                        3950.0  // Coeficiente Beta do termistor NTC 10k
#define T0                          298.15  // Temperatura de refer ncia em Kelvin (25 C)
#define R0                          10000.0 // Resist ncia nominal do termistor a 25 C (10k?)

#define SCK                         18      // PINO - CLOCK
#define SDO                         23      // PINO - DATA OUT
#define SDI                         19      // PINO - DATA IN
#define SSNRF                       4       // PINO - CHIP SELECT - NRF
#define CENRF                       5       // PINO - CHIP ENABLE - NRF
#define IRQNRF                      15      // PINO - INTERRUPCAO EXTERNA 2 - NRF

/*============================================================================================================================*/
/*
 *  Mapa de registradores do NRF24L01+
 */ 
#define NRF_CONFIG                  0x00    // READ / WRITE   - Configuration Register
#define NRF_EN_AA                   0x01    // READ / WRITE   - Enable "Auto Acknowledgment" Function Disable this functionality to be compatible with nRF2401
#define NRF_EN_RXADDR               0x02    // READ / WRITE   - Enabled RX Addresses
#define NRF_SETUP_AW                0x03    // READ / WRITE   - Setup of Address Widths (common for all data pipes)
#define NRF_SETUP_RETR              0x04    // READ / WRITE   - Setup of Automatic Retransmission
#define NRF_RF_CH                   0x05    // READ / WRITE   - RF Channel
#define NRF_RF_SETUP                0x06    // READ / WRITE   - RF Setup Register
#define NRF_STATUS                  0x07    // READ / WRITE   - Status Register (In parallel to the SPI command word applied on the MOSI pin, the STATUS register is shifted serially out on the MISO pin)
#define NRF_OBSERVE_TX              0x08    // READ           - Transmit observe register
#define NRF_RPD                     0x09    // READ           - Received Power Detector. This register is called CD (Carrier Detect) in the nRF24L01. The name is different in nRF24L01+ due to the different input power level threshold for this bit.
#define NRF_RX_ADDR_P0              0x0A    // READ / WRITE   - Receive address data pipe 0. 5 Bytes maximum length. (LSByte is written first. Write the number of bytes defined by SETUP_AW)        
#define NRF_RX_ADDR_P1              0x0B    // READ / WRITE   - Receive address data pipe 1. 5 Bytes maximum length. (LSByte is written first. Write the number of bytes defined by SETUP_AW)
#define NRF_RX_ADDR_P2              0x0C    // READ / WRITE   - Receive address data pipe 2. Only LSB. MSBytes are equal to RX_ADDR_P1[39:8]
#define NRF_RX_ADDR_P3              0x0D    // READ / WRITE   - Receive address data pipe 3. Only LSB. MSBytes are equal to RX_ADDR_P1[39:8]
#define NRF_RX_ADDR_P4              0x0E    // READ / WRITE   - Receive address data pipe 4. Only LSB. MSBytes are equal to RX_ADDR_P1[39:8]
#define NRF_RX_ADDR_P5              0x0F    // READ / WRITE   - Receive address data pipe 5. Only LSB. MSBytes are equal to RX_ADDR_P1[39:8]
#define NRF_TX_ADDR                 0x10    // READ / WRITE   - Transmit address. Used for a PTX device only. (LSByte is written first) Set RX_ADDR_P0 equal to this address to handle automatic acknowledge if this is a PTX device with Enhanced ShockBurst enabled.        
#define NRF_RX_PW_P0                0x11    // READ / WRITE   - Number of bytes in RX payload in data pipe 0 (1 to 32 bytes). 0 Pipe not used; 1 = 1 byte; ...; 32 = 32 bytes
#define NRF_RX_PW_P1                0x12    // READ / WRITE   - Number of bytes in RX payload in data pipe 1 (1 to 32 bytes). 0 Pipe not used; 1 = 1 byte; ...; 32 = 32 bytes
#define NRF_RX_PW_P2                0x13    // READ / WRITE   - Number of bytes in RX payload in data pipe 2 (1 to 32 bytes). 0 Pipe not used; 1 = 1 byte; ...; 32 = 32 bytes
#define NRF_RX_PW_P3                0x14    // READ / WRITE   - Number of bytes in RX payload in data pipe 3 (1 to 32 bytes). 0 Pipe not used; 1 = 1 byte; ...; 32 = 32 bytes
#define NRF_RX_PW_P4                0x15    // READ / WRITE   - Number of bytes in RX payload in data pipe 4 (1 to 32 bytes). 0 Pipe not used; 1 = 1 byte; ...; 32 = 32 bytes
#define NRF_RX_PW_P5                0x16    // READ / WRITE   - Number of bytes in RX payload in data pipe 5 (1 to 32 bytes). 0 Pipe not used; 1 = 1 byte; ...; 32 = 32 bytes
#define NRF_FIFO_STATUS             0x17    // READ / WRITE   - FIFO Status Register
#define NRF_DYNPD                   0x1C    // READ / WRITE   - Enable dynamic payload length
#define NRF_FEATURE                 0x1D    // READ / WRITE   - Feature Register
  
/*============================================================================================================================*/ 
/*
 * Comandos NRF24L01+
 */ 
  
#define NRF_R_REGISTER              0x00    // Read command and status registers. 000A AAAA; AAAAA = 5 bit Register Map Address
#define NRF_W_REGISTER              0x20    // Write command and status registers. 001A AAAA; AAAAA = 5 bit Register Map Address Executable in power down or standby modes only.
#define NRF_R_RX_PAYLOAD            0x61    // Read RX-payload: 1 - 32 bytes. A read operation always starts at byte 0. Payload is deleted from FIFO after it is read. Used in RX mode.
#define NRF_W_TX_PAYLOAD            0xA0    // Write TX-payload: 1 - 32 bytes. A write operation always starts at byte 0 used in TX payload.
#define NRF_FLUSH_TX                0xE1    // Flush TX FIFO, used in TX mode.
#define NRF_FLUSH_RX                0xE2    // Flush RX FIFO, used in RX mode. Should not be executed during transmission of acknowledge, that is, acknowledge package will not be completed.
#define NRF_REUSE_TX_PL             0xE3    // Used for a PTX device Reuse last transmitted payload. TX payload reuse is active until W_TX_PAYLOAD or FLUSH TX is executed. TX payload reuse must not be activated or deactivated during package transmission.
#define NRF_R_RX_PL_WID             0x60    // Read RX payload width for the top R_RX_PAYLOAD in the RX FIFO.
#define NRF_W_ACK_PAYLOAD           0xAB    // Used in RX mode. Write Payload to be transmitted together with ACK packet on PIPE PPP. (PPP valid in the range from 000 to 101). Maximum three ACK packet payloads can be pending. Payloads with same PPP are handled using first in - first out principle. Write payload: 1 - 32 bytes. A write operation always starts at byte 0.
#define NRF_W_TX_PAYLOAD_NO_ACK     0xB0    // Used in TX mode. Disables AUTOACK on this specific packet.
#define NRF_NOP                     0xFF    // No Operation. Might be used to read the STATUS register
  
/*============================================================================================================================*/ 
/*
 * Bits auxiliares para o  NRF24L01+
 */ 
  
#define ADDRESS                     0x5252E1AB12 //"node3"
/*
#define ENDTX                       0x0009D10830
                                 // 0x3008D10900
#define ENDRX                       0x52A0C1711E                                                   
                                 // 0x1E71C1A052
*/

/*============================================================================================================================*/
/*
 * Prot tipo das fun  es
 */

uint16_t spi_xfer(uint16_t mensagem);
uint16_t getADC(int sensor);
void sleep_mode();
void wake_up();
void tipo_interrupt(int i);
float NTC_To_Temperature(uint16_t adc_value); 
int porcentagem_bateria(float bateria);
void error(int16_t erro);

/*============================================================================================================================*/
/*
 * Vari veis globais
 */


extern bool flag_timer;             // Flag interrup  o timer
extern bool flag_mpu;               // Flag interrup  o mpu
extern bool flag_nrf;               // Flag interrup  o nrf
extern bool enviar_dados;           // Flag para enviar dados
extern bool dados_recebidos;        // Flag para dados recebidos   
extern int16_t receivedCommand;     // Comando recebido pelo NRF24L01+
extern int16_t accel[3], gyro[3], mag[3], temp; //vari veis mpu
extern float temperatura;           // Temperatura em  C
extern float tensao_bateria;        // Tens o da bateria em V
extern float strain_gauge_x;        // Leitura do strain gauge X
extern float strain_gauge_y;        // Leitura do strain gauge Y
extern float strain_gauge_z;        // Leitura do strain gauge Z
