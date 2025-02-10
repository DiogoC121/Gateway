#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "HT_SSD1306Wire.h"

// Inicializa o objeto SSD1306Wire para o display OLED da Heltec V2 LoRa
// Endereço I2C: 0x3C, Pinos SDA: 4, SCL: 15
// Endereço de comunicação (deve ser o mesmo no dispositivo)
const byte address[6] = "00001";
//const byte address[5] = {0x52, 0x52, 0xE1, 0xAB, 0x12}; // "node3"

#define BUTTON_PIN    13
#define LED_PIN       25 
#define CE_PIN        12
#define CSN_PIN       17
#define IRQ_PIN       2 
#define nrsck         5
#define nfmiso        19
#define nrmosi        23

static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

volatile bool dataReceived = false; // Flag para indicar que dados foram recebidos
unsigned long buttonPressTime = 0;
bool buttonActive = false;
bool longPressActive = false;

// Array para armazenar os dados recebidos
uint16_t receivedData[30] = {0};

// Inicializa o objeto RF24
RF24 radio(CE_PIN, CSN_PIN);

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200); // Inicializa a comunicação serial com uma taxa de 115200 bps

  pinMode(LED_PIN, OUTPUT); // Configura o pino do LED como saída
  digitalWrite(LED_PIN, LOW); // Inicia com o LED apagado

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o pino do botão como entrada com pull-up
  
  // Configura a interrupção no pino IRQ_PIN para detectar borda de descida
 // attachInterrupt(digitalPinToInterrupt(IRQ_PIN), handleIRQ, FALLING);

  // Configura a interrupção no pino BUTTON_PIN para detectar mudanças de estado
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
  VextON();
  delay(100);

  // Inicializa o display
  display.init();
  //display.flipScreenVertically(); // Inverte a tela se necessário
  display.screenRotate(ANGLE_90_DEGREE);
  display.setFont(ArialMT_Plain_10); // Define a fonte
  display.setBrightness(200); // Brilho máximo (0 a 255)
  display.drawString(0, 0, "Dispositivo");
  display.drawString(0, 10, "Gateway");
  display.drawString(0, 20, "TCC - Diogo");
  display.drawString(0, 30, "Correia da");
  display.drawString(0, 40, "Silva - UnB");
  display.drawString(0, 50, "15/0058641");
  display.display();

// Escreve no monitor serial
  Serial.println("Dispositivo Gateway");
  Serial.println("TCC - Diogo Correia da Silva");
  Serial.println("15/0058641");
  Serial.println("UnB - FCTE - FGA");

  delay(3000);
  
  SPI.begin(nrsck, nfmiso, nrmosi);

  // Inicializa o NRF24L01
  initNRF24L01();
}
void loop() {
  
  digitalWrite(LED_PIN, LOW);  // Apaga o LED
  // Limpa o display
  display.clear();
  
  display.screenRotate(ANGLE_90_DEGREE);
  display.setFont(ArialMT_Plain_10); // Define a fonte
  display.setBrightness(200); // Brilho máximo (0 a 255)

 //Exibe 12 linhas de texto, cada uma com uma letra
 display.drawString(0, 0, "Accel X: ");  // Linha 1
 display.drawString(0, 10, "Accel Y: "); // Linha 2
 display.drawString(0, 20, "Accel Z: "); // Linha 3
 display.drawString(0, 30, "Gyro X: "); // Linha 4
 display.drawString(0, 40, "Gyro Y: "); // Linha 5
 display.drawString(0, 60, "Mag X: "); // Linha 7
 display.drawString(0, 70, "Mag Y: "); // Linha 8
 display.drawString(0, 80, "Mag Z: "); // Linha 9
 display.drawString(0, 90, "Temp NTC: "); // Linha 10
 display.drawString(0, 100, "Bateria: "); // Linha 11
 display.drawString(0, 110, "SG X:  Y:  Z: "); // Linha 12

  // Atualiza o display com o conteúdo desenhado
  display.display();

  // Escreve no monitor serial
  Serial.println("Accel X: ");  // Linha 1
  Serial.println("Accel Y: "); // Linha 2
  Serial.println("Accel Z: "); // Linha 3
  Serial.println("Gyro X: "); // Linha 4
  Serial.println("Gyro Y: "); // Linha 5
  Serial.println("Gyro Z: \t"); // Linha 6
  Serial.println("Mag X: "); // Linha 7
  Serial.println("Mag Y:"); // Linha 8
  Serial.println("Mag Z"); // Linha 9
  Serial.println("Temperatura NTC: "); // Linha 10
  Serial.println("Nível da bateria: "); // Linha 11
  Serial.println("SG X: "); // Linha 12
  Serial.println("SG Y: "); // Linha 13
  Serial.println("SG Z: "); // Linha 14

  // Recebe os dados do NRF24L01
  if (receiveData()) {
    // Exibe os dados recebidos no display
    
    display.clear();
    display.drawString(0, 0, "Accel X: " + String((int16_t)(receivedData[0] << 8 | receivedData[1])));
    display.drawString(0, 10, "Accel Y: " + String((int16_t)(receivedData[2] << 8 | receivedData[3])));
    display.drawString(0, 20, "Accel Z: " + String((int16_t)(receivedData[4] << 8 | receivedData[5])));
    display.drawString(0, 30, "Gyro X: " + String((int16_t)(receivedData[6] << 8 | receivedData[7])));
    display.drawString(0, 40, "Gyro Y: " + String((int16_t)(receivedData[8] << 8 | receivedData[9])));
    display.drawString(0, 50, "Gyro Z: " + String((int16_t)(receivedData[10] << 8 | receivedData[11])));
    display.drawString(0, 60, "Mag X: " + String((int16_t)(receivedData[12] << 8 | receivedData[13])));
    display.drawString(0, 70, "Mag Y: " + String((int16_t)(receivedData[14] << 8 | receivedData[15])));
    display.drawString(0, 80, "Mag Z: " + String((int16_t)(receivedData[16] << 8 | receivedData[17])));
    //display.drawString(0, 90, "Temp MPU: " + String((int16_t)(receivedData[18] << 8 | receivedData[19])));
    display.drawString(0, 90, "Temp NTC: " + String((int16_t)(receivedData[20] << 8 | receivedData[21])));
    display.drawString(0, 100, "Bateria: " + String((int16_t)(receivedData[22] << 8 | receivedData[23])));
    //display.drawString(0, 120, "SG X: " + String((int16_t)(receivedData[24] << 8 | receivedData[25])));
    //display.drawString(0, 130, "SG Y: " + String((int16_t)(receivedData[26] << 8 | receivedData[27])));
    //display.drawString(0, 140, "SG Z: " + String((int16_t)(receivedData[28] << 8 | receivedData[29])));
    display.display();
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    digitalWrite(LED_PIN, HIGH);  // Acende o LED
    if (buttonActive == false) {
      buttonActive = true;
      buttonPressTime = millis();
    }
    if ((millis() - buttonPressTime > 1000) && (longPressActive == false)) {
      longPressActive = true;
      sendCommand(0x02);
      Serial.println("Enviando comando 0x02");  // Comando de pressionamento longo
        display.clear();
        display.drawString(0, 0, "Comando: ");  // Linha 1
        display.drawString(0, 10, "0x02");  // Linha 2
        // Atualiza o display com o conteúdo desenhado
        display.display();
    }
  } else {
    digitalWrite(LED_PIN, LOW);  // Apaga o LED
    if (buttonActive == true) {
      if (longPressActive == true) {
        longPressActive = false;
      } else {
        sendCommand(0x01);
        Serial.println("Enviando comando 0x01");  // Comando de pressionamento curto
        display.clear();
        display.drawString(0, 0, "Comando: ");  // Linha 1
        display.drawString(0, 10, "0x01");  // Linha 2
        // Atualiza o display com o conteúdo desenhado
        display.display();
      }
      buttonActive = false;
      
    }
  }
  // Aguarda um pouco antes de repetir
  delay(1000);
}

void initNRF24L01() {
  // Inicializa o NRF24L01
  if (!radio.begin()) {
    Serial.println("NRF24L01 não encontrado!");
    display.clear();
    display.drawString(0, 0, "NRF24L01");  // Linha 1
    display.drawString(0, 10, "não encontrado!");  // Linha 2
    // Atualiza o display com o conteúdo desenhado
    display.display();
    delay(3000);
    
    display.clear();
  } else{

    // Configura o endereço do pipe 0
    radio.openReadingPipe(0, address);

    // Configura a potência do transmissor (RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX)
    radio.setPALevel(RF24_PA_LOW);

    // Configura a taxa de transmissão (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS)
    radio.setDataRate(RF24_250KBPS);

    // Configura o canal de comunicação (0-125)
    radio.setChannel(76);

    // Habilita o modo de recepção
    radio.startListening();

    Serial.println("NRF24L01 inicializado com sucesso!");
    display.clear();
    display.drawString(0, 0, "NRF24L01");  // Linha 1
    display.drawString(0, 10, "inicializado!");  // Linha 2
    // Atualiza o display com o conteúdo desenhado
    display.display();
    delay(1000);
  }
}

bool receiveData() {
  if (radio.available()) {
    // Lê os dados recebidos
    radio.read(&receivedData, sizeof(receivedData));
    dataReceived = true;
    return true;
  }
  
  dataReceived = false;
  return false;
}

void sendCommand(uint8_t command) {
  // Para enviar um comando, primeiro paramos de ouvir
  radio.stopListening();

  // Abrimos o pipe de escrita com o mesmo endereço
  radio.openWritingPipe(address);

  // Enviamos o comando
  bool result = radio.write(&command, sizeof(command));

  // Voltamos a ouvir
  radio.startListening();

  if (result) {
    Serial.println("Comando enviado com sucesso!");
    display.clear();
    display.drawString(0, 0, "Comando");  // Linha 1
    display.drawString(0, 10, "enviado!");  // Linha 2
    // Atualiza o display com o conteúdo desenhado
    display.display();
    delay(1000);
  } else {
    Serial.println("Falha ao enviar comando!");
    display.clear();
    display.drawString(0, 0, "Comando");  // Linha 1
    display.drawString(0, 10, "não enviado!");  // Linha 2
    // Atualiza o display com o conteúdo desenhado
    display.display();
    delay(1000);
  }
}