/*
 * termo.c - Jogo Termo (variante em português do Wordle)
 *
 * Compilar: gcc -o termo termo.c -Wall -Wextra
 * Executar: ./termo (palavras.txt deve estar no mesmo diretório)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>

#define MAX_WORDS 2000
#define WORD_LEN 5
#define MAX_ATTEMPTS 6

/* ---------- Conversão de caractere ---------- */

/*
 * Converte um caractere UTF-8 em um código numérico único para indexação.
 * - ASCII (0x00-0x7F): retorna o próprio byte.
 * - UTF-8 de 2 bytes (0xC0-0xDF + 0x80-0xBF): retorna 0x80 + segundo_byte
 *   (mapeia para 128-191, distinto de ASCII e entre si).
 * Isso garante que a, á, ã, ç etc. tenham códigos diferentes.
 */
static int char_to_code(const char *s)
{
    unsigned char c = (unsigned char)*s;
    if (c < 0x80)
        return c;
    if ((c & 0xE0) == 0xC0)
        return 0x80 + (unsigned char)s[1];
    /* 3+ bytes: usa hash simples do primeiro byte */
    return 0xF0 + c;
}

/* ---------- Utilidades UTF-8 ---------- */

/*
 * Conta quantos pontos de código (letras) existem em uma string UTF-8.
 * Em UTF-8, bytes de continuação têm o padrão 10xxxxxx (0x80-0xBF).
 * Contamos apenas bytes que NÃO são de continuação.
 */
static int utf8_len(const char *s)
{
    int count = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if ((c & 0xC0) != 0x80) /* não é byte de continuação */
            count++;
        s++;
    }
    return count;
}

/*
 * Valida se uma string é UTF-8 bem formado.
 * Retorna 1 se válido, 0 caso contrário.
 */
static int utf8_valid(const char *s)
{
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80) {
            s++;
        } else if ((c & 0xE0) == 0xC0) {
            if ((s[1] & 0xC0) != 0x80) return 0;
            s += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
            s += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 ||
                (s[3] & 0xC0) != 0x80)
                return 0;
            s += 4;
        } else {
            return 0;
        }
    }
    return 1;
}

/*
 * Converte uma letra UTF-8 para maiúscula.
 * Para ASCII, usa toupper. Para caracteres latinos acentuados (2 bytes,
 * primeiro byte 0xC3), mapeia manualmente o segundo byte.
 */
static void utf8_upper(const char *src, char *dst, int dst_size)
{
    unsigned char c = (unsigned char)src[0];

    /* ASCII simples */
    if (c < 0x80) {
        if (dst_size < 2) return;
        dst[0] = (char)toupper(c);
        dst[1] = '\0';
        return;
    }

    /* Caractere UTF-8 de 2 bytes (0xC3 0x80-0xBF cobre a maioria dos acentos) */
    if ((c & 0xE0) == 0xC0) {
        unsigned char c2 = (unsigned char)src[1];
        if (c == 0xC3 && c2 >= 0xA0 && c2 <= 0xBF) {
            /* acentuados: mapear para 0x80-0x9F */
            if (dst_size < 3) return;
            dst[0] = '\xC3';
            dst[1] = (char)(c2 - 0x20);
            dst[2] = '\0';
            return;
        }
        /* outros 2 bytes: copiar como está */
        if (dst_size < 3) return;
        dst[0] = (char)c;
        dst[1] = (char)c2;
        dst[2] = '\0';
        return;
    }

    /* 3+ bytes: copiar como está */
    int len = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
    if (dst_size < len + 1) return;
    for (int i = 0; i < len; i++)
        dst[i] = src[i];
    dst[len] = '\0';
}

/* ---------- Constantes ---------- */

static const char *DICT_FILE = "palavras.txt";

/* ---------- Funções do jogo ---------- */

/*
 * Carrega palavras do arquivo para o vetor words.
 * Retorna o número de palavras carregadas ou -1 em caso de erro.
 * Valida: UTF-8 válido, exatamente 5 letras (pontos de código).
 */
static int load_words(const char *filename, char words[][256], int max_words)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", filename);
        return -1;
    }

    int count = 0;
    char line[256];
    while (count < max_words && fgets(line, sizeof(line), fp)) {
        /* Remove newline/carriage return */
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        /* Pula linhas vazias */
        if (len == 0) continue;

        /* Valida UTF-8 */
        if (!utf8_valid(line)) {
            fprintf(stderr, "Aviso: ignorando palavra invalida (UTF-8): %s\n", line);
            continue;
        }

        /* Valida tamanho: exatamente 5 letras */
        if (utf8_len(line) != WORD_LEN) {
            fprintf(stderr, "Aviso: ignorando palavra com tamanho %d: %s\n",
                    utf8_len(line), line);
            continue;
        }

        strcpy(words[count], line);
        count++;
    }

    fclose(fp);
    return count;
}

/*
 * Seleciona uma palavra aleatória do vetor words.
 */
static int pick_word(int count)
{
    return rand() % count;
}

/*
 * Verifica se um palpite existe na lista de palavras.
 * Retorna 1 se existe, 0 caso contrário.
 */
static int is_valid_guess(const char *guess, const char *words[], int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(guess, words[i]) == 0)
            return 1;
    }
    return 0;
}

/*
 * Calcula o feedback do Wordle (lógica de letras repetidas).
 *
 * Algoritmo:
 * 1. Conta ocorrências de cada letra na palavra secreta (total[256]).
 * 2. Primeira passagem: marca letras exatas (posição correta).
 *    Decrementa remaining[] para cada acerto.
 * 3. Segunda passagem: para posições não marcadas, verifica se a letra
 *    existe em remaining (posição errada) ou não existe (^).
 *
 * Parâmetros:
 *   secret  - palavra secreta (lowercase, UTF-8)
 *   guess   - palpite do jogador (lowercase, UTF-8)
 *   feedback - saída: array de inteiros (0=ausente, 1=posicao_errada, 2=correta)
 *   guess_len - número de letras no palpite
 *   secret_len - número de letras no segredo
 */
static void calc_feedback(const char *secret, const char *guess,
                          int *feedback, int guess_len, int secret_len)
{
    /*
     * Usamos arrays de 256 para mapear códigos de caracteres.
     * char_to_code() garante que a, á, ã, ç etc. tenham códigos diferentes.
     */
    int total[256] = {0};  /* ocorrências de cada caractere na palavra secreta */
    int remaining[256] = {0}; /* ocorrências restantes após marcar acertos */
    int is_correct[32] = {0}; /* marca posições já resolvidas como corretas */

    /* Fase 1: contar letras da palavra secreta */
    const char *s = secret;
    while (*s) {
        int code = char_to_code(s);
        total[code]++;
        /* Pula bytes de continuação */
        s++;
        while (*s && ((unsigned char)*s & 0xC0) == 0x80)
            s++;
    }

    /* Fase 2: marcar letras exatas (posição correta) */
    s = secret;
    const char *g = guess;
    int si = 0, gi = 0;
    while (si < secret_len && gi < guess_len) {
        int scode = char_to_code(s);
        int gcode = char_to_code(g);

        if (scode == gcode) {
            /* Letra e posição corretos */
            feedback[gi] = 2;
            is_correct[gi] = 1;
            total[gcode]--;
        } else {
            feedback[gi] = -1; /* processar na fase 3 */
        }

        /* Avança pela letra UTF-8 (pula bytes de continuação) */
        s++;
        while (*s && ((unsigned char)*s & 0xC0) == 0x80) s++;
        g++;
        while (*g && ((unsigned char)*g & 0xC0) == 0x80) g++;
        si++;
        gi++;
    }

    /* Copia total para remaining */
    memcpy(remaining, total, sizeof(total));

    /* Fase 3: marcar letras existentes em posição errada */
    g = guess;
    gi = 0;
    while (gi < guess_len) {
        int gcode = char_to_code(g);

        if (!is_correct[gi] && remaining[gcode] > 0) {
            feedback[gi] = 1; /* posição errada */
            remaining[gcode]--;
        } else if (!is_correct[gi]) {
            feedback[gi] = 0; /* não existe mais */
        }

        g++;
        while (*g && ((unsigned char)*g & 0xC0) == 0x80) g++;
        gi++;
    }
}

/*
 * Exibe o resultado de um palpite:
 * - Letra correta (posição certa): letra em maiúscula
 * - Letra existente (posição errada): "!"
 * - Letra inexistente: "^"
 */
static void show_feedback(const char *guess, const int *feedback, int len)
{
    const char *g = guess;
    int i = 0;
    while (i < len) {
        char upper[8];
        utf8_upper(g, upper, sizeof(upper));

        if (feedback[i] == 2)
            printf("%s", upper);     /* posição correta: letra maiúscula */
        else if (feedback[i] == 1)
            printf("!");             /* posição errada */
        else
            printf("^");             /* não existe */

        /* Avança pela letra UTF-8 */
        g++;
        while (*g && ((unsigned char)*g & 0xC0) == 0x80) g++;
        i++;
    }
    printf("\n");
}

/*
 * Converte uma string para lowercase (UTF-8).
 * Para caracteres 2 bytes (0xC3 0x80-0xBF), converte para 0xC3 0xA0-0xBF.
 */
static void to_lower_utf8(char *s)
{
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 'A' && c <= 'Z') {
            *s = (char)(c + 32);
            s++;
        } else if (c == 0xC3) {
            unsigned char c2 = (unsigned char)s[1];
            if (c2 >= 0x80 && c2 <= 0x9F) {
                s[1] = (char)(c2 + 0x20);
            }
            s += 2;
        } else {
            s++;
        }
    }
}

/*
 * Loop principal de uma partida.
 */
static void play_game(const char *words[], int word_count)
{
    int secret_idx = pick_word(word_count);
    const char *secret = words[secret_idx];
    int secret_len = utf8_len(secret);

    printf("\n========================================\n");
    printf("         TERMO - Jogo de Palavras       \n");
    printf("========================================\n");
    printf("Acerte a palavra de 5 letras em %d tentativas.\n", MAX_ATTEMPTS);
    printf("Feedback: letra = certa | ! = existe | ^ = nao existe\n\n");

    int won = 0;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        printf("--- Tentativa %d/%d ---\n", attempt, MAX_ATTEMPTS);

        char guess[256];
        int valid = 0;

        while (!valid) {
            printf("Digite seu palpite: ");
            if (!fgets(guess, sizeof(guess), stdin)) {
                printf("\nErro ao ler entrada. Tente novamente.\n");
                continue;
            }

            /* Remove newline */
            int len = (int)strlen(guess);
            while (len > 0 && (guess[len - 1] == '\n' || guess[len - 1] == '\r'))
                guess[--len] = '\0';

            /* Valida UTF-8 */
            if (!utf8_valid(guess)) {
                printf("Palpite invalido (caracteres invalidos). Tente novamente.\n");
                continue;
            }

            /* Valida tamanho */
            if (utf8_len(guess) != WORD_LEN) {
                printf("Palpite deve ter exatamente 5 letras. Tente novamente.\n");
                continue;
            }

            /* Converte para lowercase para comparação */
            to_lower_utf8(guess);

            /* Valida se existe na lista */
            if (!is_valid_guess(guess, words, word_count)) {
                printf("Palpite nao encontrado na lista de palavras. Tente novamente.\n");
                continue;
            }

            valid = 1;
        }

        /* Calcula e exibe feedback */
        int feedback[32] = {0};
        calc_feedback(secret, guess, feedback, WORD_LEN, secret_len);
        show_feedback(guess, feedback, WORD_LEN);

        /* Verifica vitória (todas as letras na posição correta) */
        int all_correct = 1;
        for (int i = 0; i < WORD_LEN; i++) {
            if (feedback[i] != 2) {
                all_correct = 0;
                break;
            }
        }

        if (all_correct) {
            printf("\nParabens! Voce acertou a palavra '%s' em %d tentativa(s)!\n",
                   secret, attempt);
            won = 1;
            break;
        }
    }

    if (!won) {
        printf("\nVoce perdeu! A palavra era: %s\n", secret);
    }
}

/*
 * Pergunta se o jogador quer jogar novamente.
 * Retorna 1 para sim, 0 para não.
 */
static int play_again(void)
{
    char resp[16];
    while (1) {
        printf("\nDeseja jogar novamente? (s/n): ");
        if (!fgets(resp, sizeof(resp), stdin)) return 0;

        /* Remove newline */
        int len = (int)strlen(resp);
        while (len > 0 && (resp[len - 1] == '\n' || resp[len - 1] == '\r'))
            resp[--len] = '\0';

        /* Aceita s/S/n/N */
        if (len == 1) {
            char c = resp[0];
            if (c == 's' || c == 'S') return 1;
            if (c == 'n' || c == 'N') return 0;
        }

        printf("Resposta invalida. Digite 's' para sim ou 'n' para nao.\n");
    }
}

/* ---------- Programa principal ---------- */

int main(void)
{
    /* Configura locale para UTF-8 (necessário para acentuação no terminal) */
    setlocale(LC_ALL, "");

    /* Inicializa gerador de números aleatórios */
    srand((unsigned int)time(NULL));

    /* Carrega palavras do arquivo */
    static char raw_words[MAX_WORDS][256];
    int word_count = load_words(DICT_FILE, raw_words, MAX_WORDS);
    if (word_count <= 0) {
        fprintf(stderr, "Erro: nenhuma palavra valida carregada de '%s'\n",
                DICT_FILE);
        return 1;
    }

    /* Converte para array de ponteiros para uso nas funções */
    const char *words[MAX_WORDS];
    for (int i = 0; i < word_count; i++)
        words[i] = raw_words[i];

    printf("Carregadas %d palavras de '%s'.\n", word_count, DICT_FILE);

    /* Loop principal: jogar / jogar novamente */
    do {
        play_game(words, word_count);
    } while (play_again());

    printf("Obrigado por jogar!\n");
    return 0;
}
