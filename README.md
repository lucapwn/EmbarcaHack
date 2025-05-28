# EmbarcaHack

O **EmbarcaHack** é um Hackathon de Sistemas Embarcados promovido pelo [EmbarcaTech](https://embarcatech.softex.br/) - Residência Tecnológica em Sistemas Embarcados.

![Badge](https://img.shields.io/static/v1?label=license&message=MIT&color=1E90FF)
![Badge](https://img.shields.io/static/v1?label=build&message=passing&color=00d110)

## Conteúdo

- [Apresentação](#apresentação)
- [Documentos](#documentos)
- [Objetivos](#objetivos)
- [Requisitos Funcionais](#requisitos-funcionais)
- [Arquitetura de Hardware](#arquitetura-de-hardware)
  - [Conexões dos Componentes](#conexões-dos-componentes)
  - [Estrutura da BitDogLab](#estrutura-da-bitdoglab)
- [Arquitetura do Firmware](#arquitetura-do-firmware)
- [Fluxograma](#fluxograma)
- [Indicação do uso de IA](#indicação-do-uso-de-ia)
- [Conclusão](#conclusão)
- [Referências](#referências)
- [Licença](#licença)

## Apresentação

Este repositório tem como finalidade apresentar os principais aspectos do desenvolvimento do projeto [IncluiAuth](https://github.com/lucapwn/EmbarcaHack), abordando sua arquitetura, implementação e funcionalidades. O IncluiAuth é uma interface de autenticação alternativa, desenvolvida para promover a acessibilidade de pessoas com deficiência, o qual permite a inserção de senhas por meio de um joystick ou por detecção sonora, tornando sistemas de segurança mais inclusivos.

Veja este vídeo de exemplo no YouTube em [```https://www.youtube.com/watch?v=Q9PgCGsT6_M```](https://www.youtube.com/watch?v=Q9PgCGsT6_M).

![BitDogLab](https://github.com/user-attachments/assets/f87bad89-73f1-4589-aa28-94ec056434df)

## Documentos

- [Desafio EmbarcaHack](https://github.com/user-attachments/files/20361960/Desafio.-.EmbarcaHack.pdf)
- [Relatório do Projeto](https://github.com/user-attachments/files/20361999/Relatorio.-.EmbarcaHack.pdf)

## Objetivos

Este projeto tem como objetivos principais:

- Criar uma solução de autenticação acessível, capaz de atender às necessidades de usuários com diferentes deficiências;
- Proporcionar múltiplos métodos de entrada de senha (joystick e som) para ampliar a acessibilidade de pessoas com limitações motoras nas mãos e dedos, por exemplo;
- Fornecer feedback visual e sonoro durante o processo de autenticação;
- Implementar um sistema de segurança confiável com validação de senha;
- Desenvolver uma solução integrável a diferentes sistemas que demandam controle de acesso, como portas e portões eletrônicos, catracas automatizadas, controle veicular e terminais de atendimento automático, como os utilizados em hospitais e outros ambientes.

Esses objetivos visam promover a inclusão digital, assegurando que pessoas com limitações motoras ou outras deficiências possam utilizar sistemas de autenticação de forma autônoma, segura e eficiente.

## Requisitos Funcionais

Os requisitos funcionais do sistema proposto são:

| ID   | Requisito Funcional                                                                                                                                                          |
| ---- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| RF01 | O sistema deve fornecer dois modos de autenticação: via joystick e via detecção sonora (palmas/batidas);                                                                     |
| RF02 | O sistema deve permitir a seleção do método de autenticação via botões físicos (Botões A e B);                                                                               |
| RF03 | O sistema deve exibir informações do status de autenticação em um display OLED (acesso permitido ou negado);                                                                 |
| RF04 | O sistema deve fornecer feedback visual via matriz de LEDs RGB durante o processo de autenticação (opção selecionada, acesso permitido ou negado);                           |
| RF05 | O sistema deve fornecer feedback sonoro (beeps) para indicar as ações do usuário e informar se a autenticação foi bem-sucedida ou negada;                                    |
| RF06 | O sistema deve permitir a entrada de senha de 4 dígitos por meio de seleção de posições em uma matriz 5x5;                                                                   |
| RF07 | O sistema deve validar a senha inserida comparando-a com uma senha previamente configurada;                                                                                  |
| RF08 | O sistema deve fornecer feedback visual e sonoro distintos para indicar se o acesso foi permitido ou negado;                                                                 |
| RF09 | No modo joystick, o sistema deve permitir a navegação pela matriz utilizando o joystick analógico e a seleção de elementos por meio do botão central;                        |
| RF10 | No modo som, o sistema deve reconhecer sons únicos (vozes, palmas e batidas) para navegação pela matriz e sons duplos (vozes, palmas e batidas) para a seleção de elementos. |

## Arquitetura de Hardware

A arquitetura de hardware do sistema contempla os seguintes componentes:

| Componente                                               | Descrição                                                                                                  |
| -------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| Raspberry Pi Pico W                                      | Microcontrolador responsável por processar os dados e controlar os periféricos.                            |
| Display OLED 128x64 0.96" (conectado via I2C)            | Interface de saída visual responsável por exibir mensagens.                                                |
| Matriz de LEDs RGB 5x5 WS2812B (controlada via PIO)      | Interface de saída visual responsável por exibir animações e a posição da senha.                           |
| Buzzer passivo (controlado via PWM)                      | Interface de saída sonora responsável por reproduzir beeps para indicar uma ação.                          |
| Joystick analógico KY-023 (conectado às entradas ADC)    | Interface de entrada responsável por coletar a posição da senha através de coordenadas X e Y.              |
| Microfone analógico GY-MAX4466 (conectado à entrada ADC) | Interface de entrada responsável por coletar a posição da senha através de sons (vozes, palmas e batidas). |
| 2 botões tácteis (chaves push-button 12x12x7.5mm)        | Interfaces de entrada responsáveis por coletar o método de autenticação desejado.                          |

### Conexões dos Componentes

A tabela a seguir apresenta o esquema de conexões dos componentes:

| Interface | Componente               | Pino |
| --------- | ------------------------ | ---- |
| ADC0      | VRY (eixo Y do joystick) | 26   |
| ADC1      | VRX (eixo X do joystick) | 27   |
| ADC2      | Microfone                | 28   |
| GPIO      | Botão A                  | 5    |
| GPIO      | Botão B                  | 6    |
| PIO       | Matriz de LEDs           | 7    |
| I2C SDA   | Display OLED             | 14   |
| I2C SCL   | Display OLED             | 15   |
| PWM       | Buzzer                   | 21   |
| GPIO      | Botão do joystick (SW)   | 22   |

### Estrutura da BitDogLab

As imagens a seguir ilustram a placa BitDogLab utilizada neste projeto:

<img src="https://github.com/user-attachments/assets/51168dc1-6a9c-481e-9948-4a4b89271412" width="345"><br>

<img src="https://github.com/user-attachments/assets/7c04f30c-6320-49ef-9161-fb71baa16276" width="345"><br>

<img src="https://github.com/user-attachments/assets/d27586c2-0b5a-4160-8275-d903127fc254" width="345"><br>

<img src="https://github.com/user-attachments/assets/db90be2f-5b4b-4192-b0ca-1edb28300c85" width="345"><br>

## Arquitetura do Firmware

O firmware foi desenvolvido em linguagem C para o [SDK](https://github.com/raspberrypi/pico-sdk) do Raspberry Pi Pico W, utilizando o [VSCode](https://code.visualstudio.com/) como IDE, através da extensão [Raspberry Pi Pico](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico), e está estruturado em módulos que incluem:

| ID | Módulo                                   | Descrição                                                                                                                |
| -- | ---------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| 1  | Inicialização e Configuração de Hardware | Configuração das portas (GPIO, ADC, I2C, PWM e PIO) e inicialização dos periféricos (display OLED, matriz de LEDs, etc). |
| 2  | Controle da Matriz de LEDs               | Gerenciamento dos LEDs RGB WS2812B via PIO e funções para definir cores, resetar e escrever na matriz.                   |
| 3  | Gerenciamento de Áudio                   | Controle do buzzer via PWM e definição de diferentes frequências para feedback sonoro.                                   |
| 4  | Gerenciamento do Display OLED            | Interface com o display via I2C e funções para exibir textos e informações de status.                                    |
| 5  | Processamento de Entrada do Joystick     | Leitura dos valores analógicos dos eixos X e Y e detecção de pressão do botão do joystick.                               |
| 6  | Processamento de Entrada Sonora          | Detecção de palmas via microfone e temporização para detecção de palmas duplas.                                          |
| 7  | Lógica de Autenticação                   | Verificação da senha inserida, gerenciamento do processo de entrada de senha e feedback de acesso permitido ou negado.   |
| 8  | Interface de Usuário                     | Gerenciamento da seleção do método de autenticação e exibição do status do processo de autenticação.                     |

## Fluxograma

<img src="https://www.mermaidchart.com/raw/438adeef-2648-4d42-a505-f6783c759e18?theme=light&version=v0.1&format=svg" width="800">

## Indicação do uso de IA

O projeto fez uso de Inteligência Artificial (ChatGPT) para auxiliar na redação do relatório, a qual contribuiu para a organização e correção gramatical do texto. Além disso, a extensão LanguageTool foi empregada para aprimorar a revisão linguística.

## Conclusão

O projeto [IncluiAuth](https://github.com/lucapwn/EmbarcaHack) alcançou os objetivos propostos, ao qual proporcionou uma solução inovadora de autenticação voltada para pessoas com deficiência. A implementação dos dois métodos de entrada, joystick e som, oferece flexibilidade para atender diferentes tipos de necessidades, permitindo que usuários com limitações motoras ou outras deficiências utilizem sistemas de autenticação de forma segura e autônoma.

A combinação de feedback visual, por meio da matriz de LEDs e do display OLED, com o feedback sonoro fornecido pelo buzzer, proporciona uma experiência de uso mais intuitiva e acessível. Além disso, a arquitetura modular do firmware facilita a adaptação e a expansão do sistema, permitindo a inclusão de novos métodos de entrada (futuramente por Wi-Fi, por exemplo) e a integração com outras plataformas.

O IncluiAuth demonstra como tecnologias acessíveis e de fácil implementação podem ser combinadas para promover a inclusão digital, contribuindo para a construção de uma sociedade mais acessível e equitativa.

## Referências

[BitDogLab](https://github.com/BitDogLab/BitDogLab)

[BitDogLab-C](https://github.com/BitDogLab/BitDogLab-C )

[Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf) 

[Raspberry Pi Pico Extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)

[Making the Web Accessible](https://www.w3.org/WAI/) 

[RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) 

[ChatGPT](https://chatgpt.com/) 

[LanguageTool](https://languagetool.org/pt) 

[VSCode](https://code.visualstudio.com/) 

[Mermaid](https://mermaid.js.org/)

## Licença

Esse software é licenciado pelo [MIT](https://choosealicense.com/licenses/mit/).
