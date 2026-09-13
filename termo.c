

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>

#define MAX_WORDS 2000
#define WORD_LEN 5
#define MAX_ATTEMPTS 6


static int char_to_code(const char *s)
{
    unsigned char c = (unsigned char)*s;
    if (c < 0x80)
        return c;
    if ((c & 0xE0) == 0xC0)
        return 0x80 + (unsigned char)s[1];
    return 0xF0 + c;
}


static int utf8_len(const char *s)
{
    int count = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if ((c & 0xC0) != 0x80) 
            count++;
        s++;
    }
    return count;
}

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

    if ((c & 0xE0) == 0xC0) {
        unsigned char c2 = (unsigned char)src[1];
        if (c == 0xC3 && c2 >= 0xA0 && c2 <= 0xBF) {
            if (dst_size < 3) return;
            dst[0] = '\xC3';
            dst[1] = (char)(c2 - 0x20);
            dst[2] = '\0';
            return;
        }
        if (dst_size < 3) return;
        dst[0] = (char)c;
        dst[1] = (char)c2;
        dst[2] = '\0';
        return;
    }


    int len = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
    if (dst_size < len + 1) return;
    for (int i = 0; i < len; i++)
        dst[i] = src[i];
    dst[len] = '\0';
}



static const char *DICT_FILE = "palavras.txt";

/* ---------- Funções do jogo ---------- */
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
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        /* Pula linhas vazias */
        if (len == 0) continue;

        if (!utf8_valid(line)) {
            fprintf(stderr, "Aviso: ignorando palavra invalida (UTF-8): %s\n", line);
            continue;
        }

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


static int pick_word(int count)
{
    return rand() % count;
}


static int is_valid_guess(const char *guess, const char *words[], int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(guess, words[i]) == 0)
            return 1;
    }
    return 0;
}

static void calc_feedback(const char *secret, const char *guess,
                          int *feedback, int guess_len, int secret_len)
{
    int total[256] = {0}; 
    int remaining[256] = {0}; 
    int is_correct[32] = {0};


    const char *s = secret;
    while (*s) {
        int code = char_to_code(s);
        total[code]++;

        s++;
        while (*s && ((unsigned char)*s & 0xC0) == 0x80)
            s++;
    }


    s = secret;
    const char *g = guess;
    int si = 0, gi = 0;
    while (si < secret_len && gi < guess_len) {
        int scode = char_to_code(s);
        int gcode = char_to_code(g);

        if (scode == gcode) {
            feedback[gi] = 2;
            is_correct[gi] = 1;
            total[gcode]--;
        } else {
            feedback[gi] = -1; 
        }


        s++;
        while (*s && ((unsigned char)*s & 0xC0) == 0x80) s++;
        g++;
        while (*g && ((unsigned char)*g & 0xC0) == 0x80) g++;
        si++;
        gi++;
    }

   
    memcpy(remaining, total, sizeof(total));

 
    g = guess;
    gi = 0;
    while (gi < guess_len) {
        int gcode = char_to_code(g);

        if (!is_correct[gi] && remaining[gcode] > 0) {
            feedback[gi] = 1; 
            remaining[gcode]--;
        } else if (!is_correct[gi]) {
            feedback[gi] = 0; 
        }

        g++;
        while (*g && ((unsigned char)*g & 0xC0) == 0x80) g++;
        gi++;
    }
}

static void show_feedback(const char *guess, const int *feedback, int len)
{
    const char *g = guess;
    int i = 0;
    while (i < len) {
        char upper[8];
        utf8_upper(g, upper, sizeof(upper));

        if (feedback[i] == 2)
            printf("%s", upper);     
        else if (feedback[i] == 1)
            printf("!");             
        else
            printf("^");             
        g++;
        while (*g && ((unsigned char)*g & 0xC0) == 0x80) g++;
        i++;
    }
    printf("\n");
}

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

            int len = (int)strlen(guess);
            while (len > 0 && (guess[len - 1] == '\n' || guess[len - 1] == '\r'))
                guess[--len] = '\0';

            if (!utf8_valid(guess)) {
                printf("Palpite invalido (caracteres invalidos). Tente novamente.\n");
                continue;
            }

            if (utf8_len(guess) != WORD_LEN) {
                printf("Palpite deve ter exatamente 5 letras. Tente novamente.\n");
                continue;
            }

            to_lower_utf8(guess);

            if (!is_valid_guess(guess, words, word_count)) {
                printf("Palpite nao encontrado na lista de palavras. Tente novamente.\n");
                continue;
            }

            valid = 1;
        }


        int feedback[32] = {0};
        calc_feedback(secret, guess, feedback, WORD_LEN, secret_len);
        show_feedback(guess, feedback, WORD_LEN);

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

static int play_again(void)
{
    char resp[16];
    while (1) {
        printf("\nDeseja jogar novamente? (s/n): ");
        if (!fgets(resp, sizeof(resp), stdin)) return 0;


        int len = (int)strlen(resp);
        while (len > 0 && (resp[len - 1] == '\n' || resp[len - 1] == '\r'))
            resp[--len] = '\0';

        if (len == 1) {
            char c = resp[0];
            if (c == 's' || c == 'S') return 1;
            if (c == 'n' || c == 'N') return 0;
        }

        printf("Resposta invalida. Digite 's' para sim ou 'n' para nao.\n");
    }
}


int main(void)
{
    setlocale(LC_ALL, "");

    srand((unsigned int)time(NULL));

  
    static char raw_words[MAX_WORDS][256];
    int word_count = load_words(DICT_FILE, raw_words, MAX_WORDS);
    if (word_count <= 0) {
        fprintf(stderr, "Erro: nenhuma palavra valida carregada de '%s'\n",
                DICT_FILE);
        return 1;
    }

    
    const char *words[MAX_WORDS];
    for (int i = 0; i < word_count; i++)
        words[i] = raw_words[i];

    printf("Carregadas %d palavras de '%s'.\n", word_count, DICT_FILE);


    do {
        play_game(words, word_count);
    } while (play_again());

    printf("Obrigado por jogar!\n");
    return 0;
}
