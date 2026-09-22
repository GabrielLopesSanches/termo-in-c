#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_WORDS     2000
#define WORD_LEN      5
#define MAX_ATTEMPTS  6
#define LINE_BUF      256

static const char *DICT_FILE = "sem_acentos.txt";


#define FB_CORRETA  2   
#define FB_EXISTE   1   
#define FB_AUSENTE  0   


static void para_maiusculas(char *s)
{
    for (int i = 0; s[i] != '\0'; i++)
        s[i] = (char)toupper((unsigned char)s[i]);
}

static int eh_palavra_valida(const char *s)
{
    int len = (int)strlen(s);
    if (len != WORD_LEN)
        return 0;

    for (int i = 0; i < len; i++) {
        if (!isalpha((unsigned char)s[i]))
            return 0;
    }
    return 1;
}

static void remove_quebra_linha(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

static int carrega_dicionario(const char *arquivo, char palavras[][LINE_BUF], int max)
{
    FILE *fp = fopen(arquivo, "r");
    if (!fp) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", arquivo);
        return -1;
    }

    int total = 0;
    char linha[LINE_BUF];

    while (total < max && fgets(linha, sizeof(linha), fp)) {
        remove_quebra_linha(linha);

        if (strlen(linha) == 0)
            continue; 

        if (!eh_palavra_valida(linha)) {
            continue;
        }

        para_maiusculas(linha);
        strcpy(palavras[total], linha);
        total++;
    }

    fclose(fp);
    return total;
}

static int buscaSequencial(char *dicionario[], int tamanho, char *palavra)
{
    for (int i = 0; i < tamanho; i++) {
        if (strcmp(dicionario[i], palavra) == 0)
            return i;
    }
    return -1;
}

static int sorteia_palavra(int total_palavras)
{
    return rand() % total_palavras;
}


static void calcula_feedback(const char *secreta, const char *palpite, int *feedback)
{
    int contagem[26] = {0};

    for (int i = 0; i < WORD_LEN; i++)
        contagem[secreta[i] - 'A']++;

    /* 1a passada: acertos na posicao certa */
    for (int i = 0; i < WORD_LEN; i++) {
        if (palpite[i] == secreta[i]) {
            feedback[i] = FB_CORRETA;
            contagem[palpite[i] - 'A']--;
        } else {
            feedback[i] = -1; 
        }
    }

    for (int i = 0; i < WORD_LEN; i++) {
        if (feedback[i] == FB_CORRETA)
            continue;

        int idx = palpite[i] - 'A';
        if (contagem[idx] > 0) {
            feedback[i] = FB_EXISTE;
            contagem[idx]--;
        } else {
            feedback[i] = FB_AUSENTE;
        }
    }
}



static void exibe_tentativa(const char *palpite, const int *feedback)
{
    /* largura da borda: 2*WORD_LEN + 1 (ex.: 11 para WORD_LEN = 5) */
    int largura = 2 * WORD_LEN + 1;

    printf("+");
    for (int i = 0; i < largura; i++) printf("-");
    printf("+\n");

    printf("|");
    for (int i = 0; i < WORD_LEN; i++)
        printf(" %c", palpite[i]);
    printf(" |\n");

    printf("|");
    for (int i = 0; i < WORD_LEN; i++) {
        char simbolo;
        switch (feedback[i]) {
            case FB_CORRETA: simbolo = '^'; break;
            case FB_EXISTE:  simbolo = '!'; break;
            default:         simbolo = 'x'; break;
        }
        printf(" %c", simbolo);
    }
    printf(" |\n");

    printf("+");
    for (int i = 0; i < largura; i++) printf("-");
    printf("+\n");
}

static int le_linha(char *buffer, int tamanho)
{
    if (fgets(buffer, tamanho, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    remove_quebra_linha(buffer);
    return 1;
}



static void joga_uma_partida(char *dicionario[], int total_palavras)
{
    int indice_secreta = sorteia_palavra(total_palavras);
    char secreta[LINE_BUF];
    strcpy(secreta, dicionario[indice_secreta]);

    printf("\n========================================\n");
    printf("         TERMO - Descubra a palavra      \n");
    printf("========================================\n");
    printf("Acerte a palavra de %d letras em %d tentativas.\n", WORD_LEN, MAX_ATTEMPTS);
    printf("Feedback: ^ = posicao correta | ! = existe, posicao errada | x = nao existe\n\n");

    time_t inicio = time(NULL);
    int venceu = 0;
    int tentativa;

    for (tentativa = 1; tentativa <= MAX_ATTEMPTS; tentativa++) {
        char palpite[LINE_BUF];
        int valido = 0;

        while (!valido) {
            printf("Tentativa %d/%d - digite seu palpite: ", tentativa, MAX_ATTEMPTS);
            if (!le_linha(palpite, sizeof(palpite))) {
                printf("\nEntrada encerrada. Encerrando o jogo.\n");
                return;
            }

            if (!eh_palavra_valida(palpite)) {
                printf("Palpite invalido: deve conter exatamente %d letras.\n", WORD_LEN);
                continue;
            }

            para_maiusculas(palpite);

            if (buscaSequencial(dicionario, total_palavras, palpite) == -1) {
                printf("Essa palavra nao esta no dicionario. Tente outra (tentativa nao sera descontada).\n");
                continue;
            }

            valido = 1;
        }

        int feedback[WORD_LEN];
        calcula_feedback(secreta, palpite, feedback);
        exibe_tentativa(palpite, feedback);

        int acertou_tudo = 1;
        for (int i = 0; i < WORD_LEN; i++) {
            if (feedback[i] != FB_CORRETA) {
                acertou_tudo = 0;
                break;
            }
        }

        if (acertou_tudo) {
            venceu = 1;
            break;
        }
    }

    time_t fim = time(NULL);
    int tentativas_usadas = venceu ? tentativa : MAX_ATTEMPTS;
    int tempo_segundos = (int)difftime(fim, inicio);

    if (venceu) {
        printf("\nParabens! Voce acertou a palavra '%s' em %d tentativa(s)!\n",
               secreta, tentativas_usadas);
    } else {
        printf("\nVoce perdeu! A palavra era: %s\n", secreta);
    }

    /* Pede o nome do jogador e imprime a linha-resumo da partida */
    char nome[LINE_BUF];
    printf("\nDigite seu nome: ");
    if (!le_linha(nome, sizeof(nome)) || strlen(nome) == 0)
        strcpy(nome, "Jogador");

    printf("\n%s; %s; %d; %d\n", nome, secreta, tentativas_usadas, tempo_segundos);
}


static int jogar_novamente(void)
{
    char resposta[16];
    while (1) {
        printf("\nDeseja jogar novamente? (s/n): ");
        if (!le_linha(resposta, sizeof(resposta)))
            return 0;

        if (strlen(resposta) == 1) {
            char c = (char)tolower((unsigned char)resposta[0]);
            if (c == 's') return 1;
            if (c == 'n') return 0;
        }
        printf("Resposta invalida. Digite 's' para sim ou 'n' para nao.\n");
    }
}


int main(void)
{
    srand((unsigned int)time(NULL));

    static char palavras_buffer[MAX_WORDS][LINE_BUF];
    int total_palavras = carrega_dicionario(DICT_FILE, palavras_buffer, MAX_WORDS);

    if (total_palavras <= 0) {
        fprintf(stderr, "Erro: nenhuma palavra valida carregada de '%s'\n", DICT_FILE);
        return 1;
    }

    /* vetor de ponteiros, para casar com a assinatura de buscaSequencial */
    char *dicionario[MAX_WORDS];
    for (int i = 0; i < total_palavras; i++)
        dicionario[i] = palavras_buffer[i];

    printf("Carregadas %d palavras de '%s'.\n", total_palavras, DICT_FILE);

    do {
        joga_uma_partida(dicionario, total_palavras);
    } while (jogar_novamente());

    printf("\nObrigado por jogar!\n");
    return 0;
}