/*
 * ============================================================================
 * LEGENDA (APRESENTAÇÃO) — semantica.c
 * ============================================================================
 * Pilha de escopos: cada { ... } abre um escopo; ao sair, as vars locais somem.
 * Assim "x" dentro de um if não conflita com "x" de outro bloco (se escopos
 * diferentes), e não pode redeclarar no MESMO escopo.
 *
 * analisar_semantica() caminha a AST e confere usos vs declarações/tipos.
 * ============================================================================
 */
#include "semantica.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS   128
#define MAX_ESCOPE 64
#define MAX_FUNCS  64

typedef struct {
    char nome[100];
    char tipo[20];
} Variavel;

typedef struct {
    Variavel vars[MAX_VARS];
    int total;
} Escopo;

typedef struct {
    char nome[100];
    char tipoRetorno[20];
    int  numParams;
} Funcao;

static Escopo pilha[MAX_ESCOPE];
static int topo = -1;
static Funcao funcs[MAX_FUNCS];
static int totalFuncs = 0;

void sem_iniciar(void) {
    topo = -1;
    totalFuncs = 0;
}

void sem_abrir_escopo(void) {
    if (topo + 1 >= MAX_ESCOPE) {
        fprintf(stderr, "Erro semantico: muitos escopos aninhados\n");
        exit(1);
    }
    topo++;
    pilha[topo].total = 0;
}

void sem_fechar_escopo(void) {
    if (topo < 0) {
        fprintf(stderr, "Erro semantico: fechar escopo inexistente\n");
        exit(1);
    }
    topo--;
}

static int var_no_escopo_atual(const char* nome) {
    if (topo < 0) return 0;
    Escopo* e = &pilha[topo];
    for (int i = 0; i < e->total; i++)
        if (strcmp(e->vars[i].nome, nome) == 0) return 1;
    return 0;
}

void sem_declarar_var(const char* nome, const char* tipo) {
    if (topo < 0) {
        fprintf(stderr, "Erro semantico: declarar '%s' sem escopo\n", nome);
        exit(1);
    }
    if (var_no_escopo_atual(nome)) {
        fprintf(stderr, "Erro semantico: variavel '%s' ja declarada neste escopo\n", nome);
        exit(1);
    }
    Escopo* e = &pilha[topo];
    if (e->total >= MAX_VARS) {
        fprintf(stderr, "Erro semantico: limite de variaveis no escopo\n");
        exit(1);
    }
    strncpy(e->vars[e->total].nome, nome, 99);
    strncpy(e->vars[e->total].tipo, tipo, 19);
    e->total++;
}

int sem_var_existe(const char* nome) {
    for (int i = topo; i >= 0; i--) {
        for (int j = 0; j < pilha[i].total; j++)
            if (strcmp(pilha[i].vars[j].nome, nome) == 0) return 1;
    }
    return 0;
}

const char* sem_tipo_var(const char* nome) {
    for (int i = topo; i >= 0; i--) {
        for (int j = 0; j < pilha[i].total; j++)
            if (strcmp(pilha[i].vars[j].nome, nome) == 0)
                return pilha[i].vars[j].tipo;
    }
    return NULL;
}

void sem_declarar_funcao(const char* nome, const char* tipoRetorno, int numParams) {
    if (sem_funcao_existe(nome)) {
        fprintf(stderr, "Erro semantico: funcao '%s' ja declarada\n", nome);
        exit(1);
    }
    if (totalFuncs >= MAX_FUNCS) {
        fprintf(stderr, "Erro semantico: tabela de funcoes cheia\n");
        exit(1);
    }
    strncpy(funcs[totalFuncs].nome, nome, 99);
    strncpy(funcs[totalFuncs].tipoRetorno, tipoRetorno, 19);
    funcs[totalFuncs].numParams = numParams;
    totalFuncs++;
}

int sem_funcao_existe(const char* nome) {
    for (int i = 0; i < totalFuncs; i++)
        if (strcmp(funcs[i].nome, nome) == 0) return 1;
    return 0;
}

const char* sem_tipo_funcao(const char* nome) {
    for (int i = 0; i < totalFuncs; i++)
        if (strcmp(funcs[i].nome, nome) == 0) return funcs[i].tipoRetorno;
    return NULL;
}

int sem_num_params(const char* nome) {
    for (int i = 0; i < totalFuncs; i++)
        if (strcmp(funcs[i].nome, nome) == 0) return funcs[i].numParams;
    return -1;
}

int sem_tipos_compativeis(const char* t1, const char* t2) {
    if (!t1 || !t2) return 0;
    if (strcmp(t1, t2) == 0) return 1;
    /* coerção numérica simples: int <-> float, bool <-> int */
    if ((strcmp(t1, "int") == 0 && strcmp(t2, "float") == 0) ||
        (strcmp(t1, "float") == 0 && strcmp(t2, "int") == 0)) return 1;
    if ((strcmp(t1, "int") == 0 && strcmp(t2, "bool") == 0) ||
        (strcmp(t1, "bool") == 0 && strcmp(t2, "int") == 0)) return 1;
    return 0;
}

const char* sem_tipo_resultado_binario(const char* t1, const char* t2, const char* op) {
    (void)op;
    if (!t1) return t2;
    if (!t2) return t1;
    if (strcmp(t1, "float") == 0 || strcmp(t2, "float") == 0) return "float";
    if (strcmp(t1, "bool") == 0 && strcmp(t2, "bool") == 0) return "bool";
    return "int";
}

/* ---------- Percurso pós-parse (validação extra na AST) ---------- */

static const char* tipo_de_expr(NoArvore* n);

static void exigir_var(NoArvore* id) {
    if (!sem_var_existe(id->valor)) {
        fprintf(stderr, "Erro semantico: variavel '%s' nao declarada\n", id->valor);
        exit(1);
    }
    const char* t = sem_tipo_var(id->valor);
    if (t) strncpy(id->tipoVariavel, t, 19);
}

static const char* tipo_de_expr(NoArvore* n) {
    if (!n) return "int";

    if (strcmp(n->tipo, "NUMERO") == 0) {
        strcpy(n->tipoVariavel, "int");
        return "int";
    }
    if (strcmp(n->tipo, "FLOAT_LITERAL") == 0) {
        strcpy(n->tipoVariavel, "float");
        return "float";
    }
    if (strcmp(n->tipo, "BOOL_LITERAL") == 0) {
        strcpy(n->tipoVariavel, "bool");
        return "bool";
    }
    if (strcmp(n->tipo, "STRING_LITERAL") == 0) {
        strcpy(n->tipoVariavel, "string");
        return "string";
    }
    if (strcmp(n->tipo, "CHAR_LITERAL") == 0) {
        strcpy(n->tipoVariavel, "char");
        return "char";
    }
    if (strcmp(n->tipo, "IDENTIFICADOR") == 0) {
        exigir_var(n);
        return n->tipoVariavel;
    }
    if (strcmp(n->tipo, "CHAMADA_FUNCAO") == 0) {
        NoArvore* nome = n->filho;
        if (!sem_funcao_existe(nome->valor)) {
            fprintf(stderr, "Erro semantico: funcao '%s' nao declarada\n", nome->valor);
            exit(1);
        }
        NoArvore* args = nome->proximo;
        int cont = 0;
        if (args) {
            for (NoArvore* a = args->filho; a; a = a->proximo) {
                tipo_de_expr(a);
                cont++;
            }
        }
        int esperados = sem_num_params(nome->valor);
        if (esperados >= 0 && cont != esperados) {
            fprintf(stderr, "Erro semantico: funcao '%s' espera %d arg(s), recebeu %d\n",
                    nome->valor, esperados, cont);
            exit(1);
        }
        const char* tr = sem_tipo_funcao(nome->valor);
        if (tr) strncpy(n->tipoVariavel, tr, 19);
        return tr ? tr : "int";
    }

    /* operadores binários */
    if (n->filho && n->filho->proximo) {
        const char* te = tipo_de_expr(n->filho);
        const char* td = tipo_de_expr(n->filho->proximo);
        const char* r = sem_tipo_resultado_binario(te, td, n->tipo);
        /* relacionais / lógicos → bool */
        if (strcmp(n->tipo, "IGUAL") == 0 || strcmp(n->tipo, "DIFERENTE") == 0 ||
            strcmp(n->tipo, "MAIOR") == 0 || strcmp(n->tipo, "MENOR") == 0 ||
            strcmp(n->tipo, "MAIOR_IGUAL") == 0 || strcmp(n->tipo, "MENOR_IGUAL") == 0 ||
            strcmp(n->tipo, "E_LOGICO") == 0 || strcmp(n->tipo, "OU_LOGICO") == 0) {
            strcpy(n->tipoVariavel, "bool");
            return "bool";
        }
        strncpy(n->tipoVariavel, r, 19);
        return r;
    }

    /* unário */
    if (n->filho) {
        const char* t = tipo_de_expr(n->filho);
        if (strcmp(n->tipo, "NOT") == 0) {
            strcpy(n->tipoVariavel, "bool");
            return "bool";
        }
        strncpy(n->tipoVariavel, t, 19);
        return t;
    }
    return "int";
}

static void visitar(NoArvore* n);

static void visitar_bloco(NoArvore* bloco) {
    sem_abrir_escopo();
    for (NoArvore* c = bloco->filho; c; c = c->proximo)
        visitar(c);
    sem_fechar_escopo();
}

static void visitar(NoArvore* n) {
    if (!n) return;

    if (strcmp(n->tipo, "PROGRAMA") == 0) {
        sem_abrir_escopo(); /* global */
        for (NoArvore* c = n->filho; c; c = c->proximo)
            visitar(c);
        sem_fechar_escopo();
        return;
    }

    if (strcmp(n->tipo, "FUNCAO") == 0) {
        /* filhos: IDENT, PARAMETROS?, TIPO, BLOCO */
        NoArvore* id = n->filho;
        NoArvore* p = id->proximo;
        NoArvore* tipoNo = NULL;
        NoArvore* bloco = NULL;
        NoArvore* params = NULL;
        int nparams = 0;

        if (p && strcmp(p->tipo, "PARAMETROS") == 0) {
            params = p;
            tipoNo = p->proximo;
        } else {
            tipoNo = p;
        }
        bloco = tipoNo ? tipoNo->proximo : NULL;

        if (params) {
            for (NoArvore* a = params->filho; a; a = a->proximo) {
                if (strcmp(a->tipo, "IDENTIFICADOR") == 0) nparams++;
            }
        }
        if (tipoNo)
            sem_declarar_funcao(id->valor, tipoNo->valor, nparams);

        sem_abrir_escopo();
        if (params) {
            for (NoArvore* a = params->filho; a; a = a->proximo) {
                if (strcmp(a->tipo, "IDENTIFICADOR") == 0 && a->proximo &&
                    strcmp(a->proximo->tipo, "TIPO") == 0) {
                    sem_declarar_var(a->valor, a->proximo->valor);
                    a = a->proximo; /* pula TIPO */
                }
            }
        }
        if (bloco) visitar_bloco(bloco);
        sem_fechar_escopo();
        return;
    }

    if (strcmp(n->tipo, "MAIN") == 0) {
        if (n->filho) visitar_bloco(n->filho);
        return;
    }

    if (strcmp(n->tipo, "DECLARACAO") == 0) {
        NoArvore* id = n->filho;
        NoArvore* tipoNo = id ? id->proximo : NULL;
        NoArvore* init = tipoNo ? tipoNo->proximo : NULL;
        if (id && tipoNo) {
            sem_declarar_var(id->valor, tipoNo->valor);
            strncpy(id->tipoVariavel, tipoNo->valor, 19);
        }
        if (init) {
            const char* te = tipo_de_expr(init);
            if (tipoNo && !sem_tipos_compativeis(tipoNo->valor, te)) {
                fprintf(stderr, "Erro semantico: tipo incompativel na inicializacao de '%s' (%s vs %s)\n",
                        id->valor, tipoNo->valor, te);
                exit(1);
            }
        }
        return;
    }

    if (strcmp(n->tipo, "ATRIBUICAO") == 0) {
        NoArvore* id = n->filho;
        NoArvore* expr = id ? id->proximo : NULL;
        exigir_var(id);
        const char* te = tipo_de_expr(expr);
        if (!sem_tipos_compativeis(id->tipoVariavel, te)) {
            fprintf(stderr, "Erro semantico: tipo incompativel em atribuicao de '%s'\n", id->valor);
            exit(1);
        }
        return;
    }

    if (strcmp(n->tipo, "IF") == 0) {
        NoArvore* cond = n->filho;
        NoArvore* thenB = cond ? cond->proximo : NULL;
        NoArvore* elseB = thenB ? thenB->proximo : NULL;
        tipo_de_expr(cond);
        if (thenB) visitar_bloco(thenB);
        if (elseB) visitar_bloco(elseB);
        return;
    }

    if (strcmp(n->tipo, "WHILE") == 0) {
        tipo_de_expr(n->filho);
        if (n->filho && n->filho->proximo) visitar_bloco(n->filho->proximo);
        return;
    }

    if (strcmp(n->tipo, "FOR") == 0) {
        /* INIT, COND, INCR, BLOCO */
        NoArvore* init = n->filho;
        NoArvore* cond = init ? init->proximo : NULL;
        NoArvore* incr = cond ? cond->proximo : NULL;
        NoArvore* bloco = incr ? incr->proximo : NULL;
        visitar(init);
        tipo_de_expr(cond);
        visitar(incr);
        if (bloco) visitar_bloco(bloco);
        return;
    }

    if (strcmp(n->tipo, "PRINT") == 0) {
        tipo_de_expr(n->filho);
        return;
    }

    if (strcmp(n->tipo, "INPUT") == 0) {
        exigir_var(n->filho);
        return;
    }

    if (strcmp(n->tipo, "RETURN") == 0) {
        tipo_de_expr(n->filho);
        return;
    }

    if (strcmp(n->tipo, "CHAMADA_FUNCAO") == 0) {
        tipo_de_expr(n);
        return;
    }
}

void analisar_semantica(NoArvore* raiz) {
    /*
     * O parser já registrou símbolos durante a análise sintática.
     * Reiniciamos e revalidamos na AST para garantir consistência
     * (módulo semântico independente / apresentável).
     */
    sem_iniciar();
    visitar(raiz);
    fprintf(stderr, "Analise semantica: OK\n");
}
