/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — AST (Árvore Sintática Abstrata)
 * ============================================================================
 * Depois do parser, o programa vira uma árvore de nós.
 * Cada nó guarda: tipo (ex: IF, SOMA), valor (ex: nome da var) e tipo de dado.
 *
 * filho   = primeiro filho
 * proximo = próximo irmão (lista de irmãos)
 *
 * Exemplo: a + b  →  nó SOMA com filhos IDENTIFICADOR(a) e IDENTIFICADOR(b)
 * ============================================================================
 */
#ifndef AST_H
#define AST_H

typedef struct NoArvore {
    char tipo[50];
    char valor[128];
    char tipoVariavel[20]; /* int, float, bool, ... — útil na semântica/codegen */
    int  offset;           /* deslocamento na pilha (preenchido no codegen) */
    int  numArgs;          /* para funções: quantidade de parâmetros */
    struct NoArvore* filho;
    struct NoArvore* proximo;
} NoArvore;

NoArvore* criarNo(const char* tipo, const char* valor);
void adicionarFilho(NoArvore* pai, NoArvore* filho);
void imprimirArvore(NoArvore* raiz, int nivel);
void liberarArvore(NoArvore* raiz);

#endif
