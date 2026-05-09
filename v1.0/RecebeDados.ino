#include <Arduino.h>
#include "esp_timer.h"
#define BIT_TIME 10000

//Variáveis globais
volatile int caracbit = 0;
volatile int bites[8]={0,0,0,0,0,0,0,0};
volatile char caracter[1];
volatile int parear = 3;
volatile bool parear0 = false;
volatile bool parear1 = false;
volatile bool parear2 = false;

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

Chunklist* addinfo (Chunklist* lista, volatile char* palavra)
{
  int i;
  if (lista == NULL)
  {
    lista = newnode();
    for (i=0; palavra[i]!='\0';i++)
      lista->dado[i]=palavra[i];
    lista->dado[i]=' ';
    lista->paro=i+1;
  }
  else
  {
    Chunklist* t = lista;
    while ((t->paro) == 4096)
    {
        if ( (t->next) != NULL)
          t=t->next;
        else
          break;
    }
    for (i=0; palavra[i]!='\0';i++)
    {
      if ((t->paro) < 4096)
      {
        t->dado[t->paro]=palavra[i];
        t->paro++;
      }
      else
      {
        t->next=newnode();
        t=t->next;
        t->dado[t->paro]=palavra[i];
        t->paro++;
      }
    }

    if ((t->paro) < 4096)
    {
      t->dado[t->paro]=' ';
      t->paro++;
    }
    else
    {
      t->next=newnode();
      t=t->next;
      t->dado[t->paro]=' ';
      t->paro++;
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

/*-------------------------------------------------------------*/

volatile char* convertchar(volatile int bits[8]) {
    char resultado = 0;

    for (int i = 7; i >= 0; i--) {
      resultado = resultado<<1;
      if (bits[i] == 1)
        resultado = resultado | bits[i]; 
    }

    caracter[0]= resultado;
    Serial.println (resultado);
    return caracter;
}

// O processador vem para cá toda vez que o timer "tocar"
void IRAM_ATTR onTimer() {

  if (!parear2)
  {
    timerAlarmWrite(timer, BIT_TIME, true);
    bool bite = GPIO.in & (1<<7);
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

/*-------------------------------------------------------*/

void IRAM_ATTR detectouLaser() {parear0=true;}
void IRAM_ATTR detectouLaser2() {parear1=true;}

/*-------------------------------------------------------*/

void setup() {
  Serial.begin(115200);
  delay(2000);
  while(!Serial) {;}
  Serial.println ("Conectado");
  pinMode(D4, INPUT);
  delay(2000);
  
  caracbit=0;
  attachInterrupt(digitalPinToInterrupt(D4), detectouLaser, RISING);

  while(!parear0) {;}

  Serial.println ("Quase lá!");
  delay(250);

  attachInterrupt(digitalPinToInterrupt(D4), detectouLaser2, FALLING);

  while(!parear1){;}

  Serial.println ("Pareado");
  xTaskCreatePinnedToCore(configureTimer, "ConfigTimer", 2000, NULL, 1, NULL, 1); //task associada ao core 1
}

void configureTimer(void * pvParameters) {  
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, (BIT_TIME*2.5), true);
    timerAlarmEnable(timer);

    vTaskDelete(NULL);
}
  
void loop() {
  //empty
}
