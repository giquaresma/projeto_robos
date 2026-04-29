//  CARRINHO SEGUIDOR DE LINHA 
//
//  LOGICA DE MODO AUTOMOTICA VIA BLUETOOTH:
//    Se encontrar o controle via BT - Modo 2 (jogo musical)
//    Se nao encontrar               - Modo 1 (segue-linha puro)
//
// DIRECAO FIXA PRA FRENTE
//
//  PINOS (conforme esquematico da PCB):
//    PWM_M1 = GPIO13  (EN1,2 do L293D esquerdo   - motores M1/M2)
//    PWM_M2 = GPIO12  (EN3,4 do L293D esquerdo   - motores M1/M2)
//    PWM_M3 = GPIO18  (EN1,2 do L293D direito    - motores M3/M4)
//    PWM_M4 = GPIO19  (EN3,4 do L293D direito    - motores M3/M4)
//    BUZZER = GPIO33
//    OUT1_IR= GPIO2   (sonda 1  - mais a esquerda)
//    OUT2_IR= GPIO4   (sonda 2)
//    OUT3_IR= GPIO5   (sonda 3  - central)
//    OUT4_IR= GPIO22  (sonda 4)  
//    OUT5_IR= GPIO23  (sonda 5  - mais a direita)



#include <Arduino.h>
#include <BluetoothSerial.h> 

// Motor
#define PWM_M1  13   
#define PWM_M2  12   
#define PWM_M3  18   
#define PWM_M4  19   

// Sensor 
#define S1  2    
#define S2  4    
#define S3  5    
#define S4  22   
#define S5  23  

// Buzzer
#define BUZZER  33

// Canais LEDC (PWM do ESP32)
#define CH_M1  0
#define CH_M2  1
#define CH_M3  2
#define CH_M4  3
#define CH_BUZ 4
#define PWM_FREQ  5000
#define PWM_RES   8  

// Parƒmetros de controle
#define VEL_BASE      180   // velocidade base
#define VEL_JOGO      160   // velocidade ao andar no jogo ap¢s acerto
#define KP             70   // ganho proporcional (aumente se oscilar pouco, diminua se oscilar muito)
#define TEMPO_JOGO_MS 2000  // ms que o rob“ anda ap¢s acertar

//  Bluetooth
#define NOME_BT_ROBO      "Robo_SegLinha"
#define NOME_BT_CONTROLE  "Controle_Jogo"
#define TEMPO_BUSCA_MS    8000   // quanto tempo tenta encontrar o controle

BluetoothSerial BT;

//  Notas musicais
const int    freqNotas[7]  = {262, 294, 330, 349, 392, 440, 494};
const char*  nomeNotas[7]  = {"Do","Re","Mi","Fa","Sol","La","Si"};

//  Estado global
enum Modo { SEGUE_LINHA, JOGO };
Modo modoAtual = SEGUE_LINHA;

int  notaSorteada    = 0;
bool aguardandoResp  = false;
bool andandoJogo     = false;
unsigned long tInicioAnda = 0;


//  MOTORES
void setMotores(int velEsq, int velDir) {
  velEsq = constrain(velEsq, 0, 255);
  velDir = constrain(velDir, 0, 255);
  ledcWrite(CH_M1, velEsq);
  ledcWrite(CH_M2, velEsq);
  ledcWrite(CH_M3, velDir);
  ledcWrite(CH_M4, velDir);
}

void pararMotores() {
  setMotores(0, 0);
}

//  SEGUE-LINHA ? l¢gica proporcional com 5 sondas
// Sensor BFD-1000: LOW = detecta linha preta
void segueLinha() {
  int s[5];
  s[0] = !digitalRead(S1); 
  s[1] = !digitalRead(S2);
  s[2] = !digitalRead(S3);
  s[3] = !digitalRead(S4);
  s[4] = !digitalRead(S5);

  int soma = s[0]+s[1]+s[2]+s[3]+s[4];

  if (soma == 0) {
    // Perdeu a linha 
    pararMotores();
    return;
  }

  // Erro: pesos -2,-1,0,+1,+2 
  // Multiplicado por 10 para ter resolu‡Æo inteira
  int erro = ((-2*s[0]) + (-1*s[1]) + (0*s[2]) + (1*s[3]) + (2*s[4])) * 10 / soma;

  int correcao = KP * erro / 20;

  setMotores(VEL_BASE - correcao, VEL_BASE + correcao);
}


//  BUZZER
void tocarFreq(int freq, int durMs) {
  ledcSetup(CH_BUZ, freq, PWM_RES);
  ledcWrite(CH_BUZ, 128);
  delay(durMs);
  ledcWrite(CH_BUZ, 0);
}

void tocarAcerto() {
  tocarFreq(262, 120); delay(50);
  tocarFreq(330, 120); delay(50);
  tocarFreq(392, 200);
}

void tocarErro() {
  tocarFreq(180, 200); delay(80);
  tocarFreq(180, 200);
}


//  JOGO
void sortearNota() {
  notaSorteada   = random(0, 7);
  aguardandoResp = true;
  Serial.print("[JOGO] Nota sorteada: ");
  Serial.println(nomeNotas[notaSorteada]);
  tocarFreq(freqNotas[notaSorteada], 800);
}

void processarRespostaJogo(int indice) {
  if (!aguardandoResp) return;
  aguardandoResp = false;

  if (indice == notaSorteada) {
    Serial.println("[JOGO] ACERTO!");
    tocarAcerto();
    andandoJogo  = true;
    tInicioAnda  = millis();
  } else {
    Serial.print("[JOGO] ERRO! Resposta: ");
    Serial.print(nomeNotas[indice]);
    Serial.print(" | Correta: ");
    Serial.println(nomeNotas[notaSorteada]);
    tocarErro();
    delay(400);
    sortearNota();  // toca a nota correta novamente
  }
}


//  BLUETOOTH ? tenta encontrar o controle
bool buscarControle() {
  Serial.println("[BT] Procurando controle...");
  BTScanResults* resultados = BT.discover(TEMPO_BUSCA_MS);
  if (resultados == nullptr) return false;

  for (int i = 0; i < resultados->getCount(); i++) {
    BTAdvertisedDevice* disp = resultados->getDevice(i);
    if (String(disp->getName().c_str()) == NOME_BT_CONTROLE) {
      Serial.println("[BT] Controle encontrado! Modo JOGO ativado.");
      return true;
    }
  }
  Serial.println("[BT] Controle NÇO encontrado. Modo SEGUE-LINHA ativado.");
  return false;
}

void setup() {
  Serial.begin(115200);

  // Sensores IR
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);

  // PWM motores
  ledcSetup(CH_M1, PWM_FREQ, PWM_RES); ledcAttachPin(PWM_M1, CH_M1);
  ledcSetup(CH_M2, PWM_FREQ, PWM_RES); ledcAttachPin(PWM_M2, CH_M2);
  ledcSetup(CH_M3, PWM_FREQ, PWM_RES); ledcAttachPin(PWM_M3, CH_M3);
  ledcSetup(CH_M4, PWM_FREQ, PWM_RES); ledcAttachPin(PWM_M4, CH_M4);
  pararMotores();

  // PWM buzzer
  ledcSetup(CH_BUZ, 1000, PWM_RES);
  ledcAttachPin(BUZZER, CH_BUZ);
  ledcWrite(CH_BUZ, 0);

  randomSeed(analogRead(36)); // pino flutuante como semente aleat¢ria

  // Bluetooth ? inicia e procura o controle
  BT.begin(NOME_BT_ROBO);
  bool achouControle = buscarControle();

  if (achouControle) {
    modoAtual = JOGO;
    Serial.println("[MODO] JOGO MUSICAL");
    delay(500);
    sortearNota();
  } else {
    modoAtual = SEGUE_LINHA;
    Serial.println("[MODO] SEGUE-LINHA");
  }
}

void loop() {

  // ÄÄ MODO 1: SEGUE-LINHA ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
  if (modoAtual == SEGUE_LINHA) {
    segueLinha();
    return;
  }

  // ÄÄ MODO 2: JOGO MUSICAL ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
  // Andando ap¢s acerto ? segue linha por TEMPO_JOGO_MS
  if (andandoJogo) {
    segueLinha();
    if (millis() - tInicioAnda >= TEMPO_JOGO_MS) {
      andandoJogo = false;
      pararMotores();
      delay(600);
      sortearNota();
    }
    return;
  }

  // Aguarda resposta via Bluetooth
  if (BT.available()) {
    int indice = BT.read(); // controle envia 0-6 (¡ndice da nota)
    if (indice >= 0 && indice <= 6) {
      processarRespostaJogo(indice);
    }
  }
}
