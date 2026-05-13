#ifndef PAGINAS_H
#define PAGINAS_H

#include <Arduino.h>

// --- PÁGINA INICIAL (INDEX) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Nano - Painel</title>
    <style>
        body 
        { 
          font-family: sans-serif; 
          text-align: center; 
          background: #cecece; 
          padding: 20px; 
        }
        .card 
        { 
          background: rgb(232, 230, 230); 
          outline: 2.5px solid blue; 
          padding: 20px; 
          border-radius: 40px; 
          box-shadow: 0 2px 5px rgba(0,0,0,0.1); 
          display: inline-block; 
          min-width: 300px; 
          max-width: 700px; 
        }
        .btn 
        { 
          display: inline-block; 
          padding: 15px 25px; 
          margin: 10px; 
          margin-top: 40px; 
          color: white; 
          text-decoration: none; 
          border-radius: 10px; 
          font-weight: bold; 
          transition: background-color 0.5s ease; 
        }  
        .send 
        { 
          background: #28a745; 
        } 
        .send:hover 
        { 
          background-color: #1c7e33; 
        }
        .receive 
        { 
          background: #dc3545;
        } 
        .receive:hover 
        { 
          background-color: #9b222e; 
        }
        .align 
        { 
          display: block; 
          padding: 15px 25px; 
          max-width: 300px; 
          margin: 20px auto 0px auto; 
          color: white; 
          background: #082cbe; 
        }
        .align:hover 
        { 
          background-color: #081f79; 
        } 
        .status 
        { 
          color: #161616; 
          margin-top: 15px; 
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Escolha o modo de operação</h1>
        <div class="status">
            Status Atual: <br>
            <strong id="txt-status" style="color: #007BFF; font-size: 1.2em;">%MODO_ATUAL%</strong>
        </div>
        <a href="/set-envio" class="btn send">Envio de dados</a>
        <a href="/set-recepcao" class="btn receive">Recepção de dados</a>
        <a href="/set-alinhamento" class="btn align">Alinhar as bases</a>
    </div>

    <script>
        // Atualiza o texto do status a cada 1 segundo (Real-time)
        setInterval(function() {
            fetch('/status-atual')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('txt-status').innerText = data;
                });
        }, 1000);
    </script>
</body>
</html>
)rawliteral";

// --- PÁGINA DE RECEPÇÃO ---
const char recepcao_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body 
    { 
      font-family: sans-serif; 
      margin: 20px; text-align: 
      center; background: #f0f0f0; 
    }
    .caixa 
    { 
      border: 2px solid #333; 
      padding: 15px; 
      background: #fff; 
      white-space: pre-wrap; 
      height: 350px; 
      overflow-y: auto; 
      text-align: left; 
      border-radius: 10px; 
      font-family: monospace; 
    }
    #status-val 
    { 
      font-weight: bold; 
      color: blue; 
    }
    .btn-voltar 
    { 
      display: inline-block; 
      margin-top: 20px; 
      padding: 12px 25px; 
      background: #6c757d; 
      color: white; 
      text-decoration: none; 
      border-radius: 8px; 
      font-weight: bold; 
    }
    .btn-voltar:hover 
    { 
      background: #495057; 
    }
    .btn-ok 
    { 
      display: inline-block; 
      margin-top: 20px; 
      padding: 12px 25px; 
      background: #6c757d; 
      color: white; 
      text-decoration: none; 
      border-radius: 8px; 
      font-weight: bold; 
    }
    .btn-ok:hover 
    { 
      background: #495057; 
    }
  </style>
</head>
<body>
  <h1>Modo Recepção de Dados</h1>
  <p>Status: <span id="status-val">Aguardando dados...</span></p>
  <div class="caixa" id="alvo-da-lista"></div>
  <br>
  <a href="/voltar" class="btn-voltar">Voltar</a>
  <br>
  <a href="/set-recepcao" id="btn-ok" class="btn-ok">Limpar Caixa</a>

  <script>
    async function solicitarDados() 
    {
      const caixa = document.getElementById('alvo-da-lista');
      const status = document.getElementById('status-val');
      const btn = document.getElementById('btn-ok');
      let ativo = true;
      
      // Pequena pausa para garantir que o ESP32 trocou a flag
      await new Promise(r => setTimeout(r, 500));

      while(ativo) 
      {
        btn.disable=true;
        
        try 
        {
          // O Date.now() gera um número diferente a cada milissegundo (ALTERAR PARA CONTADOR!!!!!)
          const res = await fetch('/proximo-char?nocache=' + Date.now());
          const char = await res.text();

          if (char === "EOF") 
          {
            status.innerText = "Concluído";
            status.style.color = "green";
            btn.disable = false;
            ativo = false;
          } 
          else if (char === "IDLE") 
          {
             // Se o ESP ainda não estiver em modo recepção, espera um pouco
             await new Promise(r => setTimeout(r, 200));
          } 
          else 
          {
            caixa.innerText += char;
            caixa.scrollTop = caixa.scrollHeight;
          }
        } 
        catch (e) 
        {
          console.error("Erro na leitura");
          ativo = false;
        }
      }
    }
    window.onload = solicitarDados;
  </script>
</body>
</html>
)rawliteral";

// --- PÁGINA DE ENVIO ---
const char envio_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Envio</title>
</head>
<body>
    <h1>Modo Envio de Dados</h1>
    <textarea class="input" id="mensagem" placeholder="Digite aqui a mensagem a ser enviada..."></textarea>
    <br>
    <button class="btn-enviar" id="btn-submit" onclick="enviarDados()">Enviar</button> 
    <div id="status-envio"></div>
    <br>
    <a href="/voltar" class="btn-voltar">Voltar</a>

    <script>
        let contador = 0;

        async function enviarDados() {
            const caixa = document.getElementById('mensagem');
            const btn = document.getElementById('btn-submit');
            const status = document.getElementById('status-envio');
            const texto = caixa.value;

            if (texto.length === 0) return;

            // Bloqueia interface durante o envio
            btn.disabled = true;
            caixa.disabled = true;
            
            for (let i = 0; i < texto.length; i++) 
            {
                const char = texto[i];
                status.innerText = `Processsando: ${Math.round(((i + 1)/texto.length)*100)}%`;
                
                try 
                {
                    // Enviamos o caractere e um contador para evitar cache
                    await fetch(`/receber-char?c=${encodeURIComponent(char)}&cnt=${contador++}`);
                } 
                catch (e) 
                {
                    console.error("Erro ao enviar char");
                    status.innerText = "Erro na conexão!";
                    break;
                }
            }

            // Sinaliza fim do envio
            await fetch(`/receber-char?c=EOF&cnt=${contador++}`);
            
            status.innerText = "Envio sendo realizado!";
            status.style.color = "green";
        }
    </script>
</body>
</html>
)rawliteral";

// --- PÁGINA PLACEHOLDER (ALINHAMENTO) ---
const char placeholder_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <meta charset="UTF-8">
</head>
<body>
    <h1>Modo: %MODO_ATUAL%</h1>
    <p>-</p>
    <a href="/voltar" style="padding: 15px 25px; background: #082cbe; color: white; text-decoration: none; border-radius: 10px;">Voltar ao Início</a>
</body>
</html>
)rawliteral";

#endif
