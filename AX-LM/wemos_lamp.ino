/*
 * Sistema de Controle de Lâmpada via MySQL com OTA
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

const char* ssid = "WIfi name";
const char* password = "Wifi password";

const char* apiBaseUrl = "API Link";

const int idDispositivo = 7;

const int pinoRele = D2;

unsigned long ultimaConsulta = 0;
const unsigned long intervaloConsulta = 5000; 
bool releAtivo = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSistema de Controle de Lampada com OTA");
  
  pinMode(pinoRele, OUTPUT);
  digitalWrite(pinoRele, LOW); 
  Serial.println("Rele configurado - Estado: DESLIGADO\n");
  
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
  ArduinoOTA.setHostname("ESP8266-Lampada");
  
  // Senha para OTA (recomendado por segurança)
  ArduinoOTA.setPassword("admin123");
  
  // Porta OTA (padrão: 8266)
  ArduinoOTA.setPort(8266);
  
  // Callback quando OTA iniciar
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("\n=== Iniciando atualizacao OTA: " + type + " ===");
    // Desligar relé durante atualização por segurança
    digitalWrite(pinoRele, LOW);
    releAtivo = false;
  });
  
  // Callback ao finalizar
  ArduinoOTA.onEnd([]() {
    Serial.println("\n=== Atualizacao concluida! ===");
  });
  
  // Callback de progresso
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });
  
  // Callback de erro
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
  Serial.println("Nome: ESP8266-Lampada");
  Serial.println("Senha: admin123");
  Serial.print("IP para OTA: ");
  Serial.println(WiFi.localIP());
  Serial.println("=======================\n");
}

void consultarDispositivo() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem conexâo WiFi");
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
      
      // Formato 1: {"success": true, "data": {...}}
      if (doc.containsKey("success")) {
        success = doc["success"];
        if (success && doc.containsKey("data")) {
          status = doc["data"]["status"].as<String>();
          ativo = doc["data"]["ativo"].as<bool>();
          nome = doc["data"]["nome"].as<String>();
        }
      }
      // Formato 2: {"id": 7, "status": "online", ...}
      else if (doc.containsKey("status")) {
        success = true;
        status = doc["status"].as<String>();
        ativo = doc["ativo"].as<bool>();
        nome = doc["nome"].as<String>();
      }
      
      if (success) {
        Serial.print("Nome: ");
        Serial.println(nome);
        Serial.print("Status: ");
        Serial.println(status);
        Serial.print("Ativo: ");
        Serial.println(ativo ? "SIM" : "NAO");
        
        // Online = Liga / Offline = Desliga
        if (ativo && status == "offline") {
          if (!releAtivo) {
            digitalWrite(pinoRele, HIGH);
            releAtivo = true;
            Serial.println(">>> RELE LIGADO - Lampada ACESA <<<");
            atualizarUltimaConexao();
          } else {
            Serial.println(">>> RELE JA ESTA LIGADO <<<");
          }
        } else {
          if (releAtivo) {
            digitalWrite(pinoRele, LOW);
            releAtivo = false;
            Serial.println(">>> RELE DESLIGADO - Lampada APAGADA <<<");
          } else {
            Serial.println(">>> RELE JA ESTA DESLIGADO <<<");
          }
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