#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>/*isalpha por enquanto*/

#define MAX_WORDS     2000
#define WORD_LEN      5
#define MAX_ATTEMPTS  6
#define LINE_BUF      256 /*tamanho fixo dos buffers*/

static const char *DICT_FILE = "sem_acentos.txt";

/*FB = feedback*/
#define FB_CORRETA  2  /*FB correto*/
#define FB_EXISTE   1  /*FB existente*/
#define FB_AUSENTE  0  /*FB inexistente*/


static void para_maiusculas(char *s)/*ela modifica a string original diretamente na memória*/
{
    for (int i = 0; s[i] != '\0'; i++)/*\0 -> caractere que marca o fim de toda string em C*/
        s[i] = (char)toupper((unsigned char)s[i]);/*toupper converte p maiúsculo automaticamente e converte-se pra unsigned char primeiro, garantindo que o valor é sempre positivo (0–255)*/ 
}

static int eh_palavra_valida(const char *s)
{
    int len = (int)strlen(s);/*strlen -> tamanho da string*/
    if (len != WORD_LEN)
        return 0;/*se for diferente de 5 retorna 0*/

    for (int i = 0; i < len; i++) {
        if (!isalpha((unsigned char)s[i])) /*isalpha verifica se é uma letra do alfabeto e o ! é o NOT*/
            return 0;
    }
    return 1;
}

static void remove_quebra_linha(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))/*enquanto o último caractere for \n ou \r, substitui por \0*/
        s[--len] = '\0';/*decrementa o len*/
}

static int carrega_dicionario(const char *arquivo, char palavras[][LINE_BUF], int max)/*Parâmetro char palavras[][LINE_BUF] é a forma de receber uma matriz como parâmetro em C*/
{
    FILE *fp = fopen(arquivo, "r");/*abre o arquivo em modo leitura*/
    if (!fp) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", arquivo);
        return -1;
    }

    int total = 0;
    char linha[LINE_BUF];

    while (total < max && fgets(linha, sizeof(linha), fp)) {/*max = 2000    */
        remove_quebra_linha(linha);

        if (strlen(linha) == 0)/*pula se ficou vazia*/
            continue; 

        if (!eh_palavra_valida(linha)) {/*pula se não for uma palavra válida (5 letras)*/
            continue;
        }

        para_maiusculas(linha);
        strcpy(palavras[total], linha);/*só então converte pra maiúscula e copia pra palavras[total] com strcpy */
        total++;
    }

    fclose(fp);
    return total;
}

static int buscaSequencial(char *dicionario[], int tamanho, char *palavra)
{
    for (int i = 0; i < tamanho; i++) {
        if (strcmp(dicionario[i], palavra) == 0) /*strcmp compara duas strings em sequência, e retorna 0 quando idênticas*/
            return i; /*retorna o índice*/
    }
    return -1;
}

static int sorteia_palavra(int total_palavras)
{
    return rand() % total_palavras; /*rand() gera numeros aleatórios nesse intervalo de total_palavras*/
}


static void calcula_feedback(const char *secreta, const char *palpite, int *feedback) /*const definiu secreta como variavel de leitura, impedindo que seu valor seja alterado após inicialização*/
{
    int contagem[26] = {0}; /*26 letras do alfabeto*/

    for (int i = 0; i < WORD_LEN; i++)/*conta as letras da palavra secreta*/
        contagem[secreta[i] - 'A']++;/*converte uma letra em um índice*/

    /* 1a passada: acertos na posicao certa */
    for (int i = 0; i < WORD_LEN; i++) {/*compara posição por posição*/
        if (palpite[i] == secreta[i]) {
            feedback[i] = FB_CORRETA;/*Se a letra do palpite bate exatamente com a da secreta naquela posição -> marca FB_CORRETA*/
            contagem[palpite[i] - 'A']--;/*e decrementa o contador daquela letra, não podendo ser contada depois*/
        } else {
            feedback[i] = -1; 
        }
    }
    /*2a passada: existe em posição errada*/
    for (int i = 0; i < WORD_LEN; i++) {
        if (feedback[i] == FB_CORRETA)
            continue;/*as letras que foram pulam com o continue*/

        int idx = palpite[i] - 'A';
        if (contagem[idx] > 0) {
            feedback[i] = FB_EXISTE;/*Se sobrar -> FB_EXISTE, e desconta mais uma*/
            contagem[idx]--;
        } else {
            feedback[i] = FB_AUSENTE;/*Se não sobrar mais nenhuma -> FB_AUSENTE*/
        }
    }
}



static void exibe_tentativa(const char *palpite, const int *feedback)
{
    int largura = 2 * WORD_LEN + 1;/*calcula quantos traços '-' a borda precisa ter*/

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
    if (fgets(buffer, tamanho, stdin) == NULL) {/*retornou NULL -> entrada acabou */
        buffer[0] = '\0';
        return 0;
    }
    remove_quebra_linha(buffer);/*Se leu normal, limpa a quebra de linha e retorna 1*/
    return 1;
}



static void joga_uma_partida(char *dicionario[], int total_palavras)
{
    int indice_secreta = sorteia_palavra(total_palavras);/*sorteia o índice*/
    char secreta[LINE_BUF];
    strcpy(secreta, dicionario[indice_secreta]);/*copia a palavra*/

    printf("\n========================================\n");
    printf("         TERMO - Descubra a palavra      \n");
    printf("========================================\n");
    printf("Acerte a palavra de %d letras em %d tentativas.\n", WORD_LEN, MAX_ATTEMPTS);
    printf("Feedback: ^ = posicao correta | ! = existe, posicao errada | x = nao existe\n\n");

    time_t inicio = time(NULL);/*time(NULL) retorna o horário atual*/
    int venceu = 0;
    int tentativa;

    for (tentativa = 1; tentativa <= MAX_ATTEMPTS; tentativa++) {/*loop para 6 tentativas*/
        char palpite[LINE_BUF];
        int valido = 0;

        while (!valido) {/*fica pedindo tentativa até receber um válido(5 letras)*/
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
    int tentativas_usadas = venceu ? tentativa : MAX_ATTEMPTS; /*se venceu, usa o número da tentativa em que acertou; se perdeu, usa o máximo*/
    int tempo_segundos = (int)difftime(fim, inicio);/*calcula a diferença em segundos entre os dois instantes*/

    if (venceu) {
        printf("\nParabens! Voce acertou a palavra '%s' em %d tentativa(s)!\n",
               secreta, tentativas_usadas);
    } else {
        printf("\nVoce perdeu! A palavra era: %s\n", secreta);
    }

    /* Pede o nome do jogador e imprime o resumo da partida */
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
    srand((unsigned int)time(NULL));/*usa o horário atual do sistema, para o rand devolver sempre números aleatórios*/

    static char palavras_buffer[MAX_WORDS][LINE_BUF];/*declara a matriz que vai guardar até 2000 palavras (uma por linha, cada uma com até 256 bytes) static para evitar overflow*/
    int total_palavras = carrega_dicionario(DICT_FILE, palavras_buffer, MAX_WORDS);

    if (total_palavras <= 0) {
        fprintf(stderr, "Erro: nenhuma palavra valida carregada de '%s'\n", DICT_FILE);
        return 1;
    }

    /* vetor de ponteiros, para casar com a assinatura de buscaSequencial */
    char *dicionario[MAX_WORDS];/*cada posição aponta pra uma das linhas de palavras_buffer*/
    for (int i = 0; i < total_palavras; i++)
        dicionario[i] = palavras_buffer[i];

    printf("Carregadas %d palavras de '%s'.\n", total_palavras, DICT_FILE);

    do {
        joga_uma_partida(dicionario, total_palavras);
    } while (jogar_novamente());

    printf("\nObrigado por jogar!\n");
    return 0;
}