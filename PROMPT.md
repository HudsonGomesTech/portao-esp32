# Prompt para gravar a placa com o Claude Code

Abra o Claude Code no PowerShell do Windows, com a placa ESP32-C3 ligada no USB, e cole o texto abaixo.

```text
Quero gravar o programa de demonstração "Portão com ESP32", da Hudson Gomes Tech, na placa ESP32-C3 que está ligada
no USB deste computador. O projeto está em https://github.com/HudsonGomesTech/portao-esp32

Faça nesta ordem e me explique cada passo em português simples:

1. Leia o README do projeto.
2. Confira se o Python e o esptool estão instalados. Se faltar, instale (winget para o Python, pip para o esptool).
3. Descubra em qual porta COM a placa está. Se ela não aparecer, me peça para segurar o botão BOOT e reconectar o cabo.
4. Antes de gravar, faça uma cópia de segurança de tudo o que está na placa hoje: leia a memória inteira com o esptool
   (read_flash, do endereço 0 até o tamanho total que o flash_id informar) e salve na pasta Documentos com a data no
   nome. Confira se o arquivo ficou com o tamanho certo. Esse arquivo pode conter a senha do Wi-Fi: não envie a ninguém.
5. Baixe o arquivo .bin da pasta firmware do projeto e confira o SHA-256 com o arquivo SHA256SUMS da mesma pasta.
6. Me mostre o comando de gravação e espere eu confirmar.
7. Grave o .bin no endereço 0x0 (chip esp32c3) e confira a gravação.
8. No fim, me peça para tirar e pôr o cabo USB: a placa só começa a rodar o programa novo depois disso. Aí me diga
   o que fazer no celular para configurar a placa.

Não grave nada antes da cópia de segurança terminar e de eu confirmar.
```

Para voltar ao que estava na placa antes, peça ao Claude Code: "grave de volta na placa a cópia de segurança que está
na pasta Documentos, no endereço 0x0".
