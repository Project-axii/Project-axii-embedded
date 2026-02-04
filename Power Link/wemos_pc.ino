#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

// ==================== CONFIGURAÇÕES ====================
const char* apSSID = "AXII_POWERLINK";        
const char* apPassword = "12345678";        
const char* apiBaseUrl = "LINK_API_DISPOSITIVOS";

const int idDispositivo = 6;

const int pino = D2;

// ==================== VARIÁVEIS GLOBAIS ====================
ESP8266WebServer server(80);
String ssid = "";
String password = "";
bool modoConfiguracao = false;
int falhasConexao = 0;

unsigned long ultimaConsulta = 0;
const unsigned long intervaloConsulta = 5000; 
String estadoAnterior = ""; 
bool primeiraLeitura = true; 
const unsigned long duracaoPulso = 500;

const int LED_BUILTIN_PIN = LED_BUILTIN;

// ==================== FUNÇÕES DE EEPROM ====================
void salvarWiFi(String s, String p) {
  EEPROM.begin(512);
  
  for (int i = 0; i < 200; i++) {
    EEPROM.write(i, 0);
  }
  
  for (int i = 0; i < s.length() && i < 99; i++) {
    EEPROM.write(i, s[i]);
  }
  
  EEPROM.write(100, '|');
  
  for (int i = 0; i < p.length() && i < 99; i++) {
    EEPROM.write(101 + i, p[i]);
  }
  
  EEPROM.commit();
  EEPROM.end();
  
  Serial.println("=== Salvando na EEPROM ===");
  Serial.println("SSID: " + s);
  Serial.print("Senha: ");
  for(int i = 0; i < p.length(); i++) {
    Serial.print("*");
  }
  Serial.println("\n========================");
}

void lerWiFi() {
  EEPROM.begin(512);
  ssid = "";
  password = "";
  
  for (int i = 0; i < 100; i++) {
    char c = EEPROM.read(i);
    if (c == 0 || c == '|') break;
    ssid += c;
  }
  
  for (int i = 101; i < 200; i++) {
    char c = EEPROM.read(i);
    if (c == 0) break;
    password += c;
  }
  
  EEPROM.end();
  
  if (ssid.length() > 0) {
    Serial.println("Credenciais encontradas:");
    Serial.println("SSID: " + ssid);
    Serial.print("Senha: ");
    for(int i = 0; i < password.length(); i++) {
      Serial.print("*");
    }
    Serial.println();
  } else {
    Serial.println("Nenhuma credencial salva");
  }
}

void limparWiFi() {
  EEPROM.begin(512);
  for (int i = 0; i < 200; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Credenciais apagadas!");
}

// ==================== SERVIDOR WEB ====================
void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html lang="pt-BR">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AXII Power Link - Configuração WiFi</title>
    <style>
      * { 
        margin: 0; 
        padding: 0; 
        box-sizing: border-box; 
      }
      
      body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
        background: linear-gradient(135deg, #1e3c72 0%, #2a5298 50%, #7e22ce 100%);
        min-height: 100vh;
        display: flex;
        flex-direction: column;
        padding: 20px;
        color: #ffffff;
      }
      
      .header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 20px 0;
        margin-bottom: 30px;
      }
      
      .logo {
        display: flex;
        align-items: center;
        gap: 10px;
      }
      
      .logo-icon {
        width: 40px;
        height: 40px;
        background: rgba(34, 197, 94, 0.3);
        border-radius: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-weight: bold;
        font-size: 20px;
      }
      
      .logo-text {
        font-size: 28px;
        font-weight: bold;
        letter-spacing: 2px;
      }
      
      .subtitle {
        font-size: 14px;
        color: rgba(255, 255, 255, 0.7);
        font-weight: normal;
        letter-spacing: 1px;
      }
      
      .status-badge {
        background: rgba(34, 197, 94, 0.2);
        padding: 8px 16px;
        border-radius: 20px;
        font-size: 12px;
        backdrop-filter: blur(10px);
        border: 1px solid rgba(34, 197, 94, 0.3);
      }
      
      .container {
        max-width: 500px;
        width: 100%;
        margin: 0 auto;
      }
      
      .card {
        background: rgba(30, 41, 59, 0.7);
        border-radius: 16px;
        padding: 32px;
        backdrop-filter: blur(20px);
        border: 1px solid rgba(255, 255, 255, 0.1);
        box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
        margin-bottom: 20px;
      }
      
      .card-title {
        font-size: 22px;
        font-weight: 600;
        margin-bottom: 24px;
        color: #ffffff;
      }
      
      .info-box {
        background: rgba(34, 197, 94, 0.15);
        border-left: 3px solid #22c55e;
        padding: 16px;
        border-radius: 8px;
        margin-bottom: 24px;
      }
      
      .info-box p {
        font-size: 13px;
        line-height: 1.6;
        color: rgba(255, 255, 255, 0.9);
        margin-bottom: 8px;
      }
      
      .info-box p:last-child {
        margin-bottom: 0;
      }
      
      .form-group {
        margin-bottom: 20px;
      }
      
      label {
        display: block;
        font-size: 14px;
        font-weight: 500;
        margin-bottom: 8px;
        color: rgba(255, 255, 255, 0.9);
      }
      
      input[type="text"], 
      input[type="password"] {
        width: 100%;
        padding: 14px;
        background: rgba(255, 255, 255, 0.1);
        border: 1px solid rgba(255, 255, 255, 0.2);
        border-radius: 8px;
        font-size: 15px;
        color: #ffffff;
        transition: all 0.3s;
      }
      
      input[type="text"]::placeholder,
      input[type="password"]::placeholder {
        color: rgba(255, 255, 255, 0.5);
      }
      
      input[type="text"]:focus, 
      input[type="password"]:focus {
        outline: none;
        background: rgba(255, 255, 255, 0.15);
        border-color: #22c55e;
        box-shadow: 0 0 0 3px rgba(34, 197, 94, 0.2);
      }
      
      .btn {
        width: 100%;
        padding: 14px;
        background: #22c55e;
        color: white;
        border: none;
        border-radius: 8px;
        font-size: 15px;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.3s;
        margin-top: 12px;
        box-shadow: 0 4px 12px rgba(34, 197, 94, 0.3);
      }
      
      .btn:hover {
        background: #16a34a;
        transform: translateY(-2px);
        box-shadow: 0 6px 20px rgba(34, 197, 94, 0.4);
      }
      
      .btn:active {
        transform: translateY(0);
      }
      
      .btn-reset {
        background: rgba(239, 68, 68, 0.8);
        color: white;
        box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
      }
      
      .btn-reset:hover {
        background: rgba(220, 38, 38, 0.9);
        box-shadow: 0 6px 20px rgba(239, 68, 68, 0.4);
      }
      
      .footer {
        text-align: center;
        margin-top: 20px;
        font-size: 13px;
        color: rgba(255, 255, 255, 0.6);
      }
      
      .power-icon {
        display: inline-block;
        width: 20px;
        height: 20px;
        background: linear-gradient(135deg, #22c55e 0%, #16a34a 100%);
        border-radius: 50%;
        margin-right: 8px;
        vertical-align: middle;
        box-shadow: 0 0 10px rgba(34, 197, 94, 0.5);
      }
    </style>
  </head>
  <body>
    <div class="header">
      <div class="logo">
        <div class="logo-icon">A</div>
        <div>
          <div class="logo-text">AXII</div>
          <div class="subtitle">Power Link</div>
        </div>
      </div>
      <div class="status-badge">Modo Configuração</div>
    </div>
    
    <div class="container">
      <div class="card">
        <h2 class="card-title">
          <span class="power-icon"></span>
          Configuração de Rede WiFi
        </h2>
        
        <div class="info-box">
          <p><strong>Instruções de Conexão:</strong></p>
          <p>1. Digite o nome da sua rede WiFi (SSID)</p>
          <p>2. Digite a senha da rede</p>
          <p>3. Clique em "Conectar à Rede"</p>
          <p>4. O dispositivo irá reiniciar e controlar o PC automaticamente</p>
        </div>
        
        <form action="/save" method="GET">
          <div class="form-group">
            <label for="ssid">Nome da Rede (SSID)</label>
            <input type="text" id="ssid" name="ssid" placeholder="Digite o nome da sua rede" required>
          </div>
          
          <div class="form-group">
            <label for="pass">Senha da Rede</label>
            <input type="password" id="pass" name="pass" placeholder="Digite a senha">
          </div>
          
          <button type="submit" class="btn">Conectar à Rede</button>
        </form>
      </div>
      
      <div class="card">
        <form action="/reset" method="GET">
          <button type="submit" class="btn btn-reset">Limpar Configurações</button>
        </form>
      </div>
      
      <div class="footer">
        <p>Servidor: 192.168.4.1 | AXII Power Link System</p>
      </div>
    </div>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", page);
}

void handleSave() {
  String newSSID = server.arg("ssid");
  String newPASS = server.arg("pass");
  
  if (newSSID.length() == 0) {
    server.send(400, "text/html", "<html><body><h2>Erro: SSID vazio!</h2><a href='/'>Voltar</a></body></html>");
    return;
  }
  
  salvarWiFi(newSSID, newPASS);
  
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <style>
      body { 
        font-family: Arial; 
        text-align: center; 
        padding: 50px;
        background: linear-gradient(135deg, #1e3c72 0%, #2a5298 50%, #7e22ce 100%);
        color: white;
        min-height: 100vh;
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .box {
        background: rgba(30, 41, 59, 0.9);
        color: #fff;
        padding: 40px;
        border-radius: 15px;
        display: inline-block;
        box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        border: 1px solid rgba(34, 197, 94, 0.3);
      }
      .spinner {
        border: 4px solid rgba(255, 255, 255, 0.3);
        border-top: 4px solid #22c55e;
        border-radius: 50%;
        width: 50px;
        height: 50px;
        animation: spin 1s linear infinite;
        margin: 20px auto;
      }
      @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
      }
      h2 { margin-bottom: 10px; }
      p { color: rgba(255, 255, 255, 0.8); line-height: 1.6; }
    </style>
  </head>
  <body>
    <div class="box">
      <h2>Configuração Salva!</h2>
      <div class="spinner"></div>
      <p>Reiniciando o dispositivo...</p>
      <p>O AXII Power Link estará pronto para uso em instantes.</p>
    </div>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", page);
  delay(2000);
  ESP.restart();
}

void handleReset() {
  limparWiFi();
  
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <style>
      body { 
        font-family: Arial; 
        text-align: center; 
        padding: 50px;
        background: linear-gradient(135deg, #dc2626 0%, #ef4444 100%);
        color: white;
        min-height: 100vh;
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .box {
        background: rgba(30, 41, 59, 0.9);
        color: #fff;
        padding: 40px;
        border-radius: 15px;
        display: inline-block;
        border: 1px solid rgba(255, 255, 255, 0.2);
      }
      h2 { margin-bottom: 10px; }
      p { color: rgba(255, 255, 255, 0.8); }
    </style>
  </head>
  <body>
    <div class="box">
      <h2>Configuração Apagada!</h2>
      <p>O dispositivo irá reiniciar em modo de configuração.</p>
    </div>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", page);
  delay(2000);
  ESP.restart();
}

// ==================== CONEXÃO WIFI ====================
bool connectWiFi() {
  Serial.print("Conectando em: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
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
    return true;
  } else {
    Serial.println("\nFalha ao conectar WiFi!");
    return false;
  }
}

void iniciarModoConfiguracao() {
  modoConfiguracao = true;
  
  Serial.println("\n=== MODO CONFIGURAÇÃO ===");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  
  Serial.println("Ponto de acesso criado!");
  Serial.print("Nome da rede: ");
  Serial.println(apSSID);
  Serial.print("Senha: ");
  Serial.println(apPassword);
  Serial.print("IP do servidor: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("\nConecte-se à rede e acesse: http://192.168.4.1");
  Serial.println("========================\n");
  
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/reset", handleReset);
  server.begin();
  
  for(int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN_PIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    delay(100);
  }
}

// ==================== CONTROLE POWER PC ====================
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
  http.setTimeout(5000);
  
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
      // Formato 2: {"id": 6, "status": "online", ...}
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
  Serial.println("-------------------------------\n");
}

void enviarPulso() {
  Serial.print(">>> ENVIANDO PULSO POWER (");
  Serial.print(duracaoPulso);
  Serial.println("ms) <<<");
  
  digitalWrite(pino, HIGH);
  Serial.println("Pino LIGADO");
  
  delay(duracaoPulso);
  
  digitalWrite(pino, LOW);
  Serial.println("Pino DESLIGADO");
  Serial.println(">>> Pulso concluido! <<<\n");
}

void atualizarUltimaConexao() {
  WiFiClient client;
  HTTPClient http;
  
  String url = String(apiBaseUrl) + "?action=update&id=" + String(idDispositivo);
  
  http.begin(client, url);
  http.setTimeout(5000);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    Serial.println("Ultima conexao atualizada!");
  }
  
  http.end();
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n========================================");
  Serial.println("   AXII Power Link");
  Serial.println("   Sistema de Controle Power PC");
  Serial.println("   com Configuração WiFi Integrada");
  Serial.println("========================================\n");
  
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  
  pinMode(pino, OUTPUT);
  digitalWrite(pino, LOW);
  Serial.println("Pino configurado - Estado: REPOUSO\n");
  
  lerWiFi();
  
  if (ssid.length() > 0) {
    bool conectado = false;
    for(int tentativa = 1; tentativa <= 3; tentativa++) {
      Serial.print("Tentativa ");
      Serial.print(tentativa);
      Serial.println(" de 3...");
      
      if (connectWiFi()) {
        conectado = true;
        break;
      }
      
      if (tentativa < 3) {
        Serial.println("Aguardando 2 segundos antes de tentar novamente...\n");
        delay(2000);
      }
    }
    
    if (conectado) {
      // Configurar OTA
      ArduinoOTA.setHostname("AXII Power Link");
      ArduinoOTA.setPassword("admin123");
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
      
      Serial.println("\n=== OTA CONFIGURADO ===");
      Serial.println("Nome: AXII Power Link");
      Serial.println("Senha: admin123");
      Serial.print("IP para OTA: ");
      Serial.println(WiFi.localIP());
      Serial.println("=======================\n");
      
      Serial.println("Sistema pronto para uso!");
      Serial.println("Modo: OPERAÇÃO NORMAL\n");
      
      for(int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN_PIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN_PIN, HIGH);
        delay(200);
      }
    } else {
      Serial.println("\nNão foi possível conectar após 3 tentativas!");
      Serial.println("Entrando em modo de configuração...\n");
      
      iniciarModoConfiguracao();
    }
  } else {
    iniciarModoConfiguracao();
  }
}

// ==================== LOOP ====================
void loop() {
  if (modoConfiguracao) {
    server.handleClient();
  } else {
    ArduinoOTA.handle();
    
    if (WiFi.status() != WL_CONNECTED) {
      falhasConexao++;
      Serial.print("WiFi desconectado! Tentando reconectar... (Falha ");
      Serial.print(falhasConexao);
      Serial.println("/5)");
      
      if(connectWiFi()) {
        falhasConexao = 0;
        Serial.println("Reconectado com sucesso!\n");
      } else {
        if(falhasConexao >= 5) {
          Serial.println("\nMuitas falhas de conexão!");
          Serial.println("Entrando em modo de configuração...");
          Serial.println("Verifique se a rede ainda existe ou se a senha mudou.\n");
          
          digitalWrite(pino, LOW);
          primeiraLeitura = true;
          estadoAnterior = "";
          
          iniciarModoConfiguracao();
          return;
        }
        delay(5000);
      }
      return;
    }
    
    if(falhasConexao > 0) {
      falhasConexao = 0;
    }
    
    if (millis() - ultimaConsulta >= intervaloConsulta) {
      ultimaConsulta = millis();
      consultarDispositivo();
    }
  }
  
  delay(100);
}
