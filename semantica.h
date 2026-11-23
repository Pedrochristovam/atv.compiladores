/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — Módulo Semântico (3ª fase)
 * ============================================================================
 * Responsabilidades:
 *  - Tabela de símbolos com pilha de escopos (locais / aninhados)
 *  - Tabela de funções (nome + tipo de retorno + nº de parâmetros)
 *  - Detectar: variável/função não declarada ou duplicada
 *  - Checagem básica de tipos em atribuições e expressões
 *
 * Separado do parser para atender o enunciado (módulo por fase).
 * ============================================================================
 */
#ifndef SEMANTICA_H
#define SEMANTICA_H

#include "ast.h"

void sem_iniciar(void);
void sem_abrir_escopo(void);
void sem_fechar_escopo(void);

void sem_declarar_var(const char* nome, const char* tipo);
int  sem_var_existe(const char* nome);
const char* sem_tipo_var(const char* nome);

void sem_declarar_funcao(const char* nome, const char* tipoRetorno, int numParams);
int  sem_funcao_existe(const char* nome);
const char* sem_tipo_funcao(const char* nome);
int  sem_num_params(const char* nome);

/* Compatibilidade / coerção simples entre tipos */
int  sem_tipos_compativeis(const char* t1, const char* t2);
const char* sem_tipo_resultado_binario(const char* t1, const char* t2, const char* op);

/* Percorre a AST e valida usos (chamado após o parse) */
void analisar_semantica(NoArvore* raiz);

#endif
