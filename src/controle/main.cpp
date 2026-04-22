//================================================================
//  CONTROLE ? ESP32 (esquem†tico U3)
//  PINOS (conforme esquem†tico do controle):
//    SW1 : GPIO0  (D¢)    pull-up interno, ativo em LOW
//    SW2 : GPIO2  (RÇ)
//    SW3 : GPIO3  (Mi)
//    SW4 : GPIO4  (F†)
//    SW5 : GPIO5  (Sol)
//    SW6 : GPIO18 (L†)     GPIO16/17 s∆o TX/RX, usar 18
//    SW7 : GPIO7  (Si)
//
//    Chave deslizante SS12D00 (modo) ? n∆o usada no firmware,
//    pois o modo Ç detectado automaticamente pelo robì via BT.
//
//  ATENÄ«O: GPIO0 Ç o pino de boot do ESP32.
//    Com pull-up interno ativo, funciona normalmente como entrada.
//    PorÇm, se segurar o bot∆o SW1 durante o boot, entra em modo
//    de gravaá∆o. Evite apertar SW1 ao ligar/resetar.
//=================================================================

#include <Arduino.h>
#include <BluetoothSerial.h>


//  Nome BT
#define NOME_BT_CONTROLE  "Controle_Jogo"
#define NOME_BT_ROBO      "Robo_SegLinha"

BluetoothSerial BT;

//  Pinos dos bot‰es
const int pinosBotoes[7] = {0, 2, 3, 4, 5, 18, 7};
//                          Do Re Mi Fa Sol La  Si

const char* nomeNotas[7] = {"Do","Re","Mi","Fa","Sol","La","Si"};

unsigned long ultimoPress[7] = {0};
#define DEBOUNCE_MS 250

// Estado da conex∆o
bool conectado = false;

//  Callback de conex∆o BT
void onBTEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    conectado = true;
    Serial.println("[BT] Robì conectado!");
  }
  if (event == ESP_SPP_CLOSE_EVT) {
    conectado = false;
    Serial.println("[BT] Robì desconectado.");
  }
}


//  SETUP
void setup() {
  Serial.begin(115200);

  // Bot‰es com pull-up interno
  for (int i = 0; i < 7; i++) {
    pinMode(pinosBotoes[i], INPUT_PULLUP);
  }

  // Inicia BT como servidor (o robì se conecta ao controle)
  BT.register_callback(onBTEvent);
  BT.begin(NOME_BT_CONTROLE);

  Serial.println("[CONTROLE] Aguardando conex∆o do robì...");
  Serial.print("[CONTROLE] Nome BT: ");
  Serial.println(NOME_BT_CONTROLE);
}


//  LOOP
void loop() {

  for (int i = 0; i < 7; i++) {
    if (digitalRead(pinosBotoes[i]) == LOW) {
      unsigned long agora = millis();
      if (agora - ultimoPress[i] > DEBOUNCE_MS) {
        ultimoPress[i] = agora;

        Serial.print("[BTN] Pressionado: ");
        Serial.println(nomeNotas[i]);

        if (conectado) {
          BT.write(i);  // envia °ndice da nota: 0=D¢, 1=RÇ ... 6=Si
          Serial.println("[BT] Enviado ao robì.");
        } else {
          Serial.println("[BT] Robì n∆o conectado ? bot∆o ignorado.");
        }
      }
    }
  }

  delay(10);
}
