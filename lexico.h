/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — Módulo Léxico
 * ============================================================================
 * Responsabilidade: ler caracteres e devolver o PRÓXIMO token.
 * É a 1ª fase do compilador (Analisador Léxico).
 * ============================================================================
 */
#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

/* Texto do lexema atual (ex.: "soma", "42", "int") */
extern char yytext[256];

/* Abre o arquivo-fonte; se fp==NULL, lê stdin */
void lexico_iniciar(FILE* fp);

/* Devolve o código do token (ver tokens.h) ou FIM_ARQUIVO */
int yylex(void);

#endif
