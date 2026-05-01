const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <style>
    body { font-family: sans-serif; margin: 20px; }
    .caixa { border: 1px solid #333; padding: 10px; background: #fafafa; white-space: pre-wrap; height: 300px; overflow-y: auto; }
    #status-val { font-weight: bold; color: blue; }
  </style>
</head>
<body>
  <h2>Status: <span id="status-val">Iniciando...</span></h2>
  <hr>
  <h3>Dados da Lista:</h3>
  <div class="caixa" id="alvo-da-lista">Aguardando processamento...</div>

  <script>
    let intervaloStatus = setInterval(atualizarStatus, 500); 
    let listaCarregada = false; // Flag para não carregar a lista várias vezes

    function atualizarStatus() {
      fetch('/api/status')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status-val').innerText = data;

          if ((data === "Pareado" || data === "Quase lá!") && !listaCarregada) {
            console.log("Dados detectados! Iniciando carga da lista...");
            carregarLista();
            listaCarregada = true; // Impede mais de 1 chamada
          }
          
          // Parar o loop de status após o parear
          if (data === "Pareado") {
             clearInterval(intervaloStatus);
          }
        })
        .catch(err => console.error("Erro ao buscar status:", err));
    }

    function carregarLista() {
      fetch('/api/lista')
        .then(response => response.text())
        .then(data => {
          if (data === "Aguardando dados...") {
            listaCarregada = false; 
            setTimeout(carregarLista, 1000);
          } else {
            document.getElementById('alvo-da-lista').innerText = data;
          }
        })
        .catch(err => {
            console.error("Erro ao carregar lista:", err);
            listaCarregada = false;
        });
    }

    window.onload = function() {
        atualizarStatus();
    };
  </script>
</body>
</html>)rawliteral";
