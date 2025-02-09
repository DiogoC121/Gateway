
#include"../lib/defines.h"
#include <Arduino.h>
#include <SPI.h>
///#include"../lib/nrf24l01.h"
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <heltec.h>

#define BUTTON_PIN    13
#define CE_PIN        5
#define CSN_PIN       4
#define IRQ_PIN       15 

RF24 radio(CE_PIN, CSN_PIN); // Cria o objeto radio

const byte address[6] = {
                            (ADDRESS >> 32) & 0xFF, // Byte 0: 0x52
                            (ADDRESS >> 24) & 0xFF, // Byte 1: 0x52
                            (ADDRESS >> 16) & 0xFF, // Byte 2: 0xE1
                            (ADDRESS >> 8) & 0xFF,  // Byte 3: 0xAB
                            ADDRESS & 0xFF,         // Byte 4: 0x12
                            0x00                    // Byte 5: 0x00
                        };                          // Endereço para comunicação

volatile bool dataReceived = false; // Flag para indicar que dados foram recebidos
unsigned long buttonPressTime = 0;
bool buttonActive = false;
bool longPressActive = false;

void IRAM_ATTR handleIRQ() {
  dataReceived = true; // Sinaliza que há dados recebidos
}

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(IRQ_PIN, INPUT_PULLUP); // Configura o pino IRQ como entrada com pull-up
  
  if (!radio.begin()) {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "NRF24L01 failed!");
    Heltec.display->display();
    while (1);
  }
  
  radio.openReadingPipe(0, address);
  radio.startListening();
  
  // Configura a interrupção no pino IRQ
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), handleIRQ, FALLING);
  
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "NRF24L01 OK!");
  Heltec.display->display();
  delay(1000);
}

void loop() {
  // Verifica se há dados recebidos via interrupção
  if (dataReceived) {
    dataReceived = false; // Reseta a flag
    uint8_t rxBuffer[30];
    bool txFail, rxFail, rxReady;
    radio.whatHappened(txFail, rxFail, rxReady); // Verifica o status do NRF24L01
    
    if (rxReady) { // Se dados foram recebidos
      radio.read(&rxBuffer, sizeof(rxBuffer));
      
      // Processa os dados recebidos
      float mpuAcelX = ((rxBuffer[0] << 8) | rxBuffer[1]) / 100.0;
      float mpuAcelY = ((rxBuffer[2] << 8) | rxBuffer[3]) / 100.0;
      float mpuAcelZ = ((rxBuffer[4] << 8) | rxBuffer[5]) / 100.0;
      float mpuGiroX = ((rxBuffer[6] << 8) | rxBuffer[7]) / 100.0;
      float mpuGiroY = ((rxBuffer[8] << 8) | rxBuffer[9]) / 100.0;
      float mpuGiroZ = ((rxBuffer[10] << 8) | rxBuffer[11]) / 100.0;
      float mpuMagX = ((rxBuffer[12] << 8) | rxBuffer[13]) / 100.0;
      float mpuMagY = ((rxBuffer[14] << 8) | rxBuffer[15]) / 100.0;
      float mpuMagZ = ((rxBuffer[16] << 8) | rxBuffer[17]) / 100.0;
      float mpuTemp = ((rxBuffer[18] << 8) | rxBuffer[19]) / 100.0;
      float ntcTemp = ((rxBuffer[20] << 8) | rxBuffer[21]) / 100.0;
      float batVolt = ((rxBuffer[22] << 8) | rxBuffer[23]) / 100.0;
      float sgX = ((rxBuffer[24] << 8) | rxBuffer[25]) / 100.0;
      float sgY = ((rxBuffer[26] << 8) | rxBuffer[27]) / 100.0;
      float sgZ = ((rxBuffer[28] << 8) | rxBuffer[29]) / 100.0;
      

      // Exibe os dados no monitor serial
      Serial.print("mpuAcelX: "); Serial.println(mpuAcelX);
      Serial.print("mpuAcelY: "); Serial.println(mpuAcelY);
      Serial.print("mpuAcelZ: "); Serial.println(mpuAcelZ);
      Serial.print("mpuGiroX: "); Serial.println(mpuGiroX);
      Serial.print("mpuGiroY: "); Serial.println(mpuGiroY);
      Serial.print("mpuGiroZ: "); Serial.println(mpuGiroZ);
      Serial.print("mpuMagX: "); Serial.println(mpuMagX);
      Serial.print("mpuMagY: "); Serial.println(mpuMagY);
      Serial.print("mpuMagZ: "); Serial.println(mpuMagZ);
      Serial.print("mpuTemp: "); Serial.println(mpuTemp);
      Serial.print("ntcTemp: "); Serial.println(ntcTemp);
      Serial.print("batVolt: "); Serial.println(batVolt);
      Serial.print("sgX: "); Serial.println(sgX);
      Serial.print("sgY: "); Serial.println(sgY);
      Serial.print("sgZ: "); Serial.println(sgZ);

      // Exibe os dados no LCD alguns dos dados
      Heltec.display->clear();
      Heltec.display->drawString(0, 0, "mpuAcelX: " + String(mpuAcelX));
      Heltec.display->drawString(0, 10, "mpuAcelY: " + String(mpuAcelY));
      Heltec.display->drawString(0, 20, "mpuAcelZ: " + String(mpuAcelZ));
      Heltec.display->drawString(0, 30, "mpuGiroX: " + String(mpuGiroX));
      Heltec.display->drawString(0, 40, "mpuGiroY: " + String(mpuGiroY));
      Heltec.display->drawString(0, 50, "mpuGiroZ: " + String(mpuGiroZ));
      Heltec.display->drawString(0, 60, "ntcTemp: " + String(ntcTemp));
      Heltec.display->drawString(0, 70, "batVolt: " + String(batVolt));
      Heltec.display->display();
    }
    // Verifica o estado do botão
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (buttonActive == false) {
        buttonActive = true;
        buttonPressTime = millis();
      }
      if ((millis() - buttonPressTime > 2000) && (longPressActive == false)) {
        longPressActive = true;
        sendCommand(0x02);
      }
    } 
    else {
      if (buttonActive == true) {
        if (longPressActive == true) {
          longPressActive = false;
        } else {
          sendCommand(0x01);
        }
        buttonActive = false;
      }
    }
  }
}

void sendCommand(uint8_t command) {
  radio.stopListening();
  radio.openWritingPipe(address);
  radio.write(&command, sizeof(command));
  radio.startListening();
  
  Serial.print("Command sent: 0x");
  Serial.println(command, HEX);
}