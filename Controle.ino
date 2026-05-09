#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "paginas.h"
#include "esp_timer.h"

#define BIT_TIME 10000
#define SIZE 4096

//----------  Variáveis globais  -----------------

volatile int vemDado = 0;
volatile int caracbit = 0;
volatile int bites[8]={0,0,0,0,0,0,0,0};
volatile int nextCaracterCount = 0;
volatile int bitEsperadoNoPareamento = 3;

volatile bool prontoParaReceber = false;
volatile bool prontoParaEnviar = false;
volatile bool interrupcao = false;

volatile char caracterParaEnvio = 'U';

volatile unsigned long timePassed = 0;

hw_timer_t *recpTimer = NULL;
hw_timer_t *envTimer = NULL;

// --- Estrutura de dados (Chunklist) ---

typedef struct node
{
  char dado[SIZE];
  short paro;
  struct node* next;

}Chunklist;

Chunklist* create();
Chunklist* newnode();
Chunklist* addinfo(Chunklist* lista, char* palavra);
void destroylist(Chunklist* lista);
char nextcaracter(Chunklist* lista);

//----------------------------------------

Chunklist* create() { return NULL; }
Chunklist* lista = create();

Chunklist* newnode ()
{
  Chunklist* no= (Chunklist*)malloc(sizeof(Chunklist));
  no->paro=0;
  no->next=NULL;

  return no;
}

Chunklist* addinfo (Chunklist* lista, volatile char c) 
{
  int i;
  if (lista == NULL)
  {
    lista = newnode();
    lista->dado[0] = c; // Grava o caractere direto
    lista->paro = 1;
  }
  else
  {
    Chunklist* t = lista;
    // Navega até o último nó que tenha espaço
    while (t->next != NULL && t->paro >= SIZE)
    {
       t = t->next;
    }

    if (t->paro < SIZE)
    {
      t->dado[t->paro] = c;
      t->paro++;
    }
    else
    {
      t->next = newnode();
      t = t->next;
      t->dado[0] = c;
      t->paro = 1;
    }
  }
  return lista;
}

void destroylist (Chunklist* lista)
{
  Chunklist* t = lista;
  while (lista != NULL)
  {
    t=t->next;
    free(lista);
    lista=t;
  }
}

char nextcaracter(Chunklist* lista)
{
  if ((nextCaracterCount < SIZE) && (nextCaracterCount < lista->paro))
    return (lista->dado[nextCaracterCount++]);
  else if ((nextCaracterCount != SIZE) && (nextCaracterCount == lista->paro))
  {
    nextCaracterCount = 0;
    return (0); //loop das palavras lista->dado[nextCaracterCount]
  }  
  else
  {
    Chunklist *t = lista;
    int i;
    for (i=(nextCaracterCount/SIZE);i>0;i--)
    {
      t=t->next;
    }

    if (t == NULL)
    {
      nextCaracterCount = 0;
      return (0); //loop das palavraslista->dado[nextCaracterCount]
    }
    
    else
    {
      int pos = nextCaracterCount%SIZE;
      nextCaracterCount++;
      return lista->dado[pos];
    }
  }
}

/*-------------------------------------------------------*/

void desativarPinoOut(int gpio) {pinMode(9, INPUT);}

/*------------------  AttachInterrupts  ------------------------*/

void IRAM_ATTR detectouTentativaPareamento() {interrupcao=true;}
void IRAM_ATTR processoPareamento() {interrupcao=true;}

// ----------------- Controle das páginas -------------------
enum ModoOperacao { IDLE, ENVIO, ENVIANDO, RECEPCAO, RECEBIDO, ALINHAMENTO };
ModoOperacao estadoAtual = IDLE;

String getModoTexto() {
  switch (estadoAtual) {
    case ENVIO:       return "MODO ENVIO";
    case ENVIANDO:    return "MODO ENVIO";
    case RECEPCAO:    return "MODO RECEPÇÃO";
    case RECEBIDO:    return "MODO RECEPÇÃO";
    case ALINHAMENTO: return "MODO ALINHAMENTO";
    default:          return "AGUARDANDO SELEÇÃO";
  }
}

// Substitui os placeholders %TAG% no HTML
String processor(const String& var) {
  if (var == "MODO_ATUAL") return getModoTexto();
  return String();
}

AsyncWebServer server(80);

//----------------- DEFINIÇÃO DE ROTAS ----------------------

void rotasServidor ()
{
  // Rota Raiz (Painel)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Rota para atualização de status em tempo real
  server.on("/status-atual", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", getModoTexto());
  });

  // LOGICA: ENVIO
  server.on("/set-envio", HTTP_GET, [](AsyncWebServerRequest *request){
    estadoAtual = ENVIO;
    request->redirect("/envio-page");
  });

  server.on("/envio-page", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", envio_html);
  });

  // Recebe 1 char por vez
  server.on("/receber-char", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("c")) 
    {
        String c = request->getParam("c")->value();
        char cRecebido = c[0];  
           
        if (c == "EOF") 
        {
            pinMode(D6, OUTPUT);
            GPIO.out_w1ts = (1 << 9); //HIGH
            xTaskCreatePinnedToCore(prepareToSend, "prepareToSend", 4000, NULL, 1, NULL, 1); //task associada ao core 1
        } 
        else 
        {
            lista = addinfo (lista, cRecebido);
        }
          
        request->send(200, "text/plain", "OK"); // Responde para o próximo caracter vir
    } 
    else 
    {
        request->send(400, "text/plain", "Faltou o char");
    }
  });

  // LOGICA: ALINHAMENTO
  server.on("/set-alinhamento", HTTP_GET, [](AsyncWebServerRequest *request){
    estadoAtual = ALINHAMENTO;
    request->send_P(200, "text/html", placeholder_html, processor);
  });

  // LOGICA: RECEPÇÃO (Prepara dados e redireciona)
  server.on("/set-recepcao", HTTP_GET, [](AsyncWebServerRequest *request){
    estadoAtual = RECEPCAO;
    request->redirect("/recepcao-page");
    attachInterrupt(digitalPinToInterrupt(D4), detectouTentativaPareamento, RISING);
    xTaskCreatePinnedToCore(pairingManager, "PairingManager", 4000, NULL, 1, NULL, 1); //task associada ao core 1
  });

  server.on("/recepcao-page", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", recepcao_html);
  });

  // Entrega de 1 char por vez
  server.on("/proximo-char", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response;
    if (estadoAtual != RECEBIDO) {
      response = request->beginResponse(200, "text/plain", "IDLE");
    } 
    else if (estadoAtual == RECEBIDO) {
      char c = nextcaracter(lista);
      if (c != 0)
        response = request->beginResponse(200, "text/plain", String(c));
      else
      { 
        estadoAtual = IDLE;
        response = request->beginResponse(200, "text/plain", "EOF");
      }
    }
    //impede cache
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    request->send(response);
  });

  // VOLTAR: Reseta flag e redireciona
  server.on("/voltar", HTTP_GET, [](AsyncWebServerRequest *request){
    estadoAtual = IDLE;
    request->redirect("/");
  });

}

/*-----------------------  Envio/Recepção  ----------------------------------*/

void interromperTimer() 
{
  if (envTimer != NULL) 
  {
    timerAlarmDisable(envTimer); 
    timerStop(envTimer);
    desativarPinoOut(9);            
  }
}

volatile char convertchar(volatile int bits[8]) 
{
  char resultado = 0;
    
  for (int i = 7; i >= 0; i--) {
    resultado = resultado<<1;
    if (bits[i] == 1)
      resultado = resultado | bits[i]; 
  }

  if (resultado == 0)
    estadoAtual = RECEBIDO;

  return resultado;
}

// O processador vem para cá toda vez que o timer do receptor "tocar"
void IRAM_ATTR onRecpTimer() 
{

  if (!prontoParaReceber)
  {
    timerAlarmWrite(recpTimer, BIT_TIME, true);
    bool bite = GPIO.in & (1<<7);
    if (bite)
      bites[caracbit]=1;
    else
      bites[caracbit]=0;

    if (bite != bitEsperadoNoPareamento%2)
      timerAlarmWrite(recpTimer, BIT_TIME*1.25, true);

    bitEsperadoNoPareamento++;
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      prontoParaReceber=true;
      timerAlarmWrite(recpTimer, BIT_TIME, true);
    }
  }
  else
  {
    bool bite = GPIO.in & (1<<7);
  
    if (bite)
      bites[caracbit]=1;
    else
      bites[caracbit]=0;
  
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      lista=addinfo(lista,convertchar(bites));
    }
  }
}

void IRAM_ATTR onEnvTimer() {

  if (!prontoParaEnviar)
  {
    timerAlarmWrite(envTimer, BIT_TIME, true);
    
    if ((caracterParaEnvio >> caracbit) & 1)
      GPIO.out_w1ts = (1 << 9); //HIGH
    else
      GPIO.out_w1tc = (1 << 9); //LOW
        
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      prontoParaEnviar = true;
      caracterParaEnvio = nextcaracter(lista); 
    }
  }
  else
  {
    int bite = (caracterParaEnvio >> caracbit) & 1;
    
    if (bite)
      GPIO.out_w1ts = (1 << 9); //HIGH
    else
      GPIO.out_w1tc = (1 << 9); //LOW
    
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      caracterParaEnvio = nextcaracter(lista); 
      if (caracterParaEnvio == 0)
        interromperTimer();
    }
  }
}

/*-----------------------  Setup  --------------------------------*/

void setup() 
{
  Serial.begin(115200);
  delay(2000);

  pinMode(D4, INPUT);
  
  // Configura o ESP32 como Access Point, IP: 192.168.4.1
  WiFi.softAP("ESP32_NANO_AP", "12345678");
  
  rotasServidor();
  server.begin();
}

//---------------------  Core Tasks  -------------------------------

void configureTimerRecepcao(void * pvParameters) 
{  
  recpTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(recpTimer, &onRecpTimer, true);
  timerAlarmWrite(recpTimer, (BIT_TIME*2.5), true);
  timerAlarmEnable(recpTimer);

  vTaskDelete(NULL);
}

void pairingManager (void * pvParameters)
{
  for(;;)
  {
    if (interrupcao)
    {
      interrupcao=false;
      if (vemDado == 0)
      {
        if (micros()-timePassed > 250000)
        {
          detachInterrupt(digitalPinToInterrupt(D4));
          vemDado=1;
          attachInterrupt(digitalPinToInterrupt(D4), processoPareamento, FALLING);
        }
      }
      else if (vemDado == 1)
      {
        if (micros()-timePassed > 250000)
        {
          detachInterrupt(digitalPinToInterrupt(D4));
          vemDado=2;
          interrupcao=true;
          xTaskCreatePinnedToCore(configureTimerRecepcao, "ConfigTimerRecp", 2000, NULL, 1, NULL, 1); //task associada ao core 1
          vTaskDelete(NULL);
        }
      }
      timePassed = micros();
    }
  } 
}

void prepareToSend(void * pvParameters)
{
  vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1000ms (1 segundo)

  GPIO.out_w1tc = (1 << 9); // LOW

  // Configuração do Timer
  envTimer = timerBegin(1, 80, true);
  timerAttachInterrupt(envTimer, &onEnvTimer, true);
  timerAlarmWrite(envTimer, BIT_TIME * 2, true);
  timerAlarmEnable(envTimer);
  
  vTaskDelete(NULL);
}

//------------------- loop  --------------------------------

void loop() 
{
  //empty
}
