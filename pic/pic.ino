#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

// Configurações do leitor RFID
#define RST_PIN 2 // Pino de Reset do MFRC522
#define SS_PIN 5 
#define RED_PIN 4
#define GREEN_PIN 15
 // Pino Slave Select do MFRC522
MFRC522 rfid(SS_PIN, RST_PIN);

// Tamanho máximo para salvar UIDs
#define MAX_TAGS 10
#define UID_SIZE 4

byte savedTags[MAX_TAGS][UID_SIZE];
int tagCount = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();         // Inicializa o barramento SPI
  rfid.PCD_Init();
  pinMode(RED_PIN, OUTPUT); 
  pinMode(GREEN_PIN, OUTPUT);     // Inicializa o leitor RFID

  Serial.println("Aproxime a tag RFID para cadastrar...");
}

void loop() {
  // Verifica se há uma nova tag próxima
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Lê o UID da tag
  byte uid[UID_SIZE];
  for (byte i = 0; i < UID_SIZE; i++) {
    uid[i] = rfid.uid.uidByte[i];
  }

  // Verifica se a tag já foi cadastrada
  if (isTagRegistered(uid)) {
    Serial.println("Tag já cadastrada!");
    digitalWrite(GREEN_PIN, HIGH);
    delay(2000);
    digitalWrite(GREEN_PIN, LOW);
  } else {
    if (tagCount < MAX_TAGS) {
      saveTag(uid);
      Serial.println("Tag cadastrada com sucesso!");
      digitalWrite(RED_PIN, HIGH);
    delay(2000);
    digitalWrite(RED_PIN, LOW);
    } else {
      Serial.println("Limite de tags cadastradas atingido!");
    }
  }

  rfid.PICC_HaltA(); // Para leitura de novas tags
}

// Verifica se a tag já está cadastrada
bool isTagRegistered(byte *uid) {
  for (int i = 0; i < tagCount; i++) {
    bool match = true;
    for (int j = 0; j < UID_SIZE; j++) {
      if (savedTags[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

// Salva uma nova tag na lista
void saveTag(byte *uid) {
  for (int i = 0; i < UID_SIZE; i++) {
    savedTags[tagCount][i] = uid[i];
  }
  tagCount++;
  printUID(uid);
}

// Imprime o UID da tag no console serial
void printUID(byte *uid) {
  Serial.print("UID: ");
  for (byte i = 0; i < UID_SIZE; i++) {
    Serial.print(uid[i], HEX);
    if (i < UID_SIZE - 1) {
      Serial.print(":");
    }
  }
  Serial.println();
}
