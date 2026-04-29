#include <Arduino.h>
#include <BluetoothSerial.h>

#define NOME_BT_CONTROLE  "Controle_Jogo"
#define NOME_BT_ROBO      "Robo_SegLinha"

BluetoothSerial BT;

// GPIO14 serve como GND para os botões
#define PINO_GND_BOTOES 14

// Pinos dos botões
const int pinosBotoes[7] = {23, 22, 21, 19, 18, 17, 16};
//                           Do  Re  Mi  Fa  Sol La  Si

const char* nomeNotas[7] = {"Do","Re","Mi","Fa","Sol","La","Si"};

unsigned long ultimoPress[7] = {0};
#define DEBOUNCE_MS 250

bool conectado = false;

void onBTEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    conectado = true;
    Serial.println("[BT] Robo conectado!");
  }
  if (event == ESP_SPP_CLOSE_EVT) {
    conectado = false;
    Serial.println("[BT] Robo desconectado.");
  }
}

void setup() {
  Serial.begin(115200);

  // GPIO14 como saída em LOW — faz o papel do GND para os botões
  pinMode(PINO_GND_BOTOES, OUTPUT);
  digitalWrite(PINO_GND_BOTOES, LOW);

  // Botões com pull-up interno — ficam HIGH em repouso, LOW ao pressionar
  for (int i = 0; i < 7; i++) {
    pinMode(pinosBotoes[i], INPUT_PULLUP);
  }

  BT.register_callback(onBTEvent);
  BT.begin(NOME_BT_CONTROLE);

  Serial.println("[CONTROLE] Aguardando conexao do robo...");
  Serial.print("[CONTROLE] Nome BT: ");
  Serial.println(NOME_BT_CONTROLE);
}

void loop() {
  for (int i = 0; i < 7; i++) {
    if (digitalRead(pinosBotoes[i]) == LOW) {
      unsigned long agora = millis();
      if (agora - ultimoPress[i] > DEBOUNCE_MS) {
        ultimoPress[i] = agora;

        Serial.print("[BTN] Pressionado: ");
        Serial.println(nomeNotas[i]);

        if (conectado) {
          BT.write(i);
          Serial.println("[BT] Enviado ao robo.");
        } else {
          Serial.println("[BT] Robo nao conectado - botao ignorado.");
        }
      }
    }
  }

  delay(10);
}