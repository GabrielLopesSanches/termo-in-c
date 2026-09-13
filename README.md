# termo-in-c

Jogo Termo (variante em português do Wordle) escrito em C, para rodar no terminal.

## Como compilar e rodar

```sh
gcc -o termo termo.c -Wall -Wextra
./termo
```

Requer o arquivo `palavras.txt` no mesmo diretório do executável (uma palavra por linha, exatamente 5 letras).

## Regras

- Uma palavra secreta de 5 letras é sorteada aleatoriamente a cada partida.
- O jogador tem 6 tentativas para acertar.
- Cada palpite deve existir na lista de palavras; caso contrário, é pedido um novo palpite sem consumir tentativa.
- Para cada palpite, o programa exibe o resultado letra por letra:
  - Letra em **maiúscula** = posição correta
  - `!` = existe na palavra, mas está na posição errada
  - `^` = não existe na palavra
- Letras repetidas são tratadas corretamente (mesma lógica do Wordle original).
- Ao final de cada partida (vitória ou derrota), o jogador pode escolher jogar novamente.

## Estrutura

- `termo.c` — programa principal (arquivo único, sem dependências externas além da libc)
- `palavras.txt` — dicionário de palavras válidas (~270 palavras)
- `AGENTS.md` — notas técnicas para agentes de código

## Cuidados técnicos

- UTF-8 tratado manualmente: contagem de letras, validação, comparação e conversão de caixa para caracteres acentuados (á, ã, ç, é, etc.)
- Feedback usa arrays indexados por posição da letra (não por byte)
- Locale configurado com `setlocale(LC_ALL, "")` para suporte a UTF-8 no terminal