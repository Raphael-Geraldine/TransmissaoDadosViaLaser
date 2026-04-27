#include <Arduino.h>
#include "esp_timer.h"
#define BIT_TIME 10000

//Variáveis globais
volatile char caracter = '*';
volatile int caracbit = 0;
volatile int caraccounter = 0;
volatile int iparear = 0;
volatile bool parear = false;
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
Chunklist* pareamento = create(); 

Chunklist* newnode ()
{
  Chunklist* no= (Chunklist*)malloc(sizeof(Chunklist));
  no->paro=0;
  no->next=NULL;

  return no;
}

Chunklist* addinfo (Chunklist* lista, const char* palavra)
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

/*-------------------------------------------------------*/

void IRAM_ATTR onTimer() {

  if (!parear)
  {
    timerAlarmWrite(timer, BIT_TIME, true);
    if (iparear < 3)
    {
      if ((caracter >> caracbit) & 1)
        digitalWrite(D6, HIGH);
      else
        digitalWrite(D6, LOW);
        
      caracbit++;
      if (caracbit > 7)
      {
        caracbit = 0;
        iparear++;
        nextcaracter(pareamento); 
      }
    }

    if (iparear == 3)
    {
      nextcaracter(lista);
      parear = true;
    }
  }
  else
  {
    int bite = (caracter >> caracbit) & 1;
    
    if (bite)
      digitalWrite(D6, HIGH);
    else
      digitalWrite(D6, LOW);
    
    caracbit++;
    if (caracbit > 7)
    {
      caracbit = 0;
      caraccounter++;
      nextcaracter(lista); 
    }
  }
}

void setup() {
  pinMode(D6, OUTPUT);
  delay(1000);

  pareamento = addinfo(pareamento,"UUU"); //01010101
  nextcaracter(pareamento);
  
  for (int i=0; i<2; i++)
    lista = addinfo(lista,"Hello World!");
  
  delay(4000);

  digitalWrite(D6, HIGH);
  delay(1000);
  digitalWrite(D6, LOW);

  setCpuFrequencyMhz(240);
  xTaskCreatePinnedToCore(configureTimer, "ConfigTimer", 2000, NULL, 1, NULL, 1); //task associada ao core 1
}

void configureTimer(void * pvParameters) { 
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, BIT_TIME*2, true);
    timerAlarmEnable(timer);

    vTaskDelete(NULL);
} 

void loop() {
  //empty
}
