#include "DHT.h"
#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> // biblioteca do Wifi Manager
#include <DNSServer.h> //Servidor DNS
#include <ESP8266WebServer.h> //torna o ESP um webserver. Podemos acessar esse serviço na etapa de conexão com a rede wifi propriamente dita.

#define DHTPIN A1 // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11

float reading_seconds = 1;
int serial_port = 0;

float celsius;

//defines de id mqtt e tópicos para publicação e subscribe

#define TOPICO_SUBSCRIBE_P1 "aulacefet/led"     //nome do tópico - diferencia as aplicações. No caso, este tópico serve para envio de comandos
                                                //para acender e desligar um ledzinho.

         
#define ID_MQTT  "esp_vitoria"     //id mqtt (para identificação de sessão) - identificador para que o broker saiba qual o usuario, qual ESP está acessando o tópico
                               //IMPORTANTE: o id deve ser único no broker, pois 
                               //            se um client MQTT tentar entrar com o mesmo 
                               //            id de outro já conectado ao broker, o broker irá derrubar um deles.

#define USER_MQTT  "vitoria"   // usuario no MQTT
#define PASS_MQTT  "1234"  // senha no MQTT 

                               
 
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5 
#define D2    4 
#define D3    0 
#define D4    2 
#define D5    14  
#define D6    12 
#define D7    13 
#define D8    15 
#define D9    3  
#define D10   1  


                         
 
// WIFI
const char* SSID = "VITORIA"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "*"; // Senha da rede WI-FI que deseja se conectar
 
// MQTT
const char* BROKER_MQTT = "mqtt.eclipseprojects.io"; //URL do broker MQTT que se deseja utilizar - servidor broker
//O broker é um servidor. como este broker acima é publico, devemos ter cuidado com o uso dos tópicos. Devemos utilizar um tópico particular, senão
//se utilizarmos um tópico público, outros usuários irão sobrescrever as informações colocadas por nós.

int BROKER_PORT = 1883; // Porta do Broker MQTT
 
 
//Variáveis e objetos globais
WiFiClient espClient; //objeto referente ao nosso ESP. É um cliente WiFi. Objeto EspClient.
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

 
//Declarando o cabeçalho das funções para facilitar o compilador
void initSerial();
void initWiFi();
void initOTA();
void initMQTT();
void reconectWiFi(); 

//Funções do MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);

void InitOutput(void);
 
/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:
    initSerial();
    initWiFi();
    initOTA();
    initMQTT();
    InitOutput();

}
 

//Função: inicializa comunicação serial com baudrate 115200. O objetivo é monitorar o funcionamento do programa através da exibição
//de algumas informações no monitor serial
void initSerial() 
{
    Serial.begin(115200);
}
 
//Função: inicializar e conectar-se na rede WI-FI desejada
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    
    reconectWiFi();
}

//Função inicializa OTA - permite carga do novo programa via Wifi
void initOTA()
{
  Serial.println();
  Serial.println("Iniciando OTA....");
  ArduinoOTA.setHostname("pratica-4"); //nome da porta

  // No authentication by default
   ArduinoOTA.setPassword((const char *)"teste-ota"); // senha para carga via WiFi (OTA). Serve para evitar que qualquer pessoa conectada
                                                      // na rede possa escrever programas no esp8266.
                                                       
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}
 
//Função: inicializa parâmetros de conexão MQTT( define o endereço do broker, porta e a função de callback)
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //setar o servidor do broker passando o login e a porta
    MQTT.setCallback(mqtt_callback);            //define a função de callback (Esta função é chamada quando chega alguma informação de algum dos tópicos subescritos ao broker)
}
 
//Função: função de callback - esta função é chamada toda vez que uma informação de um dos tópicos subescritos chega)
void mqtt_callback(char* topic, byte* payload, unsigned int length) //Recebe como parâmetros o nome do tópico, o payload( o pacote com a mensagem)
                                                                    //e o tamanho da mensagem, que irá auxiliar na construção da mesma.
{
    String msg;
 
    //Obter a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
      
    }

    Serial.println("msg = " +  msg);

//mensagens que o publisher enviará para o broker, e para o tópico. Os subscribers deste tópico deverão "reagir" a mensagem.
//no caso da aula prática, o publisher foi um aplicativo de celular, e através de um tópico, em que haviam diversos ESP conectados, a mensagem
//era enviada por este publisher, e os subscribers "reagiam" acendendo o led na saída definida no trecho abaixo de código.
   if (msg.equals("ON GREEN"))
   {
      digitalWrite(D1, HIGH);
      Serial.println("Ligado led");
   }
   if (msg.equals("ON RED"))
   {
      digitalWrite(D2, HIGH);
      Serial.println("Ligado led");
   }

   if (msg.equals("OFF GREEN"))
   {
      digitalWrite(D1, LOW);
      Serial.println("Desligado led");
   }
   if (msg.equals("OFF RED"))
   {
      digitalWrite(D2, LOW);
      Serial.println("Ligado led");
   }   

 
}

//Reconectar ao broker MQTT, caso a conexão não tenha sido estabelecida ou se a mesma cair. Caso a conexão ou reconexão seja feita com sucesso,
//o subscribe nos tópicos é refeito.
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
//        if (MQTT.connect(ID_MQTT, USER_MQTT,PASS_MQTT)) // parameros usados para broker proprietário Passamos o ID do esp no MQTT, o usuário e a senha
                                                     
 
           if (MQTT.connect(ID_MQTT))
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE_P1);
 
        } 
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}

 
//Função: reconecta-se ao WiFi
void reconectWiFi() 
{
    //se já foi estabelecida a conexão com a rede WI-FI, nada é feito. 
    //Senão, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conexão na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Conectado com sucesso na rede: ");
    Serial.print(SSID);
    Serial.println();
    Serial.print("IP obtido: ");
    Serial.print(WiFi.localIP());  // mostra o endereço IP obtido por DHCP
    Serial.println();
    Serial.print("Endereço MAC: ");
    Serial.print(WiFi.macAddress()); // mostra o endereço MAC do esp8266
}

 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de cair a conexão de qualquer uma das duas, a conexão
//        é restabelecida

void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é restabelecida
    
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é restabelecida
}
 
 
//Função: inicializa o output

void InitOutput(void)
{    
    pinMode(D1, OUTPUT); //define a saida
    pinMode(D2, OUTPUT); //define a saida
    digitalWrite(D1, LOW); //LOW - led desligado. HIGH - led ligado
    digitalWrite(D2, LOW); //LOW - led desligado. HIGH - led ligado
              
}

 
 
// principal
void loop() 
{   
   
    ArduinoOTA.handle(); //tratar os eventos OTA

    //verifica o funcionamento das conexões WiFi e do broker MQTT
    VerificaConexoesWiFIEMQTT();
   
    MQTT.loop(); //checar como está funcionando o MQTT
   
}
