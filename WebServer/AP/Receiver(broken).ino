#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "ESPAsyncWebServer.h"
#include "paginaHTML.h" 

#define BIT_TIME 10000

const char *ssid = "NanoESP32_AP";
const char *password = "12345678";
AsyncWebServer server(80); //IP: 192.168.4.1

String textoArmazenado = "Nenhum dado recebido ainda.";
String textoArmazenado2 = "Servidor conectado"; 

//Variáveis globais
volatile int caracbit = 0;
volatile int bites[8]={0,0,0,0,0,0,0,0};
volatile char caracter = '*';
volatile char caracters[1];
volatile int caraccounter = 0;
volatile int parear = 3;
volatile bool parear0 = false;
volatile bool parear1 = false;
volatile bool parear2 = false;
volatile int recebidos = 0;
volatile bool dadosProntos = false;

hw_timer_t *timer = NULL;

/*-----------------------Struct--------------------------*/

typedef struct node
{
  char dado[4096];
  short paro;
  struct node* next;

}Chunklist;

Chunklist* create() { return NULL; }
Chunklist* lista = create();

Chunklist* newnode ()
{
  Chunklist* no= (Chunklist*)malloc(sizeof(Chunklist));
  no->paro=0;
  no->next=NULL;

  return no;
}

// Mude de (Chunklist* lista, volatile char* palavra) para:
Chunklist* addinfo (Chunklist* lista, char c) 
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
    while (t->next != NULL && t->paro >= 4096)
    {
       t = t->next;
    }

    if (t->paro < 4096)
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

void nextcaracter (Chunklist* lista) 
{
  if ((caraccounter < 4096) && (caraccounter < lista->paro))
    caracter = lista->dado[caraccounter];
  else if ((caraccounter != 4096) && (caraccounter == lista->paro))
  {
    caraccounter = 0;
    caracter = lista->dado[caraccounter]; //loop das palavras
  }  
  else
  {
    Chunklist *t = lista;
    int i;
    for (i=(caraccounter/4096);i>0;i--)
    {
      t=t->next;
    }

    if (t == NULL)
    {
      caraccounter = 0;
      caracter = lista->dado[caraccounter]; //loop das palavras
    }
    
    else
    {
      int pos = caraccounter%4096;
      caracter = lista->dado[pos];
    }
  }
}

/*-------------------------------------------------------------*/

char convertchar(volatile int bits[8]) {
    char resultado = 0;
    for (int i = 7; i >= 0; i--) {
        resultado = (resultado << 1);
        if (bits[i] == 1) resultado = resultado | 1; 
    }
    return resultado;
}

void interromperTimer() {
    if (timer != NULL)
      timerAlarmDisable(timer);
}

// O processador vem para cá toda vez que o timer "tocar"
void IRAM_ATTR onTimer() {

  if (!parear2)
  {
    timerAlarmWrite(timer, BIT_TIME, true);
    int bite = digitalRead (D4);
    if (bite)
      bites[caracbit]=1;
    else
      bites[caracbit]=0;

    if (bite != parear%2)
      timerAlarmWrite(timer, BIT_TIME*1.25, true);

    parear++;
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      parear2=true;
      timerAlarmWrite(timer, BIT_TIME, true);
    }
  }
  else
  {
    int bite = digitalRead (D4);
  
    if (bite)
      bites[caracbit]=1;
    else
      bites[caracbit]=0;
  
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      char c = convertchar(bites);
      lista=addinfo(lista,c); 
      recebidos++;
      
      if (recebidos >= 27)
      {
        dadosProntos=true;
        interromperTimer();
      }
    }
  }
}

/*-------------------------------------------------------*/

void IRAM_ATTR detectouLaser2() 
{
  parear1=true;
  detachInterrupt(digitalPinToInterrupt(D4));
  textoArmazenado2 = "Pareado";
  xTaskCreatePinnedToCore(configureTimer, "ConfigTimer", 2000, NULL, 1, NULL, 1); //task associada ao core 1
}

void IRAM_ATTR detectouLaser() 
{
  parear0=true;
  detachInterrupt(digitalPinToInterrupt(D4));
  textoArmazenado2 = "Quase lá!";
  delay(250);
  attachInterrupt(digitalPinToInterrupt(D4), detectouLaser2, FALLING);
}

/*-------------------------------------------------------*/

String processor(const String& var) {
  if (var == "STATUS") {
    return textoArmazenado2; 
  }
  if (var == "TEXTO_AQUI") {
    return "Carregando chunks da memória...";
  }
  return String();
}

void setup() 
{
  Serial.begin(115200);
  
  WiFi.softAP(ssid, password);
  
  // Envia o HTML
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", INDEX_HTML);
});

// Envia apenas o valor de textoArmazenado2 em tempo real
server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) 
{
  request->send(200, "text/plain", textoArmazenado2);
});

// Envia a lista encadeada 
server.on("/api/lista", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (dadosProntos) 
  {
    request->sendChunked("text/plain", [=](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      
      if (index >= recebidos) return 0;

      Chunklist* noAtual = lista;
      int tempIndex = index;

      while (tempIndex >= 4096 && noAtual != NULL) 
      {
        noAtual = noAtual->next;
        tempIndex -= 4096;
      }

      size_t i = 0;
      while (i < maxLen && (index + i) < recebidos && noAtual != NULL) 
      {
        buffer[i] = (uint8_t)noAtual->dado[tempIndex];
        
        i++;
        tempIndex++;

        
        if (tempIndex >= 4096) {
          tempIndex = 0;
          noAtual = noAtual->next;
        }
      }
      return i;
    });
  } 
  else 
  {
    request->send(200, "text/plain", "Aguardando dados...");
  }
});

  server.begin();
  Serial.println ("Conectado");
  pinMode(D4, INPUT);
  delay(2000);
  
  caracbit=0;
  attachInterrupt(digitalPinToInterrupt(D4), detectouLaser, RISING);
}

void configureTimer(void * pvParameters) 
{  
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, (BIT_TIME*2.5), true);
    timerAlarmEnable(timer);

    vTaskDelete(NULL);
}
  
void loop() {
  //empty
}
