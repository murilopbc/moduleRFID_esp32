#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

// Configurações do leitor RFID
#define RST_PIN 2   // Pino de Reset do MFRC522
#define SS_PIN 5    // Pino Slave Select do MFRC522
#define RED_PIN 4
#define GREEN_PIN 15

MFRC522 rfid(SS_PIN, RST_PIN);  // Instancia o objeto MFRC522

// Configurações de armazenamento de tags
#define MAX_TAGS 10
#define UID_SIZE 4

// Endereços na EEPROM
#define TAGS_COUNT_ADDR 0
#define TAGS_START_ADDR 10  // Início do armazenamento de tags

int senha = 5413;           // Senha para acessar o menu
int tentativaSenha = 0;     // Variável para armazenar a senha digitada
bool continuar = false;     // Flag para controlar se o usuário pode continuar
char resposta;
bool noProcessoCadastro = false;
bool noProcessoDescadastro = false;

void setup() {
  Serial.begin(9600);       
  SPI.begin();              
  rfid.PCD_Init();          
  pinMode(RED_PIN, OUTPUT); 
  pinMode(GREEN_PIN, OUTPUT);
  EEPROM.begin(512);        
  delay(2000);
  menuEscolha();            
}

void clearScreen() { 
    for(int i = 0; i < 10; i++) {
        Serial.println("\n");  // Limpa a tela do Serial Monitor
    }
}

bool isCartaoCadastrado(byte* id) {
  int tagCount = EEPROM.read(TAGS_COUNT_ADDR);
  
  // Percorre todas as tags cadastradas
  for (int tagIndex = 0; tagIndex < tagCount; tagIndex++) {
    bool match = true;
    
    // Compara cada byte do ID
    for (int byteIndex = 0; byteIndex < UID_SIZE; byteIndex++) {
      byte storedByte = EEPROM.read(TAGS_START_ADDR + (tagIndex * UID_SIZE) + byteIndex);
      if (storedByte != id[byteIndex]) {
        match = false;
        break;
      }
    }
    
    // Se todos os bytes coincidirem, o cartão está cadastrado
    if (match) return true;
  }
  
  return false;  // Nenhuma tag coincidente encontrada
}

void menuEscolha() {
  clearScreen();
  Serial.println("SECURE WAY\n");
  Serial.println("1 - Cadastrar Cartão");
  Serial.println("2 - Descadastrar Cartão");
  Serial.println("3 - Listar Cartões Cadastrados");
  Serial.println("4 - Limpar Todos os Cartões");
  int tagCount = EEPROM.read(TAGS_COUNT_ADDR);
  Serial.printf("Cartões cadastrados: %d/%d\n", tagCount, MAX_TAGS);
  Serial.println("Escolha uma opção: ");
}

void pedirSenha() {
  Serial.println("Digite a senha para prosseguir (4 dígitos):");
  
  String entrada = "";  
  while (entrada.length() == 0) {
    if (Serial.available() > 0) {
      entrada = Serial.readStringUntil('\n'); 
      entrada.trim();  
    }
  }

  tentativaSenha = entrada.toInt();

  if (tentativaSenha != senha) {
    Serial.println("Senha incorreta.");
    continuar = false;
  } else {
    Serial.println("Senha correta.");
    continuar = true;
  }
}

void cadastrarCartao() {
  if (!noProcessoCadastro) return;

  int tagCount = EEPROM.read(TAGS_COUNT_ADDR);
  if (tagCount >= MAX_TAGS) {
    Serial.println("Limite máximo de cartões atingido!");
    delay(3000);
    menuEscolha();
    return;
  }

  while (!rfid.PICC_IsNewCardPresent()) {
    delay(100);
  }

  if (rfid.PICC_ReadCardSerial()) {
    byte id[4];
    for (byte i = 0; i < 4; i++) {
      id[i] = rfid.uid.uidByte[i];
    }
    
    if (isCartaoCadastrado(id)) {
      Serial.println("Este cartão já está cadastrado.");
      delay(3000);
      menuEscolha();
      return;
    }

    // Armazena o ID do cartão na EEPROM
    for (int i = 0; i < 4; i++) {
      EEPROM.write(TAGS_START_ADDR + (tagCount * UID_SIZE) + i, id[i]);
    }
    
    // Incrementa o número de tags
    EEPROM.write(TAGS_COUNT_ADDR, tagCount + 1);
    EEPROM.commit();

    Serial.println("Cartão cadastrado com sucesso!");
    delay(3000);
    menuEscolha();
  }
  noProcessoCadastro = false;
}

void descadastrarCartao() {
  if (!noProcessoDescadastro) return;

  while (!rfid.PICC_IsNewCardPresent()) {
    delay(100);
  }
  
  if (rfid.PICC_ReadCardSerial()) {
    byte id[4];
    for (byte i = 0; i < 4; i++) {
      id[i] = rfid.uid.uidByte[i];
    }

    int tagCount = EEPROM.read(TAGS_COUNT_ADDR);
    bool encontrado = false;

    // Procura o cartão e remove
    for (int tagIndex = 0; tagIndex < tagCount; tagIndex++) {
      bool match = true;
      
      // Compara cada byte do ID
      for (int byteIndex = 0; byteIndex < UID_SIZE; byteIndex++) {
        byte storedByte = EEPROM.read(TAGS_START_ADDR + (tagIndex * UID_SIZE) + byteIndex);
        if (storedByte != id[byteIndex]) {
          match = false;
          break;
        }
      }
      
      // Se encontrado, remove o cartão
      if (match) {
        encontrado = true;
        
        // Move os cartões subsequentes para preencher o espaço
        for (int j = tagIndex; j < tagCount - 1; j++) {
          for (int k = 0; k < UID_SIZE; k++) {
            byte nextByte = EEPROM.read(TAGS_START_ADDR + ((j+1) * UID_SIZE) + k);
            EEPROM.write(TAGS_START_ADDR + (j * UID_SIZE) + k, nextByte);
          }
        }
        
        // Decrementa o número de tags
        EEPROM.write(TAGS_COUNT_ADDR, tagCount - 1);
        EEPROM.commit();
        
        Serial.println("Cartão descadastrado com sucesso!");
        break;
      }
    }

    if (!encontrado) {
      Serial.println("Cartão não cadastrado.");
    }
    
    delay(3000);
    menuEscolha();
    noProcessoDescadastro = false;
  }
}

void limparTodosCartoes() {
  // Zera o contador de tags
  EEPROM.write(TAGS_COUNT_ADDR, 0);
  
  // Opcional: Limpa a área de memória onde as tags eram armazenadas
  for (int i = 0; i < (MAX_TAGS * UID_SIZE); i++) {
    EEPROM.write(TAGS_START_ADDR + i, 0);
  }
  
  EEPROM.commit();
  
  Serial.println("Todos os cartões foram descadastrados!");
  delay(3000);
  menuEscolha();
}

void listarCartoesCadastrados() {
  int tagCount = EEPROM.read(TAGS_COUNT_ADDR);
  
  Serial.println("Cartões Cadastrados:");
  for (int tagIndex = 0; tagIndex < tagCount; tagIndex++) {
    Serial.printf("Tag %d: ", tagIndex + 1);
    for (int byteIndex = 0; byteIndex < UID_SIZE; byteIndex++) {
      byte storedByte = EEPROM.read(TAGS_START_ADDR + (tagIndex * UID_SIZE) + byteIndex);
      Serial.printf("%02X ", storedByte);
    }
    Serial.println();
  }
  
  delay(5000);
  menuEscolha();
}

void loop() {
  if (!noProcessoCadastro && !noProcessoDescadastro) {
    if (rfid.PICC_IsNewCardPresent()) {
      if (rfid.PICC_ReadCardSerial()) {
        byte id[4];
        for (byte i = 0; i < 4; i++) {
          id[i] = rfid.uid.uidByte[i];
        }

        if (isCartaoCadastrado(id)) {
          Serial.println("ACESSO LIBERADO.");
          digitalWrite(GREEN_PIN, HIGH);
          delay(3500);
          menuEscolha();
          digitalWrite(GREEN_PIN, LOW);
        } else {
          Serial.println("ACESSO NEGADO.");
          digitalWrite(RED_PIN, HIGH);
          delay(3500);
          menuEscolha();
          digitalWrite(RED_PIN, LOW);
        }

        delay(1000);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
    }
  }

  if (Serial.available() > 0) {
    resposta = Serial.read();
    switch (resposta) {
      case '1':
        pedirSenha();
        if (continuar) {
          Serial.println("Aproxime o cartão para cadastrá-lo.");
          noProcessoCadastro = true;
          cadastrarCartao();
        } else {
          Serial.println("Tente novamente...");
          delay(3000);
          menuEscolha();
        }
        break;
      case '2':
        pedirSenha();
        if (continuar) {
          Serial.println("Aproxime o cartão para descadastrá-lo.");
          noProcessoDescadastro = true;
          descadastrarCartao();
        } else {
          Serial.println("Tente novamente...");
          delay(3000);
          menuEscolha();
        }
        break;
      case '3':
        pedirSenha();
        if (continuar) {
          listarCartoesCadastrados();
        } else {
          Serial.println("Tente novamente...");
          delay(3000);
          menuEscolha();
        }
        break;
      case '4':
        pedirSenha();
        if (continuar) {
          limparTodosCartoes();
        } else {
          Serial.println("Tente novamente...");
          delay(3000);
          menuEscolha();
        }
        break;
      default:
        Serial.println("Escolha inválida. Tente novamente.");
        delay(3000);
        menuEscolha();
        break;
    }
  }
}
