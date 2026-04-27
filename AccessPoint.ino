#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "paginaHTML.h" // Inclui o seu arquivo HTML

const char *ssid = "NanoESP32_AP";
const char *password = "12345678";
WiFiServer server(80); //192.168.4.1

String textoArmazenado = "Nenhum dado salvo ainda."; // Variável que guarda o texto na RAM

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  while(!Serial){;}
  
  WiFi.softAP(ssid, password);
  server.begin();
  Serial.println("Servidor iniciado!");
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";
    String fullRequest = ""; 

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        fullRequest += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Analisa se o usuário enviou um novo texto
            if (fullRequest.indexOf("GET /salvar?msg=") >= 0) {
              int start = fullRequest.indexOf("msg=") + 4;
              int end = fullRequest.indexOf(" ", start);
              textoArmazenado = fullRequest.substring(start, end);
              textoArmazenado.replace("+", " "); // Corrige espaços
            }

            // Analisa os comandos do LED
            if (fullRequest.indexOf("GET /H") >= 0) digitalWrite(LED_BUILTIN, HIGH);
            if (fullRequest.indexOf("GET /L") >= 0) digitalWrite(LED_BUILTIN, LOW);

            // Envia a resposta HTTP
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();

            // Prepara o HTML do .h para envio
            String htmlDinamico = INDEX_HTML;
            // Substitui o marcador pelo valor da variável
            htmlDinamico.replace("%TEXTO_SALVO%", textoArmazenado);
            
            client.print(htmlDinamico);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}
