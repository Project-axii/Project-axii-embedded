#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

const char* ssid = "nome da rede";
const char* password = "senha da rede";
const char* apiBaseUrl = "LINK_API_DISPOSITIVOS";

struct Botao {
  int pino;              
  int idDispositivo;     
  bool estadoAnterior;   
  unsigned long ultimoDebounce;
  bool botaoPressionado;
  String nome;           
};

Botao botoes[] = {
  {D5, 7, HIGH, 0, false, "Botao 1 - PC Lab"},
  {D6, 6, HIGH, 0, false, "Botao 2 - AC Lab"},
  {D7, 4, HIGH, 0, false, "Botao 3 - Projetor"},
  {D8, 5, HIGH, 0, false, "Botao 4 - Iluminacao"}
};

const int numBotoes = sizeof(botoes) / sizeof(botoes[0]);
const unsigned long debounceDelay = 200;
const int LED_BUILTIN_PIN = LED_BUILTIN;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  
  Serial.println("\nSistema de Controle via Botoes");
  
  for (int i = 0; i < numBotoes; i++) {
    pinMode(botoes[i].pino, INPUT_PULLUP);
    delay(50);
  }
  
  connectWiFi();
  
  ArduinoOTA.setHostname("ESP8266-Controle");
  ArduinoOTA.setPassword("admin");
  
  ArduinoOTA.onStart([]() {
    Serial.println("\nIniciando OTA...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA concluido!");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Falha autenticacao");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha ao iniciar");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha recebimento");
    else if (error == OTA_END_ERROR) Serial.println("Falha finalizacao");
  });
  
  ArduinoOTA.begin();
  
  Serial.println("Sistema pronto!\n");
  
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN_PIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    delay(100);
  }
}

void loop() {
  ArduinoOTA.handle();
  
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    delay(5000);
    return;
  }
  
  for (int i = 0; i < numBotoes; i++) {
    verificarBotao(i);
  }
  
  delay(10);
}

void connectWiFi() {
  Serial.print("Conectando WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nErro ao conectar WiFi!");
  }
}

void verificarBotao(int indice) {
  bool leituraAtual = digitalRead(botoes[indice].pino);
  
  if (leituraAtual != botoes[indice].estadoAnterior) {
    botoes[indice].ultimoDebounce = millis();
  }
  
  if ((millis() - botoes[indice].ultimoDebounce) > debounceDelay) {
    if (leituraAtual == LOW && !botoes[indice].botaoPressionado) {
      botoes[indice].botaoPressionado = true;
      digitalWrite(LED_BUILTIN_PIN, LOW);
      
      Serial.print("\nBotao pressionado: ");
      Serial.println(botoes[indice].nome);
      
      alternarEstadoDispositivo(botoes[indice].idDispositivo, botoes[indice].nome);
      digitalWrite(LED_BUILTIN_PIN, HIGH);
    }
    
    if (leituraAtual == HIGH && botoes[indice].botaoPressionado) {
      botoes[indice].botaoPressionado = false;
    }
  }
  
  botoes[indice].estadoAnterior = leituraAtual;
}

void alternarEstadoDispositivo(int idDispositivo, String nomeBotao) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Erro: Sem conexao WiFi");
    return;
  }
  
  WiFiClient client;
  HTTPClient http;
  
  String urlConsulta = String(apiBaseUrl) + "?id=" + String(idDispositivo);
  http.begin(client, urlConsulta);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode != 200) {
    Serial.println("Erro ao consultar dispositivo");
    http.end();
    return;
  }
  
  String payload = http.getString();
  http.end();
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.println("Erro ao parsear resposta");
    return;
  }
  
  String statusAtual = "";
  bool ativo = false;
  
  if (doc.containsKey("success") && doc["success"]) {
    statusAtual = doc["data"]["status"].as<String>();
    ativo = doc["data"]["ativo"].as<bool>();
  } else if (doc.containsKey("status")) {
    statusAtual = doc["status"].as<String>();
    ativo = doc["ativo"].as<bool>();
  } else {
    Serial.println("Formato de resposta inesperado");
    return;
  }
  
  if (!ativo) {
    Serial.println("Dispositivo desativado");
    return;
  }
  
  String novoStatus = (statusAtual == "online") ? "offline" : "online";
  
  DynamicJsonDocument docUpdate(256);
  docUpdate["id"] = idDispositivo;
  docUpdate["status"] = novoStatus;
  
  String jsonData;
  serializeJson(docUpdate, jsonData);
  
  HTTPClient httpUpdate;
  httpUpdate.begin(client, apiBaseUrl);
  httpUpdate.addHeader("Content-Type", "application/json");
  httpUpdate.setTimeout(5000);
  
  int httpPostCode = httpUpdate.POST(jsonData);
  
  if (httpPostCode == 200) {
    Serial.print("Sucesso! Dispositivo ID ");
    Serial.print(idDispositivo);
    Serial.print(" alterado para: ");
    Serial.println(novoStatus);
  } else {
    Serial.print("Erro ao atualizar. Codigo: ");
    Serial.println(httpPostCode);
  }
  
  httpUpdate.end();
}