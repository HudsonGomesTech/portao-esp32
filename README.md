# Portão com ESP32

Programa de demonstração do canal **Hudson Gomes Tech** para acionar um portão eletrônico com um ESP32 e um relé, por
uma página no celular.

> **Aviso.** Este programa foi criado apenas para laboratório e estudo. Não foi feito para uso produtivo, não tem
> garantia e você usa por sua conta e risco. Mexa só nos bornes de comando (botoeira) da placa do portão, com o portão
> desligado da tomada. Se não tiver segurança para isso, chame um instalador.

## O que ele faz

- Na primeira vez, a placa abre uma rede Wi-Fi própria (`HGT-Portao-XXXX`). Você entra nela pelo celular, escolhe o
  Wi-Fi da casa (2,4 GHz) e cria o usuário e a senha da página.
- Depois, a placa entra no Wi-Fi da casa e mostra uma página com login: situação da placa (rede, sinal, endereço,
  internet) e o botão **Acionar o portão**, que dá um toque de meio segundo no relé.
- Segurar o botão **BOOT** da placa por 5 segundos apaga a configuração e volta ao começo.

## Material

| Peça | Observação |
|---|---|
| ESP32-C3 Super Mini | a placa usada no vídeo |
| Módulo relé de 1 canal, 5 V, com optoacoplador | faz o papel do botão do portão |
| Fios (jumpers) | três entre a placa e o relé, dois entre o relé e o portão |
| Cabo USB-C **de dados** | cabo só de carga não serve para gravar |
| Ferro de solda e estanho fino com fluxo (0,5 a 0,8 mm) | para soldar os pinos da placa |
| Fonte USB de 5 V | um carregador de celular serve |

## Ligação

| De | Para |
|---|---|
| ESP32 `5V` | relé `VCC` |
| ESP32 `GND` | relé `GND` |
| ESP32 `GPIO 5` | relé `IN` |
| relé `COM` e `NA` (normalmente aberto) | os dois bornes de **botoeira** da placa do portão |

O relé só encosta os dois bornes de botoeira por meio segundo, como um botão de parede. Confira no manual da sua placa
quais são esses bornes. O pino, o nível que liga o relé e a duração do toque podem ser trocados na tela de configuração.

## Como gravar na placa

### Com o Claude Code (como no vídeo)

1. No Windows, abra o PowerShell e o Claude Code instalado no computador (não a versão do navegador).
2. Ligue a placa no computador pelo cabo USB.
3. Cole o texto do arquivo [`PROMPT.md`](PROMPT.md).

O Claude Code precisa de um plano pago (o Pro já serve). O plano gratuito não inclui o Claude Code.

### Sem o Claude Code, pelo navegador

1. Baixe o arquivo `.bin` da pasta [`firmware`](firmware).
2. No Chrome ou no Edge, abra <https://espressif.github.io/esptool-js/>.
3. Clique em **Connect** e escolha a porta da placa. Se ela não aparecer, segure o botão BOOT enquanto encaixa o cabo.
4. Em **Flash Address** coloque `0x0`, escolha o arquivo `.bin` e clique em **Program**.

## Primeira configuração

1. No celular, entre na rede `HGT-Portao-XXXX` (sem senha).
2. Abra `http://192.168.4.1` se a página não abrir sozinha.
3. Escolha o Wi-Fi da casa, digite a senha dele e crie o usuário e a senha da página (8 caracteres ou mais).
4. Toque em **Salvar e conectar**. A placa reinicia e a rede `HGT-Portao` some.
5. Volte o celular para o Wi-Fi da casa e abra `http://hgt-portao.local` ou o endereço IP que o roteador deu à placa.

Teste primeiro com o relé **solto** do portão: ao tocar em **Acionar o portão**, o relé deve dar um clique. Se ele
ficar ligado direto e desligar no toque, troque o nível em *Opções avançadas* (segure BOOT por 5 s e configure de novo).

## Segurança

- A página só abre com usuário e senha. Cinco senhas erradas travam o login por um minuto.
- A rede `HGT-Portao` da primeira configuração é aberta: faça a configuração de uma vez e perto da placa.
- Dentro de casa a página usa HTTP sem criptografia. Para acessar de fora, **não abra porta no roteador**: use um túnel
  (o vídeo usa o Cloudflare Tunnel) e proteja o endereço com um aplicativo de acesso.
- A senha fica guardada na placa só como resumo (SHA-256 com sal), nunca em texto.

## Compilar você mesmo

O projeto usa o [PlatformIO](https://platformio.org/): `pio run` compila e `pio run -t upload` grava.

## Licença

[MIT](LICENSE). Pode usar, copiar e modificar, mantendo o aviso de autoria.
