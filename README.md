# Termo em C — Projeto 1 (Algoritmos e Programação II)

Jogo Termo (variante em português do Wordle), rodando no terminal, conforme
o enunciado do Projeto 1.

## Como compilar e rodar

```sh
make
./termo
```

ou manualmente:

```sh
gcc -Wall -Wextra -std=c11 -o termo termo.c
./termo
```

Baixe o dicionário oficial e salve como `sem_acentos.txt` no mesmo diretório
do executável:
https://github.com/thoughtworks/dadoware/blob/master/fontes/sem_acentos.txt

## Regras

- Uma palavra secreta de 5 letras é sorteada a cada execução.
- O jogador tem 6 tentativas para acertar.
- Se o palpite não estiver no dicionário, é pedido novamente **sem** descontar
  tentativa.
- Feedback exibido em caixa, letra por letra:
  - `^` = letra na posição correta
  - `!` = letra existe na palavra, mas em posição errada
  - `x` = letra não existe na palavra
- Letras repetidas são tratadas corretamente (mesma lógica do Wordle original).
- Ao final da partida, o programa pede o nome do jogador e imprime uma linha
  resumo: `Nome; Palavra; Tentativas; Tempo(segundos)`.
- O jogador pode optar por jogar novamente.

## Estrutura

- `termo.c` — programa principal (arquivo único, apenas libc padrão)
- `sem_acentos.txt` — dicionário oficial (baixar do link acima)
- `Makefile` — compilação com `make`

## Função de busca própria

Conforme pedido no enunciado, a busca no dicionário é feita por uma função
própria (`buscaSequencial`), sem usar `bsearch`/`lsearch` da libc.

## Observação técnica

Como o dicionário oficial (`sem_acentos.txt`) não contém acentos, o programa
trabalha apenas com ASCII simples — não há necessidade de decodificação
UTF-8 multibyte.