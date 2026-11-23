/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — parser.c
 * ============================================================================
 * Técnica: Análise Sintática Descendente Recursiva (Recursive Descent).
 * Gramática (resumo):
 *   programa     := (funcao | main)*
 *   funcao       := func ID ( params? ) : TIPO bloco
 *   main         := main ( ) bloco
 *   bloco        := { (declaracao | comando)* }
 *   expressao    := hierarquia com || && relacional + - * / e unários
 *
 * Correções vs versão antiga:
 *   - while usa token WHILE (não FOR)
 *   - for no formato do PDF: for (init; cond; incr)
 *   - operadores *, /, ==, !=, <, >, &&, ||
 * ============================================================================
 */
#include "parser.h"
#include "tokens.h"
#include "lexico.h"
#include "semantica.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int token_atual;

static void proximo(void) { token_atual = yylex(); }

static void erro(const char* msg) {
    fprintf(stderr, "Erro de sintaxe: %s (token: '%s')\n", msg, yytext);
    exit(1);
}

static void esperar(int tok, const char* msg) {
    if (token_atual != tok) erro(msg);
    proximo();
}

/* protótipos */
static NoArvore* bloco(void);
static NoArvore* declaracao(void);
static NoArvore* comando(void);
static NoArvore* atribuicao_completa(void);      /* id = expr ; */
static NoArvore* atribuicao_sem_pv(const char* nome); /* já consumiu id e está em '=' */
static NoArvore* expressao(void);
static NoArvore* chamada_funcao(const char* nome);

/* ---------- Expressões (precedência) ---------- */
/* expr → or → and → rel → soma → mult → unario → primario */

static NoArvore* primario(void) {
    NoArvore* no = NULL;

    if (token_atual == IDENTIFICADOR) {
        char nome[128];
        strncpy(nome, yytext, 127);
        proximo();
        if (token_atual == ABRE_PARENTESE) {
            proximo();
            return chamada_funcao(nome);
        }
        no = criarNo("IDENTIFICADOR", nome);
        if (!sem_var_existe(nome)) {
            fprintf(stderr, "Erro semantico: variavel '%s' nao declarada\n", nome);
            exit(1);
        }
        const char* t = sem_tipo_var(nome);
        if (t) strncpy(no->tipoVariavel, t, 19);
        return no;
    }

    if (token_atual == NUMERO) {
        no = criarNo("NUMERO", yytext);
        strcpy(no->tipoVariavel, "int");
        proximo();
        return no;
    }
    if (token_atual == FLOAT_LITERAL) {
        no = criarNo("FLOAT_LITERAL", yytext);
        strcpy(no->tipoVariavel, "float");
        proximo();
        return no;
    }
    if (token_atual == BOOL_LITERAL) {
        no = criarNo("BOOL_LITERAL", yytext);
        strcpy(no->tipoVariavel, "bool");
        proximo();
        return no;
    }
    if (token_atual == STRING_LITERAL) {
        no = criarNo("STRING_LITERAL", yytext);
        strcpy(no->tipoVariavel, "string");
        proximo();
        return no;
    }
    if (token_atual == CHAR_LITERAL) {
        no = criarNo("CHAR_LITERAL", yytext);
        strcpy(no->tipoVariavel, "char");
        proximo();
        return no;
    }
    if (token_atual == ABRE_PARENTESE) {
        proximo();
        no = expressao();
        esperar(FECHA_PARENTESE, "esperado ')'");
        return no;
    }

    erro("expressao primaria invalida");
    return NULL;
}

static NoArvore* unario(void) {
    if (token_atual == NOT) {
        proximo();
        NoArvore* no = criarNo("NOT", "");
        adicionarFilho(no, unario());
        strcpy(no->tipoVariavel, "bool");
        return no;
    }
    if (token_atual == SUBTRACAO) {
        proximo();
        NoArvore* no = criarNo("NEGATIVO", "");
        adicionarFilho(no, unario());
        return no;
    }
    return primario();
}

static NoArvore* multiplicativo(void) {
    NoArvore* esq = unario();
    while (token_atual == MULTIPLICACAO || token_atual == DIVISAO) {
        const char* tipo = (token_atual == MULTIPLICACAO) ? "MULTIPLICACAO" : "DIVISAO";
        proximo();
        NoArvore* no = criarNo(tipo, "");
        adicionarFilho(no, esq);
        adicionarFilho(no, unario());
        esq = no;
    }
    return esq;
}

static NoArvore* aditivo(void) {
    NoArvore* esq = multiplicativo();
    while (token_atual == SOMA || token_atual == SUBTRACAO) {
        const char* tipo = (token_atual == SOMA) ? "SOMA" : "SUBTRACAO";
        proximo();
        NoArvore* no = criarNo(tipo, "");
        adicionarFilho(no, esq);
        adicionarFilho(no, multiplicativo());
        esq = no;
    }
    return esq;
}

static NoArvore* relacional(void) {
    NoArvore* esq = aditivo();
    while (token_atual == IGUAL || token_atual == DIFERENTE ||
           token_atual == MAIOR || token_atual == MENOR ||
           token_atual == MAIOR_IGUAL || token_atual == MENOR_IGUAL) {
        const char* tipo = "IGUAL";
        if (token_atual == DIFERENTE) tipo = "DIFERENTE";
        else if (token_atual == MAIOR) tipo = "MAIOR";
        else if (token_atual == MENOR) tipo = "MENOR";
        else if (token_atual == MAIOR_IGUAL) tipo = "MAIOR_IGUAL";
        else if (token_atual == MENOR_IGUAL) tipo = "MENOR_IGUAL";
        else tipo = "IGUAL";
        proximo();
        NoArvore* no = criarNo(tipo, "");
        adicionarFilho(no, esq);
        adicionarFilho(no, aditivo());
        strcpy(no->tipoVariavel, "bool");
        esq = no;
    }
    return esq;
}

static NoArvore* logico_e(void) {
    NoArvore* esq = relacional();
    while (token_atual == E_LOGICO) {
        proximo();
        NoArvore* no = criarNo("E_LOGICO", "");
        adicionarFilho(no, esq);
        adicionarFilho(no, relacional());
        strcpy(no->tipoVariavel, "bool");
        esq = no;
    }
    return esq;
}

static NoArvore* expressao(void) {
    NoArvore* esq = logico_e();
    while (token_atual == OU_LOGICO) {
        proximo();
        NoArvore* no = criarNo("OU_LOGICO", "");
        adicionarFilho(no, esq);
        adicionarFilho(no, logico_e());
        strcpy(no->tipoVariavel, "bool");
        esq = no;
    }
    return esq;
}

static NoArvore* chamada_funcao(const char* nome) {
    NoArvore* no = criarNo("CHAMADA_FUNCAO", "");
    adicionarFilho(no, criarNo("IDENTIFICADOR", nome));
    NoArvore* args = criarNo("ARGUMENTOS", "");
    int n = 0;
    if (token_atual != FECHA_PARENTESE) {
        while (1) {
            adicionarFilho(args, expressao());
            n++;
            if (token_atual == VIRGULA) proximo();
            else break;
        }
    }
    esperar(FECHA_PARENTESE, "faltou ')' na chamada de funcao");
    no->numArgs = n;
    adicionarFilho(no, args);

    if (!sem_funcao_existe(nome)) {
        fprintf(stderr, "Erro semantico: funcao '%s' nao declarada\n", nome);
        exit(1);
    }
    int esp = sem_num_params(nome);
    if (esp >= 0 && n != esp) {
        fprintf(stderr, "Erro semantico: '%s' espera %d args, veio %d\n", nome, esp, n);
        exit(1);
    }
    const char* tr = sem_tipo_funcao(nome);
    if (tr) strncpy(no->tipoVariavel, tr, 19);
    return no;
}

/* ---------- Comandos ---------- */

static NoArvore* atribuicao_sem_pv(const char* nome) {
    /* token atual deve ser '=' */
    NoArvore* no = criarNo("ATRIBUICAO", "");
    if (!sem_var_existe(nome)) {
        fprintf(stderr, "Erro semantico: variavel '%s' nao declarada\n", nome);
        exit(1);
    }
    NoArvore* id = criarNo("IDENTIFICADOR", nome);
    const char* t = sem_tipo_var(nome);
    if (t) strncpy(id->tipoVariavel, t, 19);
    adicionarFilho(no, id);
    esperar(ATRIBUICAO, "esperado '='");
    adicionarFilho(no, expressao());
    return no;
}

static NoArvore* atribuicao_completa(void) {
    if (token_atual != IDENTIFICADOR) erro("esperado identificador na atribuicao");
    char nome[128];
    strncpy(nome, yytext, 127);
    proximo();
    NoArvore* no = atribuicao_sem_pv(nome);
    esperar(PONTO_VIRGULA, "esperado ';' apos atribuicao");
    return no;
}

static NoArvore* comandoIf(void) {
    NoArvore* no = criarNo("IF", "");
    proximo(); /* if */
    esperar(ABRE_PARENTESE, "esperado '(' apos if");
    adicionarFilho(no, expressao());
    esperar(FECHA_PARENTESE, "esperado ')' apos condicao do if");
    adicionarFilho(no, bloco());
    if (token_atual == ELSE) {
        proximo();
        adicionarFilho(no, bloco());
    }
    return no;
}

static NoArvore* comandoWhile(void) {
    NoArvore* no = criarNo("WHILE", "");
    proximo();
    esperar(ABRE_PARENTESE, "esperado '(' apos while");
    adicionarFilho(no, expressao());
    esperar(FECHA_PARENTESE, "esperado ')' apos condicao do while");
    adicionarFilho(no, bloco());
    return no;
}

static NoArvore* comandoFor(void) {
    /*
     * Formato do PDF: for (inicializacao; condicao; incremento) bloco
     * Ex.: for (i = 0; i < 10; i = i + 1) { ... }
     */
    NoArvore* no = criarNo("FOR", "");
    proximo();
    esperar(ABRE_PARENTESE, "esperado '(' apos for");

    /* init: id = expr ; */
    adicionarFilho(no, atribuicao_completa());

    /* condição */
    adicionarFilho(no, expressao());
    esperar(PONTO_VIRGULA, "esperado ';' apos condicao do for");

    /* incremento: id = expr   (sem ; obrigatório antes do ')') */
    if (token_atual != IDENTIFICADOR) erro("esperado incremento no for");
    char nome[128];
    strncpy(nome, yytext, 127);
    proximo();
    adicionarFilho(no, atribuicao_sem_pv(nome));

    esperar(FECHA_PARENTESE, "esperado ')' no for");
    adicionarFilho(no, bloco());
    return no;
}

static NoArvore* comandoPrint(void) {
    NoArvore* no = criarNo("PRINT", "");
    proximo();
    esperar(ABRE_PARENTESE, "esperado '(' apos print");
    adicionarFilho(no, expressao());
    esperar(FECHA_PARENTESE, "esperado ')' no print");
    esperar(PONTO_VIRGULA, "esperado ';' apos print");
    return no;
}

static NoArvore* comandoInput(void) {
    NoArvore* no = criarNo("INPUT", "");
    proximo();
    esperar(ABRE_PARENTESE, "esperado '(' apos input");
    if (token_atual != IDENTIFICADOR) erro("esperado identificador no input");
    if (!sem_var_existe(yytext)) {
        fprintf(stderr, "Erro semantico: variavel '%s' nao declarada no input\n", yytext);
        exit(1);
    }
    NoArvore* id = criarNo("IDENTIFICADOR", yytext);
    const char* t = sem_tipo_var(yytext);
    if (t) strncpy(id->tipoVariavel, t, 19);
    adicionarFilho(no, id);
    proximo();
    esperar(FECHA_PARENTESE, "esperado ')' no input");
    esperar(PONTO_VIRGULA, "esperado ';' apos input");
    return no;
}

static NoArvore* comandoReturn(void) {
    NoArvore* no = criarNo("RETURN", "");
    proximo();
    adicionarFilho(no, expressao());
    esperar(PONTO_VIRGULA, "esperado ';' apos return");
    return no;
}

static NoArvore* comando(void) {
    if (token_atual == IDENTIFICADOR) {
        char nome[128];
        strncpy(nome, yytext, 127);
        proximo();
        if (token_atual == ABRE_PARENTESE) {
            proximo();
            NoArvore* call = chamada_funcao(nome);
            /* chamada como statement: permite ; opcional */
            if (token_atual == PONTO_VIRGULA) proximo();
            return call;
        }
        NoArvore* atr = atribuicao_sem_pv(nome);
        esperar(PONTO_VIRGULA, "esperado ';' apos atribuicao");
        return atr;
    }
    if (token_atual == IF) return comandoIf();
    if (token_atual == WHILE) return comandoWhile();
    if (token_atual == FOR) return comandoFor();
    if (token_atual == PRINT) return comandoPrint();
    if (token_atual == INPUT) return comandoInput();
    if (token_atual == RETURN) return comandoReturn();
    erro("comando invalido");
    return NULL;
}

static NoArvore* declaracao(void) {
    NoArvore* no = criarNo("DECLARACAO", "");
    proximo(); /* var */
    if (token_atual != IDENTIFICADOR) erro("esperado nome apos 'var'");
    char nome[128];
    strncpy(nome, yytext, 127);
    proximo();
    esperar(DOIS_PONTOS, "esperado ':' apos nome da variavel");
    if (token_atual != TIPO) erro("esperado tipo apos ':'");
    char tipo[20];
    strncpy(tipo, yytext, 19);
    proximo();

    sem_declarar_var(nome, tipo);

    NoArvore* id = criarNo("IDENTIFICADOR", nome);
    strncpy(id->tipoVariavel, tipo, 19);
    adicionarFilho(no, id);
    adicionarFilho(no, criarNo("TIPO", tipo));

    if (token_atual == ATRIBUICAO) {
        proximo();
        adicionarFilho(no, expressao());
    }
    esperar(PONTO_VIRGULA, "esperado ';' no fim da declaracao");
    return no;
}

static NoArvore* bloco(void) {
    NoArvore* no = criarNo("BLOCO", "");
    esperar(ABRE_CHAVES, "esperado '{'");
    sem_abrir_escopo();
    while (token_atual == VAR || token_atual == IDENTIFICADOR ||
           token_atual == IF || token_atual == WHILE || token_atual == FOR ||
           token_atual == PRINT || token_atual == INPUT || token_atual == RETURN) {
        if (token_atual == VAR) adicionarFilho(no, declaracao());
        else adicionarFilho(no, comando());
    }
    esperar(FECHA_CHAVES, "esperado '}'");
    sem_fechar_escopo();
    return no;
}

static NoArvore* parametros(void) {
    NoArvore* no = criarNo("PARAMETROS", "");
    int count = 0;
    while (1) {
        if (token_atual != IDENTIFICADOR) erro("esperado nome de parametro");
        char nome[128];
        strncpy(nome, yytext, 127);
        proximo();
        esperar(DOIS_PONTOS, "esperado ':' no parametro");
        if (token_atual != TIPO) erro("esperado tipo do parametro");
        char tipo[20];
        strncpy(tipo, yytext, 19);
        proximo();

        sem_declarar_var(nome, tipo);
        adicionarFilho(no, criarNo("IDENTIFICADOR", nome));
        NoArvore* t = criarNo("TIPO", tipo);
        adicionarFilho(no, t);
        count++;

        if (token_atual == VIRGULA) proximo();
        else break;
    }
    no->numArgs = count;
    return no;
}

static NoArvore* funcao(void) {
    NoArvore* no = criarNo("FUNCAO", "");
    proximo(); /* func */
    if (token_atual != IDENTIFICADOR) erro("esperado nome da funcao");
    char nome[128];
    strncpy(nome, yytext, 127);
    adicionarFilho(no, criarNo("IDENTIFICADOR", nome));
    proximo();

    esperar(ABRE_PARENTESE, "esperado '(' apos nome da funcao");
    sem_abrir_escopo();

    NoArvore* params = NULL;
    int nparams = 0;
    if (token_atual != FECHA_PARENTESE) {
        params = parametros();
        nparams = params->numArgs;
    }
    esperar(FECHA_PARENTESE, "esperado ')' dos parametros");
    esperar(DOIS_PONTOS, "esperado ':' apos parametros");
    if (token_atual != TIPO) erro("esperado tipo de retorno");
    char tipoRet[20];
    strncpy(tipoRet, yytext, 19);
    proximo();

    sem_declarar_funcao(nome, tipoRet, nparams);
    no->numArgs = nparams;

    if (params) adicionarFilho(no, params);
    else adicionarFilho(no, criarNo("PARAMETROS", ""));
    adicionarFilho(no, criarNo("TIPO", tipoRet));
    adicionarFilho(no, bloco());

    sem_fechar_escopo();
    return no;
}

static NoArvore* main_funcao(void) {
    NoArvore* no = criarNo("MAIN", "");
    proximo();
    esperar(ABRE_PARENTESE, "esperado '(' apos main");
    esperar(FECHA_PARENTESE, "esperado ')' apos main");
    adicionarFilho(no, bloco());
    return no;
}

NoArvore* parse_programa(void) {
    sem_iniciar();
    sem_abrir_escopo(); /* escopo global */

    proximo();
    NoArvore* raiz = criarNo("PROGRAMA", "");

    while (token_atual == FUNC || token_atual == MAIN) {
        if (token_atual == FUNC) adicionarFilho(raiz, funcao());
        else adicionarFilho(raiz, main_funcao());
    }

    if (token_atual != FIM_ARQUIVO && token_atual != 0)
        erro("tokens extras apos o fim do programa");

    sem_fechar_escopo();
    fprintf(stderr, "Analise sintatica: OK\n");
    return raiz;
}
