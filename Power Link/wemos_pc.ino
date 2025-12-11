/*
 * Sistema de Controle Power PC via MySQL com OTA
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

const char* ssid = "WIfi name";
const char* password = "Wifi password";

const char* apiBaseUrl = "API Link";

const int idDispositivo = 6;

const int pino = D2;

unsigned long ultimaConsulta = 0;
const unsigned long intervaloConsulta = 5000; 
String estadoAnterior = ""; 
bool primeiraLeitura = true; 
const unsigned long duracaoPulso = 500;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSistema de Controle Power PC com OTA");

  pinMode(pino, OUTPUT);
  digitalWrite(pino, LOW); 
  Serial.println("configurado - Estado: REPOUSO\n");
  
  connectWiFi();
  setupOTA();
}

void loop() {
  ArduinoOTA.handle();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado! Reconectando...");
    connectWiFi();
    delay(5000);
    return;
  }
  
  if (millis() - ultimaConsulta >= intervaloConsulta) {
    ultimaConsulta = millis();
    consultarDispositivo();
  }
  
  delay(100);
}

void connectWiFi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  } else {
    Serial.println();
    Serial.println("Erro ao conectar WiFi!");
    Serial.println("Verifique SSID e senha");
    Serial.println();
  }
}

void setupOTA() {
  // Nome do dispositivo na rede
  ArduinoOTA.setHostname("ESP8266-PowerPC");
  
  // Senha para OTA (recomendado por segurança)
  ArduinoOTA.setPassword("admin123");
  
  // Porta OTA (padrão: 8266)
  ArduinoOTA.setPort(8266);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("\n=== Iniciando atualizacao OTA: " + type + " ===");
    digitalWrite(pino, LOW);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n=== Atualizacao concluida! ===");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\nErro[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Falha na autenticacao");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Falha ao iniciar");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Falha na conexao");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Falha ao receber");
    } else if (error == OTA_END_ERROR) {
      Serial.println("Falha ao finalizar");
    }
  });
  
  ArduinoOTA.begin();
  Serial.println("=== OTA CONFIGURADO ===");
  Serial.println("Nome: ESP8266-PowerPC");
  Serial.println("Senha: admin123");
  Serial.print("IP para OTA: ");
  Serial.println(WiFi.localIP());
  Serial.println("=======================\n");
}

void consultarDispositivo() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem conexao WiFi");
    return;
  }
  
  WiFiClient client;
  HTTPClient http;
  
  String url = String(apiBaseUrl) + "?id=" + String(idDispositivo);
  
  Serial.println("--- Consultando Dispositivo ---");
  Serial.print("URL: ");
  Serial.println(url);
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  Serial.print("Codigo HTTP: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.print("Resposta: ");
    Serial.println(payload);
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      bool success = false;
      String status = "";
      bool ativo = false;
      String nome = "";
      
      if (doc.containsKey("success")) {
        success = doc["success"];
        if (success && doc.containsKey("data")) {
          status = doc["data"]["status"].as<String>();
          ativo = doc["data"]["ativo"].as<bool>();
          nome = doc["data"]["nome"].as<String>();
        }
      }
      else if (doc.containsKey("status")) {
        success = true;
        status = doc["status"].as<String>();
        ativo = doc["ativo"].as<bool>();
        nome = doc["nome"].as<String>();
      }
      
      if (success) {
        Serial.print("Nome: ");
        Serial.println(nome);
        Serial.print("Status Atual: ");
        Serial.println(status);
        Serial.print("Ativo: ");
        Serial.println(ativo ? "SIM" : "NAO");
        
        if (ativo) {
          if (primeiraLeitura) {
            estadoAnterior = status;
            primeiraLeitura = false;
            Serial.println("Estado inicial capturado: " + status);
          }
          else if (status != estadoAnterior) {
            Serial.println("\n*** MUDANCA DE ESTADO DETECTADA! ***");
            Serial.print("Estado anterior: ");
            Serial.println(estadoAnterior);
            Serial.print("Estado novo: ");
            Serial.println(status);
            Serial.println();
            
            enviarPulso();
            
            estadoAnterior = status;
            
            atualizarUltimaConexao();
          }
          else {
            Serial.println("Sem mudanca de estado");
          }
        }
        else {
          Serial.println("Dispositivo desativado - aguardando ativacao");
          primeiraLeitura = true;
          estadoAnterior = "";
        }
      } else {
        Serial.println("Dispositivo nao encontrado");
      }
    } else {
      Serial.print("Erro ao parsear JSON: ");
      Serial.println(error.c_str());
    }
  }
  else if (httpResponseCode == 404) {
    Serial.println("Dispositivo nao encontrado (404)");
  }
  else {
    Serial.print("Erro no servidor: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void enviarPulso() {
  Serial.print(">>> ENVIANDO PULSO POWER (");
  Serial.print(duracaoPulso);
  Serial.println("ms) <<<");
  
  digitalWrite(pino, HIGH);
  Serial.println(" LIGADO");
  
  delay(duracaoPulso);
  
  digitalWrite(pino, LOW);
  Serial.println(" DESLIGADO");
  Serial.println(">>> Pulso concluido! <<<\n");
}

void atualizarUltimaConexao() {
  WiFiClient client;
  HTTPClient http;
  
  String url = String(apiBaseUrl) + "?action=update&id=" + String(idDispositivo);
  
  http.begin(client, url);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    Serial.println("Ultima conexao atualizada!");
  }
  
  http.end();
}