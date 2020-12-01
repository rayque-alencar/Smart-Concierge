#include <Stepper.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

//-----------------------------------------------------------------------------------------------------------------------------//

#define pin1 12            //pin para o sensor de temperatura e variáveis usadas na conta da temperatura.
int temp = 0;
double Voltage = 0;
double temperatura = 0;

#define CAMERA_MODEL_AI_THINKER      //qual o modelo da camera na esp32

#include "camera_pins.h"             // .h com as configurações da camera
#include "camera_code.h"

#define FLASH_LED_PIN 4              //pin do flash da camera
 
// INICIALIZAÇÃO DA REDE WI-FI
char ssid[] = "brisa-179251"; // NOME DA REDE WI-FI
char password[] = "z7v7rnv7"; // SENHA DA REDE WI-FI

// INICIALIZAÇÃO DO TELEGRAM BOT
#define BOTtoken "1482642950:AAEmfjrAg_u2sT5p-Nk1dbSHNMjEIXYp1Yc"  // BOT TOKEN (FEITO NO Botfather)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs = 1000; //tempo médio entre varredura de mensagens,
long Bot_lasttime;   //última vez que a verificação de mensagens foi feita,

//-----------------------------------------------------------------------------------------------------------------------------//

//para o funcionamento da camera 

bool flashState = LOW; //estado do flash ao iniciar

camera_fb_t * fb = NULL;
      
bool dataAvailable = false;

bool isMoreDataAvailable(){
  if (dataAvailable){
    dataAvailable = false;
    return true;
  }else{
    return false;
  }
}

byte* getNextBuffer(){
  if (fb){
    return fb->buf;
  } else {
    return nullptr;
  }
}

int getNextBufferLen(){
  if (fb){
    return fb->len;
  } else {
    return 0;
  }
}

void Foto(String chat_id){           //função para tirar foto
  fb = NULL;
  
      //tirar uma foto com a camera
      fb = esp_camera_fb_get();
      
      if (!fb){
        Serial.println("A captura da câmera falhou");
        bot.sendMessage(chat_id, "A captura da câmera falhou", "");
        return;
      }
      
      dataAvailable = true;
      Serial.println("enviando");
      bot.sendPhotoByBinary(chat_id, "image/jpeg", fb->len, isMoreDataAvailable, nullptr, getNextBuffer, getNextBufferLen);

      Serial.println("pronto!");
      esp_camera_fb_return(fb); 
}

void Flash(bool i){                  //função para ligar o flash
  digitalWrite(FLASH_LED_PIN, i);
}



//-----------------------------------------------------------------------------------------------------------------------------//

//para o funcionamento do bot telegram

void novasMensagens(int NumNovasMensagens){     //função para novas mensagens, recebe ela e faz o que foi pedido nela
  Serial.println("novas mensagens");
  Serial.println(String(NumNovasMensagens));

  for (int i = 0; i < NumNovasMensagens; i++){
         
    String chat_id = String(bot.messages[i].chat_id);  //pega o chat_id do usuario
    String text = bot.messages[i].text;                //pega a mensagem do usuario 
    String from_name = bot.messages[i].from_name;      //para quem foi enviado

    if(from_name == "") from_name = "Guest";

    if(text == "/status"){
      Flash(true);
      Foto(chat_id);
      Flash(false);
           
      String texto = String(temperatura);
      texto += " °C\n";
      bot.sendMessage(chat_id, texto, "Markdown"); 

        if(temperatura >= 38){
           String texto = "Acesso negado, você está com quadro febril, procure um médico";
           bot.sendMessage(chat_id, texto, "Markdown");   
        }else{
           String texto = "Acesso permitido";
           bot.sendMessage(chat_id, texto, "Markdown");  
        } 
    }
    
    if (text == "/start") {
      String texto = "Bem vindo ao Smart Concierge, Desenvolvido Por rayque alencar.\n\n";
      texto += "/status : Tira uma foto, e mede a temperatura da pessoa.\n\n";
      texto += "Meu GitHub: https://github.com/rayque-alencar";
      bot.sendMessage(chat_id, texto, "Markdown"); 
    }
  }   // fim do for das mensagens do bot do telegram
    
}

//-----------------------------------------------------------------------------------------------------------------------------//

//inicio da void setup 

void setup(){
  Serial.begin(115200);
  
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState); 

  if(!setupCamera()){
    Serial.println("Falha na configuração da câmera!");
    while(true){
      delay(100);
    }
  }

  //tentando conexão com a rede Wifi:
  Serial.print("Conectando Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
              
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  xTaskCreatePinnedToCore(loopcore2,"temp",10000,NULL,1,NULL,0);  //para fazer com o a temperatura seja avaliada ao mesmo tempo que a das mensagens do bot(dual core)
  delay(500);
   
  // Faz o bot esperar por uma nova mensagem por até 60 segundos
  bot.longPoll = 60;
  
}

//-----------------------------------------------------------------------------------------------------------------------------//

//Inicio dos loops

void loop(){                                                                   //loop para avaliar se tem mensagens no bot do telegram
  
  if(millis() > Bot_lasttime + Bot_mtbs){
    int NumNovasMensagens = bot.getUpdates(bot.last_message_received + 1);

    while(NumNovasMensagens){
      Serial.println("obteve resposta");
      novasMensagens(NumNovasMensagens);
      NumNovasMensagens = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}


void loopcore2(void* pvParameters){                                            //loop 2 para pegar o valor da temperatura no sensor, ao mesmo tempo das mensagens do telegram
      while(true){
      temp = analogRead(pin1);

     // Voltage = (temp / 4095.0) * 1100;   calculo para ter a temperatura correta, o meu não estar funcionando corentamente por causa sensor
     // temperatura = Voltage * 0.1;

      temperatura = random(34,40);          //por isso fiz um random para da valor a temperatura
   
      Serial.print("temperatura:");
      Serial.print(temperatura);
      Serial.println("°C\n");
   
      delay(25000);
    } 
 }
