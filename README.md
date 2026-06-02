<div align="center">

<img src="https://lfcostldktmoevensqdj.supabase.co/storage/v1/object/public/axii/white-logo.svg" alt="AXII Logo" width="120" />

# AXII — Embedded (Firmware IoT)

**Firmware dos dispositivos físicos do sistema AXII para automação de salas de aula, desenvolvido com ESP8266 e programado em C++ via Arduino IDE.**

[![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![ESP8266](https://img.shields.io/badge/ESP8266-WeMos_D1_Mini-blue)](https://www.wemos.cc/)
[![C++](https://img.shields.io/badge/C++-firmware-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![IoT](https://img.shields.io/badge/IoT-WiFi_%2B_OTA-green)](https://en.wikipedia.org/wiki/Internet_of_things)
[![Licença MIT](https://img.shields.io/badge/licença-MIT-blue)](LICENSE)

</div>

---

## Sobre o Projeto

O **AXII Embedded** contém o firmware de três módulos físicos baseados em **ESP8266 (WeMos D1 Mini)** que integram o sistema AXII à infraestrutura real da sala de aula. Esses módulos se comunicam com a mesma API PHP compartilhada com o painel web e o app mobile, executando ações físicas com base no estado dos dispositivos registrados no banco de dados.

O sistema AXII completo é composto por:
- **Embedded** (este repositório) — módulos físicos IoT que atuam diretamente nos equipamentos
- **Web** — painel de controle via navegador
- **Mobile** — controle e monitoramento pelo celular
- **Desktop** — cliente instalado nos computadores das salas

---

## Módulos

### AX-LM — Controlador de Iluminação
Controla a **iluminação da sala** por meio de um relé conectado ao ESP8266. Consulta a API a cada 5 segundos e liga ou desliga a luz conforme o status do dispositivo.

### Power Link — Controle de Energia dos PCs
Simula o pressionamento do **botão físico de power** dos computadores. Detecta mudanças de estado na API e envia um pulso elétrico de 500ms ao botão de energia do PC.

### Nexus — Central de Controle Manual
Painel físico com **4 botões** que permite ao professor controlar todos os dispositivos da sala sem precisar do celular ou computador. Cada botão alterna o estado de um dispositivo na API.

---

## Como funciona

```
┌───────────────────────────────────────────┐
│              API AXII (PHP)               │
│         Banco de dados de dispositivos    │
└──────────────┬────────────────────────────┘
               │  HTTP (GET/POST)
    ┌──────────┼─────────────────────┐
    ↓           ↓                    ↓
┌─────────┐  ┌────────────┐   ┌──────────┐
│  AX-LM  │  │ Power Link │   │  Nexus   │
│ (Relé)  │  │ (Pulso D2) │   │(4 Botões)│
│ ID: 7   │  │  ID: 6     │   │IDs:4,5,6,7│
└─────────┘  └────────────┘   └──────────┘
 Iluminação   Liga/desliga PC   Controle manual
```

**Fluxo de controle:**
- **AX-LM e Power Link** — controladores automáticos: consultam a API periodicamente e executam ações físicas com base no status retornado.
- **Nexus** — controlador manual: o botão pressiona → atualiza o status na API → os outros módulos respondem automaticamente à mudança.

---

## Módulos em Detalhe

### AX-LM — Controlador de Iluminação

| Propriedade | Valor |
|---|---|
| Arquivo | `AX-LM/wemos_lamp.ino` |
| ID do Dispositivo | `7` |
| Pino GPIO | `D2` (relé) |
| AP de Configuração | `AX-LM_CONFIG` / senha: `12345678` |
| OTA Hostname | `AX-LM` |
| OTA Senha | `admin123` |
| Intervalo de Consulta | 5 segundos |

**Lógica de operação:**
- Consulta a API a cada 5 segundos com `GET {apiBaseUrl}?id=7`
- Se `ativo = true` e `status = "offline"` → **liga o relé** (iluminação acesa)
- Se `ativo = false` ou `status = "online"` → **desliga o relé** (iluminação apagada)
- Atualiza o campo `ultima_conexao` na API ao ligar o relé

**Arquivos de hardware:**
- `circuito.pdf` — diagrama do circuito elétrico

---

### Power Link — Controle de Energia dos PCs

| Propriedade | Valor |
|---|---|
| Arquivo | `Power Link/wemos_pc.ino` |
| ID do Dispositivo | `6` |
| Pino GPIO | `D2` (pulso no botão de power) |
| AP de Configuração | `AXII_POWERLINK` / senha: `12345678` |
| OTA Hostname | `AXII Power Link` |
| OTA Senha | `admin123` |
| Duração do Pulso | 500ms |
| Intervalo de Consulta | 5 segundos |

**Lógica de operação:**
- Consulta a API a cada 5 segundos com `GET {apiBaseUrl}?id=6`
- Armazena o estado inicial na primeira leitura
- Quando detecta uma **mudança de estado** (`online → offline` ou `offline → online`) → envia um pulso de 500ms no pino D2
- O pulso simula o pressionamento físico do botão de power do PC
- Atualiza `ultima_conexao` na API após cada acionamento

**Arquivos de hardware:**
- `circuito.png` — diagrama do circuito
- `Power link.png` — foto/referência visual do módulo

---

### Nexus — Central de Controle Manual

| Propriedade | Valor |
|---|---|
| Arquivo | `Nexus/wemos-nexus.ino` |
| Pinos GPIO | `D5`, `D6`, `D7`, `D8` (botões) |
| AP de Configuração | `NEXUS_CONFIG` / senha: `12345678` |
| OTA Hostname | `ESP8266-NEXUS` |
| OTA Senha | `admin` |
| Debounce | 200ms |

**Mapeamento de botões:**

| Botão | Pino | ID Dispositivo | Equipamento |
|---|---|---|---|
| Botão 1 | `D5` | 7 | PC / Lab |
| Botão 2 | `D6` | 6 | Ar-condicionado |
| Botão 3 | `D7` | 4 | Projetor |
| Botão 4 | `D8` | 5 | Iluminação |

**Lógica de operação:**
- Ao pressionar um botão, consulta o status atual do dispositivo na API
- Alterna o status: `online → offline` ou `offline → online`
- Envia o novo status via `POST` à API
- LED embutido pisca ao pressionar um botão
- **Reset manual:** segure o botão D5 por 3 segundos para apagar as credenciais WiFi salvas

**Arquivos de hardware:**
- `circuit.pdf` — diagrama do circuito elétrico
- `nexus.png` — foto/referência visual do módulo
- `model 3d/AXII Nexus.stl` — modelo 3D para impressão da carcaça
- `model 3d/AXII Nexus.glb` — modelo 3D em formato GLB (visualização)

---

## Funcionalidades Comuns

Todos os três módulos compartilham as seguintes funcionalidades:

| Funcionalidade | Descrição |
|---|---|
| **Portal de Configuração WiFi** | Ao ligar sem credenciais salvas, cria um ponto de acesso WiFi próprio. O usuário conecta e acessa `http://192.168.4.1` para configurar a rede via navegador |
| **Persistência em EEPROM** | As credenciais WiFi são salvas na memória interna do ESP8266 e sobrevivem a reinicializações |
| **Reconexão automática** | Se o WiFi cair, tenta reconectar automaticamente. Após 5 falhas consecutivas, entra novamente em modo de configuração |
| **OTA (Over-the-Air)** | Atualização de firmware via rede WiFi sem precisar conectar o cabo USB |
| **Monitor Serial** | Logs detalhados via porta serial a 115200 baud para depuração |
| **Limpeza de configuração** | Interface web com botão para apagar credenciais e retornar ao modo de configuração |

---

## Integração com a API

Todos os módulos se comunicam com a API via HTTP. A URL base deve ser configurada na constante `apiBaseUrl` em cada arquivo `.ino`.

### Endpoints utilizados

| Método | Chamada | Descrição |
|---|---|---|
| `GET` | `{apiBaseUrl}?id={idDispositivo}` | Consulta o status atual do dispositivo |
| `GET` | `{apiBaseUrl}?action=update&id={idDispositivo}` | Atualiza o campo `ultima_conexao` |
| `POST` | `{apiBaseUrl}` com body JSON | Alterna o status do dispositivo (Nexus) |

### Formatos de resposta suportados

Os módulos aceitam dois formatos de resposta da API:

```json
// Formato 1
{
  "success": true,
  "data": {
    "id": 7,
    "nome": "Iluminação Sala A",
    "status": "online",
    "ativo": true
  }
}

// Formato 2 (resposta direta)
{
  "id": 7,
  "nome": "Iluminação Sala A",
  "status": "online",
  "ativo": true
}
```

### Payload de atualização de status (Nexus)

```json
POST {apiBaseUrl}
Content-Type: application/json

{
  "id": 7,
  "status": "offline"
}
```

---

## Requisitos de Hardware

### Componentes comuns (todos os módulos)
- **Microcontrolador:** ESP8266 — WeMos D1 Mini (ou equivalente)
- **Alimentação:** 5V via USB
- **Rede WiFi:** 2,4 GHz (o ESP8266 não suporta 5 GHz)

### Componentes por módulo

| Módulo | Hardware adicional |
|---|---|
| **AX-LM** | 1× módulo relé (5V), fiação, luminária |
| **Power Link** | Fios de conexão ao botão de power do PC; optoacoplador ou relé recomendado para isolação elétrica |
| **Nexus** | 4× push buttons, LED embutido no ESP8266, carcaça impressa em 3D (opcional — arquivo STL incluso) |

---

## Bibliotecas Arduino Necessárias

| Biblioteca | Origem | Uso |
|---|---|---|
| `ESP8266WiFi` | ESP8266 Core | Conexão WiFi |
| `ESP8266WebServer` | ESP8266 Core | Portal de configuração captive |
| `ESP8266HTTPClient` | ESP8266 Core | Requisições HTTP à API |
| `ArduinoJson` | Benoit Blanchon (v6.x) | Parsing das respostas JSON da API |
| `ArduinoOTA` | ESP8266 Core | Atualização de firmware via rede |
| `EEPROM` | ESP8266 Core | Persistência das credenciais WiFi |

---

## Como Programar

### 1. Instalar o Arduino IDE

Baixe o [Arduino IDE](https://www.arduino.cc/en/software) (versão 1.8.x ou 2.x).

### 2. Adicionar suporte ao ESP8266

1. Abra o Arduino IDE → **Arquivo** → **Preferências**
2. Em "URLs adicionais para gerenciadores de placas", adicione:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Vá em **Ferramentas** → **Placa** → **Gerenciador de Placas**
4. Pesquise `esp8266` e instale

### 3. Instalar bibliotecas

1. Vá em **Sketch** → **Incluir Biblioteca** → **Gerenciar Bibliotecas**
2. Pesquise e instale: **ArduinoJson** (versão 6.x, autor: Benoit Blanchon)
3. As demais bibliotecas vêm com o ESP8266 Core

### 4. Configurar o firmware

Abra o arquivo `.ino` do módulo desejado e configure:

```cpp
// URL da API (substitua pelo endereço real)
const char* apiBaseUrl = "http://seu-servidor.com/tcc-axii/Project-Axii-api/api/devices/list.php";

// ID do dispositivo no banco de dados (verifique na API)
const int idDispositivo = 7;
```

> As credenciais WiFi **não precisam** ser inseridas no código — elas são configuradas pelo portal web após a primeira inicialização.

### 5. Fazer upload

1. Conecte o ESP8266 via USB ao computador
2. Selecione a placa: **Ferramentas** → **Placa** → **ESP8266 Boards** → **WeMos D1 R1**
3. Selecione a porta COM correta em **Ferramentas** → **Porta**
4. Clique em **Upload** (→) e aguarde a compilação e gravação

---

## Configuração WiFi no Dispositivo

Após o upload, na primeira inicialização (ou após limpar as credenciais):

1. O módulo cria uma rede WiFi própria (ex: `AX-LM_CONFIG`)
2. Conecte seu celular ou computador a essa rede (senha: `12345678`)
3. Acesse `http://192.168.4.1` no navegador
4. Preencha o SSID e a senha da sua rede WiFi da escola
5. Clique em **Conectar à Rede** — o dispositivo reinicia e entra em operação normal

---

## Atualização OTA (Over-the-Air)

Após a configuração inicial, os módulos podem ser atualizados sem cabo USB:

1. Certifique-se de que o dispositivo está ligado e na mesma rede
2. No Arduino IDE, vá em **Ferramentas** → **Porta** e selecione o dispositivo na rede
3. Digite a senha OTA quando solicitado
4. Faça o upload normalmente

**Senhas e hostnames OTA:**

| Módulo | Hostname OTA | Senha OTA |
|---|---|---|
| AX-LM | `AX-LM` | `admin123` |
| Power Link | `AXII Power Link` | `admin123` |
| Nexus | `ESP8266-NEXUS` | `admin` |

---

## Estrutura do Repositório

```
Project-axii-embedded/
├── AX-LM/
│   ├── wemos_lamp.ino        # Firmware do controlador de iluminação
│   └── circuito.pdf          # Diagrama do circuito elétrico
│
├── Power Link/
│   ├── wemos_pc.ino          # Firmware do controlador de energia dos PCs
│   ├── circuito.png          # Diagrama do circuito
│   └── Power link.png        # Referência visual do módulo
│
├── Nexus/
│   ├── wemos-nexus.ino       # Firmware da central de controle manual
│   ├── circuit.pdf           # Diagrama do circuito elétrico
│   ├── nexus.png             # Foto/referência visual do módulo
│   └── model 3d/
│       ├── AXII Nexus.stl    # Modelo 3D para impressão da carcaça
│       ├── AXII Nexus.glb    # Modelo 3D para visualização
│       └── AXII Nexus.zip    # Arquivos do modelo compactados
│
└── LICENSE
```

---

## Depuração (Serial Monitor)

Conecte o cabo USB e abra o Monitor Serial do Arduino IDE em **115200 baud** para ver os logs em tempo real:

```
========================================
    AXII - Controle de AX-LM
   com Configuração WiFi Integrada
========================================

Credenciais encontradas:
SSID: MinhaRedeEscola
Conectando em: MinhaRedaEscola
...............
 WiFi conectado!
IP: 192.168.1.105
Sistema pronto para uso!

--- Consultando Dispositivo ---
URL: http://servidor.com/api/devices/list.php?id=7
Código HTTP: 200
Status: offline | Ativo: SIM
>>> RELÉ LIGADO - AX-LM ACESA <<<
```

---

## Avisos de Segurança

> **Atenção:** O módulo **AX-LM** pode estar conectado à rede elétrica (iluminação de 127V/220V). Sempre utilize um relé adequado com isolação elétrica e, se necessário, consulte um eletricista. Nunca manipule fiação elétrica com o circuito energizado.

> O módulo **Power Link** é conectado ao botão de power do PC. Recomenda-se o uso de um optoacoplador para isolar eletricamente o ESP8266 do computador.

> Altere as senhas OTA padrão (`admin123` / `admin`) antes de usar em ambiente de produção.

---

## 📄 Licença

Este projeto está licenciado sob a [Licença MIT](LICENSE).
