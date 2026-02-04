#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

// ==================== CONFIGURAÇÕES ====================
const char* apSSID = "NEXUS_CONFIG";        
const char* apPassword = "12345678";        
const char* apiBaseUrl = "LINK_API_DISPOSITIVOS";  

// ==================== VARIÁVEIS GLOBAIS ====================
ESP8266WebServer server(80);
String ssid = "";
String password = "";
bool modoConfiguracao = false;
int falhasConexao = 0;  

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
    <title>NEXUS - Configuração WiFi</title>
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
        background: #3b82f6;
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
        background: rgba(255, 255, 255, 0.15);
        padding: 8px 16px;
        border-radius: 20px;
        font-size: 12px;
        backdrop-filter: blur(10px);
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
        background: rgba(59, 130, 246, 0.15);
        border-left: 3px solid #3b82f6;
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
        border-color: #3b82f6;
        box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.2);
      }
      
      .btn {
        width: 100%;
        padding: 14px;
        background: #3b82f6;
        color: white;
        border: none;
        border-radius: 8px;
        font-size: 15px;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.3s;
        margin-top: 12px;
        box-shadow: 0 4px 12px rgba(59, 130, 246, 0.3);
      }
      
      .btn:hover {
        background: #2563eb;
        transform: translateY(-2px);
        box-shadow: 0 6px 20px rgba(59, 130, 246, 0.4);
      }
      
      .btn:active {
        transform: translateY(0);
      }
      
      .btn-reset {
        background: rgba(239, 68, 68, 0.8);
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
      
      .grid-icon {
        display: inline-block;
        width: 20px;
        height: 20px;
        background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
        border-radius: 4px;
        margin-right: 8px;
        vertical-align: middle;
      }
    </style>
  </head>
  <body>
    <div class="header">
      <div class="logo">
        <div class="logo-icon">A</div>
        <div>
          <div class="logo-text">AXII</div>
          <div class="subtitle">Nexus</div>
        </div>
      </div>
      <div class="status-badge">Modo Configuração</div>
    </div>
    
    <div class="container">
      <div class="card">
        <h2 class="card-title">
          <span class="grid-icon"></span>
          Configuração de Rede WiFi
        </h2>
        
        <div class="info-box">
          <p><strong>Instruções de Conexão:</strong></p>
          <p>1. Digite o nome da sua rede WiFi (SSID)</p>
          <p>2. Digite a senha da rede</p>
          <p>3. Clique em "Conectar à Rede"</p>
          <p>4. O dispositivo irá reiniciar automaticamente</p>
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
        <p>Servidor: 192.168.4.1 | NEXUS Control System</p>
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
        border-top: 4px solid #3b82f6;
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
      <p>O AXII Nexus estará pronto para uso em instantes.</p>
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
    Serial.println("\n WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n Falha ao conectar WiFi!");
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

// ==================== CONTROLE DE BOTÕES ====================
void verificarBotao(int indice) {
  bool leituraAtual = digitalRead(botoes[indice].pino);
  
  if (leituraAtual != botoes[indice].estadoAnterior) {
    botoes[indice].ultimoDebounce = millis();
  }
  
  if ((millis() - botoes[indice].ultimoDebounce) > debounceDelay) {
    if (leituraAtual == LOW && !botoes[indice].botaoPressionado) {
      botoes[indice].botaoPressionado = true;
      digitalWrite(LED_BUILTIN_PIN, LOW);
      
      Serial.print("\n Botão pressionado: ");
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
    Serial.println("Erro: Sem conexão WiFi");
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
    Serial.print("Erro ao atualizar. Código: ");
    Serial.println(httpPostCode);
  }
  
  httpUpdate.end();
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n========================================");
  Serial.println("   NEXUS - Sistema de Controle");
  Serial.println("   com Configuração WiFi Integrada");
  Serial.println("========================================\n");
  
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  
  for (int i = 0; i < numBotoes; i++) {
    pinMode(botoes[i].pino, INPUT_PULLUP);
    delay(50);
  }
  
  // ===== RESET MANUAL VIA BOTÃO =====
  Serial.println("Dica: Segure o botão D5 por 3 segundos para resetar configurações...");
  bool resetManual = true;
  for(int i = 0; i < 30; i++) {
    if(digitalRead(D5) == HIGH) {
      resetManual = false;
      break;
    }
    digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));
    delay(100);
  }
  
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  
  if(resetManual) {
    Serial.println("\n RESET MANUAL DETECTADO!");
    Serial.println("Limpando configurações...\n");
    limparWiFi();
    
    for(int i = 0; i < 10; i++) {
      digitalWrite(LED_BUILTIN_PIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN_PIN, HIGH);
      delay(100);
    }
    
    iniciarModoConfiguracao();
    return;
  }
  
  lerWiFi();
  
  if (ssid.length() > 0) {
    bool conectado = false;
    for(int tentativa = 1; tentativa <= 2; tentativa++) {
      Serial.print("Tentativa ");
      Serial.print(tentativa);
      Serial.println(" de 2...");
      
      if (connectWiFi()) {
        conectado = true;
        break;
      }
      
      if (tentativa < 3) {
        Serial.println(" Aguardando 2 segundos antes de tentar novamente...\n");
        delay(2000);
      }
    }
    
    if (conectado) {
      ArduinoOTA.setHostname("ESP8266-NEXUS");
      ArduinoOTA.setPassword("admin");
      
      ArduinoOTA.onStart([]() {
        Serial.println("\n Iniciando OTA...");
      });
      
      ArduinoOTA.onEnd([]() {
        Serial.println("\n OTA concluído!");
      });
      
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
      });
      
      ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Erro[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Falha autenticação");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Falha ao iniciar");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Falha conexão");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha recebimento");
        else if (error == OTA_END_ERROR) Serial.println("Falha finalização");
      });
      
      ArduinoOTA.begin();
      
      Serial.println("\n Sistema pronto para uso!");
      Serial.println("Modo: OPERAÇÃO NORMAL\n");
      
      for(int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN_PIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN_PIN, HIGH);
        delay(200);
      }
    } else {
      Serial.println("\n Não foi possível conectar após 2 tentativas!");
      Serial.println("Entrando em modo de configuração...\n");
      limparWiFi();
      
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
          Serial.println("\n Muitas falhas de conexão!");
          Serial.println("Entrando em modo de configuração...");
          Serial.println("Verifique se a rede ainda existe ou se a senha mudou.\n");
          limparWiFi();
          
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
    
    for (int i = 0; i < numBotoes; i++) {
      verificarBotao(i);
    }
  }
  
  delay(10);
}
