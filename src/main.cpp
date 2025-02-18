#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "HT_SSD1306Wire.h"

// Inicializa o objeto SSD1306Wire para o display OLED da Heltec V2 LoRa
// Endereço I2C: 0x3C, Pinos SDA: 4, SCL: 15
// Endereço de comunicação (deve ser o mesmo no dispositivo sensor)

const uint64_t ADDRESS = 0xE8E8F0F0E1LL;
#define BUFFER_SIZE 16 // MAX 32 int16_t // 32 bytes max - accel = 0,1,2, gyro = 3,4,5, mag = 6,7,8, tempmpu = 9,temntc = 10, tempsens = 11, bat = 12, sg = 13, 14, 15, erro = 16
#define CHANNEL 66

#define BUTTON_PIN    13
#define LED_PIN       25 
#define CE_PIN        12
#define CSN_PIN       17
#define IRQ_PIN       2 
#define NRFSCK        5
#define NRFMISO       19
#define NRFMOSI       23
//#define Vext 21 // Pino Vext no Heltec LoRa V2


static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

volatile bool dataReceived = false; // Flag para indicar que dados foram recebidos
unsigned long buttonPressTime = 0;
bool buttonActive = false;
bool longPressActive = false;

// Inicializa o objeto RF24
RF24 radio(CE_PIN, CSN_PIN);
int16_t rx_buffer[BUFFER_SIZE];
int16_t tx_buffer[1] = {0x00};

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
  
  SPI.begin(NRFSCK, NRFMISO, NRFMOSI);

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
  Serial.println("Accel X: " + String((int16_t)(rx_buffer[0])));  // Linha 1
  Serial.println("Accel Y: " + String((int16_t)(rx_buffer[1]))); // Linha 2
  Serial.println("Accel Z: " + String((int16_t)(rx_buffer[2]))); // Linha 3
  Serial.println("Gyro X: " + String((int16_t)(rx_buffer[3]))); // Linha 4
  Serial.println("Gyro Y: " + String((int16_t)(rx_buffer[4]))); // Linha 5
  Serial.println("Gyro Z: " + String((int16_t)(rx_buffer[5]))); // Linha 6
  Serial.println("Mag X: " + String((int16_t)(rx_buffer[6]))); // Linha 7
  Serial.println("Mag Y: " + String((int16_t)(rx_buffer[7]))); // Linha 8
  Serial.println("Mag Z: " + String((int16_t)(rx_buffer[8]))); // Linha 9
  Serial.println("Temperatura MPU9250: " + String((int16_t)(rx_buffer[9]))); // Linha 10
  Serial.println("Temperatura NTC: " + String((int16_t)(rx_buffer[10]))); // Linha 11
  Serial.println("Nível da bateria: " + String((int16_t)(rx_buffer[11]))); // Linha 12
  Serial.println("SG X: " + String((int16_t)(rx_buffer[12]))); // Linha 13
  Serial.println("SG Y: " + String((int16_t)(rx_buffer[13]))); // Linha 14
  Serial.println("SG Z: "+ String((int16_t)(rx_buffer[14]))); // Linha 15
  Serial.println("ERRO: "+ String((int16_t)(rx_buffer[15]))); // Linha 16

  // Recebe os dados do NRF24L01
  if (receiveData()) {
    // Exibe os dados recebidos no display
    
    display.clear();
    display.drawString(0, 0, "Accel X: " + String((int16_t)(rx_buffer[0])));
    display.drawString(0, 10, "Accel Y: " + String((int16_t)(rx_buffer[1])));
    display.drawString(0, 20, "Accel Z: " + String((int16_t)(rx_buffer[2])));
    display.drawString(0, 30, "Gyro X: " + String((int16_t)(rx_buffer[3])));
    display.drawString(0, 40, "Gyro Y: " + String((int16_t)(rx_buffer[4])));
    display.drawString(0, 50, "Gyro Z: " + String((int16_t)(rx_buffer[5])));
    display.drawString(0, 60, "Mag X: " + String((int16_t)(rx_buffer[6])));
    display.drawString(0, 70, "Mag Y: " + String((int16_t)(rx_buffer[7])));
    display.drawString(0, 80, "Mag Z: " + String((int16_t)(rx_buffer[8])));
    //display.drawString(0, 90, "Temp MPU: " + String((int16_t)(rx_buffer[9])));
    display.drawString(0, 90, "Temp NTC: " + String((int16_t)(rx_buffer[10])));
    display.drawString(0, 100, "Bateria: " + String((int16_t)(rx_buffer[11])));
    //display.drawString(0, 120, "SG X: " + String((int16_t)(rx_buffer[12])));
    //display.drawString(0, 130, "SG Y: " + String((int16_t)(rx_buffer[13])));
    //display.drawString(0, 140, "SG Z: " + String((int16_t)(rx_buffer[14])));
    //display.drawString(0, 150, "ERRO: " + String((int16_t)(rx_buffer[15])));
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
      sendCommand(2);
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
        sendCommand(1);
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
  //new:
    radio.begin();
    if (!radio.begin()) {
      Serial.println("NRF24L01 não encontrado!");
      display.clear();
      display.drawString(0, 0, "NRF24L01");  // Linha 1
      display.drawString(0, 10, "não encontrado!");  // Linha 2
      // Atualiza o display com o conteúdo desenhado
      display.display();
      delay(500);
      display.clear();
      for (int i = 0; i<10; i++){
        radio.begin();
        if (!radio.begin()) {
          Serial.println("NRF24L01 não encontrado!");
          display.clear();
          display.drawString(0, 0, "NRF24L01");  // Linha 1
          display.drawString(0, 10, "não encontrado!");  // Linha 2
          display.drawString(0, 20, "Tentativa: " + String(i));  // Linha 3
          // Atualiza o display com o conteúdo desenhado
          display.display();
          delay(1000);
          display.clear();
        }else{
          break;
        }
      }
    } 
    if (radio.begin()) {
      radio.setChannel(CHANNEL); // Canal 76 (0-125)
      radio.setDataRate(RF24_1MBPS); // Taxa de dados de 1 Mbps (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS)
      radio.setPALevel(RF24_PA_MAX); // Nível de potência mínimo (RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX)
      radio.setCRCLength(RF24_CRC_16); // CRC de 2 bytes
      radio.openReadingPipe(0, ADDRESS); // Abre o pipe de leitura
      radio.openWritingPipe(ADDRESS); // Abre o pipe de escrita
      radio.setPayloadSize(30); // Tamanho do payload para o buffer de 30 bytes
      radio.startListening(); // Inicia no modo RX
      
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
    radio.read(&rx_buffer, sizeof(rx_buffer));
      Serial.println("Dados recebidos:");
      for (int i = 0; i < BUFFER_SIZE; i++) {
          Serial.println(rx_buffer[i]);
      }
    dataReceived = true;
    return true;
  }
  
  dataReceived = false;
  return false;
}

void sendCommand(int16_t command) {
  
  tx_buffer[0] = {command};
  radio.stopListening();
  bool result = radio.write(&command, sizeof(command));
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