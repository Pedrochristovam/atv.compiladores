/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — Módulo Sintático (2ª fase)
 * ============================================================================
 * Parser descendente recursivo: cada regra da gramática vira uma função C.
 * Consome tokens do léxico e monta a AST.
 * ============================================================================
 */
#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include <stdio.h>

/* Analisa o programa a partir do arquivo já aberto no léxico */
NoArvore* parse_programa(void);

#endif
