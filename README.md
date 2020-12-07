# Projeto final

![satc](https://user-images.githubusercontent.com/74508536/99385441-c5ed8e80-28af-11eb-97ec-6b98010a5193.png)


## **Sumário**

* [Resumo](#resumo)
* [Requisitos](#requisitos)
* [Executar](#executar)
* [Funcionamento](#funcionamento)


## **Resumo**

O projeto utiliza o ESP8266 para criar um servidor HTTP e enviar as informações lidas dos sensores DHT11 e HC-SR04.
Uma página na Web irá requisitar as informações a cada 5 segundos e atualizar os valores na tela.


## **Requisitos**

### Pré-requisitos
	
* ESP8266 Toolchain
* ESP8266 RTOS SDK
* ESP8266 Python Packages

### Componentes utilizados:

* WeMos D1 R2
* ESP8266
* Sensor de temperatura e umidade DHT11
* Sensor de distância HC-SR04
* Botão

### Recursos utilizados:

#### Linguagem C

* GPIO Driver
* FreeRTOS Tasks
* FreeRTOS Event Groups
* FreeRTOS Queues
* Esp8266 HTTP Server
* ESP WiFi
* ESP Non-volatile storage
* Biblioteca DHT & Ultrasonic


## **Executar**

### Circuito eletrônico

![circuito](https://user-images.githubusercontent.com/74508536/99253370-62039100-27ef-11eb-8d94-2276312984b7.png)

Os componentes foram ligados nos seguintes pinos da GPIO, conforme o trecho de código:

```
#define LED_BUILDING       GPIO_NUM_2 	//LED DE SINALIZAÇÃO DO WIFI
#define BUTTON             GPIO_NUM_0	//BOTÃO PARA RECONECTAR WIFI
#define TRIGGER_GPIO        	    4   //TRIGGER - SENSOR HC-SR04
#define ECHO_GPIO                   5   //ECHO - SENSOR HC-SR04
static const gpio_num_t dht_gpio = 16;	//DADOS - SENSOR DHT11
```


### **Conectar**
	
Conecte o ESP8266 no PC em uma porta USB e verifique se a comunicação serial funciona.
O nome da porta será usado na configuração do projeto.


### **Configurar**
	
1. Através do terminal, vá até o diretorio do projeto.

```
cd ~/Esp8266-HTTPServer-Leitura-Sensores
```

2. Em seguida, execute o comando:

```
make menuconfig
```

3. No menu de configuração, navegue até **_Serial flasher config_** -> **_Default serial port_** e digite o nome da
   porta usada.
   
![serial](https://user-images.githubusercontent.com/74508536/99197360-ccc1b780-2770-11eb-8a6e-c981e5e57fd6.png)

Salve as alterações utilizando a opção *Save* e volte ao menu principal através da opção *Exit*.

4. Em seguida, navegue até **_configuração do projeto_** e informe o SSID e senha da rede WiFi.

Salve as alterações utilizando a opção *Save*.

Após realizar todas as configurações utilize a opção *Exit*.


### **Build e Flash**

Para compilar o aplicativo e todos os componentes e monitorar a execução do aplicativo utilizar o comando:

```
make flash monitor
```


## Funcionamento

### Aplicação

A aplicação inicializa o armazenamento não volátil e as configurações do WiFi do ESP8266. Em seguida, são criadas as
tarefas do FreeRTOS para conexão com WiFi, leituras dos sensores de temperatura e distância.

* *task_sinalizarConexaoWifi*
* *task_reconectarWifi*
* *task_LerTemperaturaUmidade*
* *task_lerDistancia*

A função *event_handler* tem a finalidade de receber os eventos de conexão do WiFi, sendo utilizados Event Groups
para indicar a situação da conexão para as demais tarefas e impedir que a criação do servidor HTTP seja realizada
sem estar conectado com o WiFi local.

A task *task_sinalizarConexaoWifi* sinaliza através do *LED BUILDING* o status da conexão com o WiFi.
* 500ms - WiFi conectando
* 100ms - WiFi falhou
* 10ms - WiFi conectado

Quando ocorrer falha na conexão do WiFi, a tarefa *task_reconectarWifi* é desbloqueada para tentar nova conexão.
A tarefa de reconexão lê um botão que ao ser pressionado tenta reconectar novamento ao WiFi.

Ao se conectar ao WiFi o servidor HTTP é criado, e então são registrados os manipuladores de URI passando a estrutura
httpd_uri_t. Nesta estrutura são configurados o nome da uri, método HTTP e a função handler.

As tasks *task_LerTemperaturaUmidade* e *task_lerDistancia* leem as informações dos sensores conectados.

Quando o usuário requisitar uma das informações, o respectivo handler irá retornar o valor na requisição.


### Cliente

A página Web irá requisitar as informações a cada 5 segundos e atualizar o valor dos componentes.

* server_url/temperatura
* server_url/umidade
* server_url/distancia

![webpage](https://user-images.githubusercontent.com/74508536/101259133-c71f1800-3705-11eb-8f95-fb62e1af44aa.png)
