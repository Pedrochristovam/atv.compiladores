/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — lexico.c
 * ============================================================================
 * Implementação manual do analisador léxico (equivalente ao Flex).
 *
 * Ideia: automato simples por casos —
 *   1) pula espaços/comentários
 *   2) reconhece operadores de 1 ou 2 caracteres (==, &&, ...)
 *   3) reconhece números (int/float), strings, chars e identificadores
 *   4) identifica palavras reservadas (if, while, func, ...)
 *
 * Por que módulo separado? O enunciado pede cada fase em pacote próprio.
 * ============================================================================
 */
#include "lexico.h"
#include "tokens.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char yytext[256];
static FILE* entrada = NULL;
static int caracter_atual = 0;
static int tem_caracter = 0;

void lexico_iniciar(FILE* fp) {
    entrada = fp ? fp : stdin;
    tem_caracter = 0;
}

static int proximo_char(void) {
    if (tem_caracter) {
        tem_caracter = 0;
        return caracter_atual;
    }
    return fgetc(entrada);
}

static void devolver_char(int c) {
    if (c == EOF) return;
    caracter_atual = c;
    tem_caracter = 1;
}

static int palavra_reservada(const char* s) {
    if (strcmp(s, "if") == 0) return IF;
    if (strcmp(s, "else") == 0) return ELSE;
    if (strcmp(s, "for") == 0) return FOR;
    if (strcmp(s, "while") == 0) return WHILE;
    if (strcmp(s, "switch") == 0) return SWITCH;
    if (strcmp(s, "case") == 0) return CASE;
    if (strcmp(s, "default") == 0) return DEFAULT;
    if (strcmp(s, "break") == 0) return BREAK;
    if (strcmp(s, "return") == 0) return RETURN;
    if (strcmp(s, "var") == 0) return VAR;
    if (strcmp(s, "func") == 0) return FUNC;
    if (strcmp(s, "main") == 0) return MAIN;
    if (strcmp(s, "print") == 0) return PRINT;
    if (strcmp(s, "input") == 0) return INPUT;
    if (strcmp(s, "true") == 0 || strcmp(s, "false") == 0) return BOOL_LITERAL;
    if (strcmp(s, "int") == 0 || strcmp(s, "float") == 0 ||
        strcmp(s, "char") == 0 || strcmp(s, "bool") == 0 ||
        strcmp(s, "string") == 0) return TIPO;
    return IDENTIFICADOR;
}

int yylex(void) {
    int c;
    int i;

    while (1) {
        c = proximo_char();
        if (c == EOF) return FIM_ARQUIVO;

        /* espaços */
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;

        /* comentário // */
        if (c == '/') {
            int n = proximo_char();
            if (n == '/') {
                while ((c = proximo_char()) != EOF && c != '\n') { }
                continue;
            }
            if (n == '*') {
                int prev = 0;
                while ((c = proximo_char()) != EOF) {
                    if (prev == '*' && c == '/') break;
                    prev = c;
                }
                continue;
            }
            devolver_char(n);
            yytext[0] = '/'; yytext[1] = '\0';
            return DIVISAO;
        }

        /* operadores de 2 caracteres */
        if (c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || c == '|') {
            int n = proximo_char();
            if (c == '=' && n == '=') { strcpy(yytext, "=="); return IGUAL; }
            if (c == '!' && n == '=') { strcpy(yytext, "!="); return DIFERENTE; }
            if (c == '<' && n == '=') { strcpy(yytext, "<="); return MENOR_IGUAL; }
            if (c == '>' && n == '=') { strcpy(yytext, ">="); return MAIOR_IGUAL; }
            if (c == '&' && n == '&') { strcpy(yytext, "&&"); return E_LOGICO; }
            if (c == '|' && n == '|') { strcpy(yytext, "||"); return OU_LOGICO; }
            devolver_char(n);
            yytext[0] = (char)c; yytext[1] = '\0';
            if (c == '=') return ATRIBUICAO;
            if (c == '!') return NOT;
            if (c == '<') return MENOR;
            if (c == '>') return MAIOR;
            return ERRO;
        }

        /* símbolos simples */
        yytext[0] = (char)c; yytext[1] = '\0';
        if (c == '+') return SOMA;
        if (c == '-') return SUBTRACAO;
        if (c == '*') return MULTIPLICACAO;
        if (c == '(') return ABRE_PARENTESE;
        if (c == ')') return FECHA_PARENTESE;
        if (c == '{') return ABRE_CHAVES;
        if (c == '}') return FECHA_CHAVES;
        if (c == ';') return PONTO_VIRGULA;
        if (c == ',') return VIRGULA;
        if (c == ':') return DOIS_PONTOS;

        /* string */
        if (c == '"') {
            i = 0;
            yytext[i++] = '"';
            while ((c = proximo_char()) != EOF && c != '"' && c != '\n') {
                if (i < 254) yytext[i++] = (char)c;
            }
            if (c == '"') yytext[i++] = '"';
            yytext[i] = '\0';
            return STRING_LITERAL;
        }

        /* char */
        if (c == '\'') {
            i = 0;
            yytext[i++] = '\'';
            c = proximo_char();
            if (c != EOF && c != '\'') yytext[i++] = (char)c;
            c = proximo_char();
            if (c == '\'') yytext[i++] = '\'';
            yytext[i] = '\0';
            return CHAR_LITERAL;
        }

        /* número int ou float */
        if (isdigit(c)) {
            i = 0;
            yytext[i++] = (char)c;
            while ((c = proximo_char()) != EOF && isdigit(c)) {
                if (i < 254) yytext[i++] = (char)c;
            }
            if (c == '.') {
                yytext[i++] = '.';
                int tem_digito = 0;
                while ((c = proximo_char()) != EOF && isdigit(c)) {
                    tem_digito = 1;
                    if (i < 254) yytext[i++] = (char)c;
                }
                devolver_char(c);
                yytext[i] = '\0';
                return tem_digito ? FLOAT_LITERAL : ERRO;
            }
            devolver_char(c);
            yytext[i] = '\0';
            return NUMERO;
        }

        /* identificador / palavra reservada */
        if (isalpha(c) || c == '_') {
            i = 0;
            yytext[i++] = (char)c;
            while ((c = proximo_char()) != EOF && (isalnum(c) || c == '_')) {
                if (i < 254) yytext[i++] = (char)c;
            }
            devolver_char(c);
            yytext[i] = '\0';
            return palavra_reservada(yytext);
        }

        return ERRO;
    }
}
