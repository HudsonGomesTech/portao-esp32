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

Os links são da [Usinainfo](https://www.usinainfo.com.br/), onde as peças do vídeo foram compradas. Não é patrocínio; as mesmas peças
existem no Mercado Livre e na Amazon. Preços no Pix em 15/11/2026, só para dar uma ideia.

**O kit (cerca de R$ 73)**

| Peça | Observação | Preço |
|---|---|---|
| [ESP32-C3 Super Mini](https://www.usinainfo.com.br/esp32/esp32-c3-super-mini-com-wifi-e-bluetooth-com-usb-c-8967.html) | a placa usada no vídeo | R$ 32,60 |
| [Módulo relé **5 V** de 1 canal, com optoacoplador e jumper H/L](https://www.usinainfo.com.br/rele-arduino/modulo-rele-5v-10a-1-canal-com-optoacoplador-para-esp32-e-arduino-2533.html) | faz o papel do botão do portão. **Cuidado para não comprar o de 12 V.** | R$ 10,93 |
| [Fonte USB 5 V 2 A com cabo USB-C](https://www.usinainfo.com.br/fonte-chaveada-usb-e-p4/carregador-fonte-usb-2a-com-cabo-usb-c-para-arduino-e-esp32-8591.html) | um carregador de celular que você já tenha também serve | R$ 29,07 |
| Fios finos | três entre a placa e o relé, dois entre o relé e o portão | sobra de casa |

O relé que aparece no vídeo é um módulo de **3 V** (`JQC3F-03VDC`, azul), ligado no pino `3.3` da placa. O módulo de
**5 V** do link acima funciona do mesmo jeito, ligado no pino `5V`. O de 12 V não funciona com esta placa.

Para gravar o programa, o cabo USB-C precisa ser **de dados**: cabo só de carga não serve.

**Compra única, se você ainda não tem (cerca de R$ 91)**

| Ferramenta | Observação | Preço |
|---|---|---|
| [Ferro de solda Hikari 34 W](https://www.usinainfo.com.br/ferro-de-solda/ferro-de-solda-hikari-34w-plus-sc40-1864.html) | confira a tensão da sua tomada (127 V ou 220 V) | R$ 68,29 |
| [Estanho em fio 1,0 mm 63/37 com fluxo](https://www.usinainfo.com.br/estanho-para-solda/fio-de-estanho-para-solda-10mm-liga-6337-tubo-22g-cobix-8293.html) | um tubo dura muito | R$ 22,27 |

## Ligação

| De | Para |
|---|---|
| ESP32 `5V` (relé de 5 V) ou `3.3` (relé de 3 V, como no vídeo) | relé `DC+` (ou `VCC`) |
| ESP32 `G` (GND) | relé `DC-` (ou `GND`) |
| ESP32 `GPIO 5` | relé `IN` |
| relé `COM` e `NO` (normalmente aberto) | os dois bornes de **botoeira** da placa do portão |

Atenção a três pontos:

- Os três fios do ESP32 vão no borne `DC+ / DC- / IN`. O outro borne (`NO / COM / NC`) é o contato do relé e vai
  **só** para o portão. Fio do ESP32 nesse borne pode queimar a placa.
- Módulo com relé de **5 V** (escrito `05VDC` no relé) é alimentado pelo pino `5V` do ESP32. Só o módulo com relé de
  **3 V** (`03VDC`) pode ser alimentado pelo pino `3.3`.
- Se o módulo tiver o jumper `H / L`, deixe em **H** (liga com nível alto), que é o padrão do programa.

O relé só encosta os dois bornes de botoeira por meio segundo, como um botão de parede. Confira no manual da sua placa
quais são esses bornes. O pino, o nível que liga o relé e a duração do toque podem ser trocados na tela de configuração.

## Como gravar na placa

### Com o Claude Code (como no vídeo)

1. No Windows, abra o PowerShell e o Claude Code instalado no computador (não a versão do navegador).
2. Ligue a placa no computador pelo cabo USB.
3. Cole o texto do arquivo [`PROMPT.md`](PROMPT.md).
4. Quando a gravação terminar, tire e ponha o cabo USB. A rede `HGT-Portao-XXXX` aparece em alguns segundos.

O Claude Code precisa de um plano pago (o Pro já serve). O plano gratuito não inclui o Claude Code.

### Sem o Claude Code, pelo navegador

1. Baixe o arquivo `.bin` da pasta [`firmware`](firmware).
2. No Chrome ou no Edge, abra <https://espressif.github.io/esptool-js/>.
3. Clique em **Connect** e escolha a porta da placa. Se ela não aparecer, segure o botão BOOT enquanto encaixa o cabo.
4. Em **Flash Address** coloque `0x0`, escolha o arquivo `.bin` e clique em **Program**.
5. Quando terminar, tire e ponha o cabo USB.

> **A rede da placa não apareceu?** Logo depois de gravar, a placa pode continuar no modo de gravação, e o programa
> novo ainda não está rodando. Tire e ponha o cabo USB, sem apertar o botão BOOT.

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
