const char INDEX_HTML[] PROGMEM = R"=====(
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
          background: #f4f4f4; 
          padding: 20px; 
         }
        
        .card 
        { 
          background: white; 
          padding: 20px; 
          border-radius: 10px; 
          box-shadow: 0 2px 5px rgba(0,0,0,0.1); 
          display: inline-block; 
          min-width: 300px;     
        }
        
        input[type="text"] 
        { 
          padding: 10px; 
          width: 80%; 
          margin: 10px 0; 
          border: 1px solid #ccc; 
          border-radius: 5px; 
        }
        
        input[type="submit"] 
        { 
          padding: 10px 20px; 
          background: #007BFF; 
          color: white; 
          border: none; 
          border-radius: 5px; 
          cursor: pointer; 
        }
        
        .btn 
        { 
          display: inline-block; 
          padding: 15px 25px; 
          margin: 10px; 
          color: white; 
          text-decoration: none; 
          border-radius: 5px; 
          font-weight: bold; 
        }
        
        .on 
        { 
          background: #28a745; 
        } 
        .off 
        { 
          background: #dc3545; 
        }
        .status 
        { 
          color: #555; 
          margin-bottom: 20px; 
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Controle ESP32</h1>
        
        <div class="status">
            Texto Atual: <br>
            <strong style="color: #007BFF; font-size: 1.2em;">%TEXTO_SALVO%</strong>
        </div>

        <form action="/salvar" method="get">
            <input type="text" name="msg" placeholder="Escreva algo aqui...">
            <br>
            <input type="submit" value="Salvar no Dispositivo">
        </form>

        <hr style="margin: 20px 0; border: 0; border-top: 1px solid #eee;">
        
        <a href="/H" class="btn on">LIGAR LED</a>
        <a href="/L" class="btn off">DESLIGAR LED</a>
    </div>
</body>
</html>
)=====";
