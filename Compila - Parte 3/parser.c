/*
 * ============================================================================
 * ANALISADOR SINTÁTICO PREDITIVO GUIADO POR TABELA - LSI-2025-2
 * ============================================================================
 *
 * Integrantes: Eduardo Boçon, Darnley Ribeiro, Jiliard Peifer e Olavo Ançay
 *
 * Este analisador sintático implementa:
 *   - Análise preditiva guiada por tabela de reconhecimento sintático
 *   - Pilha explícita para gerenciamento de símbolos
 *   - Tabela LL(1) construída a partir da gramática transformada
 *   - Detecção de erros sintáticos com linha e coluna
 *   - Integração com analisador léxico da Parte 1
 *
 * Funcionamento:
 *   1. Inicializa a pilha com símbolo inicial (NT_MAIN) e EOF
 *   2. Consulta a tabela[não-terminal][terminal] para obter regra de produção
 *   3. Aplica a regra empilhando símbolos na ordem reversa
 *   4. Compara terminais do topo da pilha com tokens da entrada
 *   5. Repete até pilha vazia ou erro encontrado
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

/* ============================================================================
 * DECLARAÇÕES EXTERNAS DO LEXER
 * ============================================================================ */

extern FILE* inputFile;
extern int line;
extern int col;
extern char currentChar;
extern void advance(void);
extern Token getToken(void);
extern void symtable_init(void);

/* ============================================================================
 * ENUMERAÇÃO DOS NÃO-TERMINAIS DA GRAMÁTICA
 * ============================================================================ */

typedef enum {
    NT_MAIN,                /* Símbolo inicial do programa */
    NT_STMT,                /* Comando (statement) */
    NT_FLIST,               /* Lista de funções */
    NT_FLIST_OPT,           /* Lista de funções opcional */
    NT_FDEF,                /* Definição de função */
    NT_PARLIST,             /* Lista de parâmetros */
    NT_PARLIST_TAIL,        /* Cauda da lista de parâmetros */
    NT_VARLIST,             /* Lista de variáveis */
    NT_VARLIST_PRIME,       /* Continuação da lista de variáveis */
    NT_ATRIBST,             /* Comando de atribuição */
    NT_ATRIBST_TAIL,        /* Cauda da atribuição (expr ou fcall) */
    NT_FCALL,               /* Chamada de função */
    NT_PARLISTCALL,         /* Lista de parâmetros em chamada */
    NT_PARLISTCALL_TAIL,    /* Cauda da lista de parâmetros em chamada */
    NT_PRINTST,             /* Comando print */
    NT_RETURNST,            /* Comando return */
    NT_RETURN_TAIL,         /* Cauda do return (opcional) */
    NT_IFSTMT,              /* Comando if */
    NT_IF_TAIL,             /* Cauda do if (else opcional) */
    NT_STMTLIST,            /* Lista de comandos */
    NT_STMTLIST_OPT,        /* Lista de comandos opcional */
    NT_EXPR,                /* Expressão */
    NT_EXPR_PRIME,          /* Continuação de expressão (relop) */
    NT_RELOP,               /* Operador relacional */
    NT_NUMEXPR,             /* Expressão numérica */
    NT_NUMEXPR_PRIME,       /* Continuação de expressão numérica */
    NT_ADDOP,               /* Operador aditivo */
    NT_TERM,                /* Termo */
    NT_TERM_PRIME,          /* Continuação de termo */
    NT_MULOP,               /* Operador multiplicativo */
    NT_FACTOR               /* Fator */
} NonTerminal;

/* ============================================================================
 * ESTRUTURA DE SÍMBOLOS DA PILHA
 * ============================================================================ */

typedef struct {
    int is_terminal;        /* 1 se terminal, 0 se não-terminal */
    union {
        TokenType terminal;        /* Tipo do token (se terminal) */
        NonTerminal non_terminal;  /* Tipo do não-terminal */
    } value;
} StackSymbol;

/* ============================================================================
 * PILHA DE PARSING
 * ============================================================================ */

#define STACK_SIZE 200
StackSymbol parse_stack[STACK_SIZE];
int stack_top = -1;

/*
 * stack_push(symbol)
 *
 * Empilha um símbolo na pilha de parsing.
 * Verifica overflow antes de empilhar.
 */
void stack_push(StackSymbol symbol) {
    if (stack_top >= STACK_SIZE - 1) {
        fprintf(stderr, "Erro fatal: Estouro da pilha do parser.\n");
        exit(1);
    }
    parse_stack[++stack_top] = symbol;
}

/*
 * stack_pop()
 *
 * Desempilha e retorna o símbolo do topo da pilha.
 * Verifica underflow antes de desempilhar.
 */
StackSymbol stack_pop() {
    if (stack_top < 0) {
        fprintf(stderr, "Erro fatal: Pilha do parser vazia.\n");
        exit(1);
    }
    return parse_stack[stack_top--];
}

/*
 * create_terminal_symbol(type)
 *
 * Cria um símbolo terminal com o tipo especificado.
 */
StackSymbol create_terminal_symbol(TokenType type) {
    return (StackSymbol){.is_terminal = 1, .value.terminal = type};
}

/*
 * create_non_terminal_symbol(type)
 *
 * Cria um símbolo não-terminal com o tipo especificado.
 */
StackSymbol create_non_terminal_symbol(NonTerminal type) {
    return (StackSymbol){.is_terminal = 0, .value.non_terminal = type};
}

/* ============================================================================
 * ENUMERAÇÃO DAS REGRAS DE PRODUÇÃO
 * ============================================================================ */

typedef enum {
    RULE_ERROR = 0,                 /* Erro - entrada não esperada */

    /* Regras para MAIN */
    RULE_MAIN_STMT,                 /* MAIN → STMT */
    RULE_MAIN_FLIST,                /* MAIN → FLIST */
    RULE_MAIN_EPSILON,              /* MAIN → ε */

    /* Regras para STMT */
    RULE_STMT_INT,                  /* STMT → int VARLIST ; */
    RULE_STMT_ATRIB,                /* STMT → ATRIBST ; */
    RULE_STMT_PRINT,                /* STMT → PRINTST ; */
    RULE_STMT_RETURN,               /* STMT → RETURNST ; */
    RULE_STMT_IF,                   /* STMT → IFSTMT */
    RULE_STMT_BLOCK,                /* STMT → { STMTLIST } */
    RULE_STMT_SEMICOLON,            /* STMT → ; */

    /* Regras para FLIST */
    RULE_FLIST,                     /* FLIST → FDEF FLIST_OPT */
    RULE_FLIST_OPT,                 /* FLIST_OPT → FDEF FLIST_OPT */
    RULE_FLIST_OPT_EPSILON,         /* FLIST_OPT → ε */

    /* Regra para FDEF */
    RULE_FDEF,                      /* FDEF → def id ( PARLIST ) { STMTLIST } */

    /* Regras para PARLIST */
    RULE_PARLIST,                   /* PARLIST → int id PARLIST_TAIL */
    RULE_PARLIST_EPSILON,           /* PARLIST → ε */
    RULE_PARLIST_TAIL,              /* PARLIST_TAIL → , PARLIST */
    RULE_PARLIST_TAIL_EPSILON,      /* PARLIST_TAIL → ε */

    /* Regras para VARLIST */
    RULE_VARLIST,                   /* VARLIST → id VARLIST_PRIME */
    RULE_VARLIST_PRIME,             /* VARLIST_PRIME → , VARLIST */
    RULE_VARLIST_PRIME_EPSILON,     /* VARLIST_PRIME → ε */

    /* Regras para ATRIBST */
    RULE_ATRIBST,                   /* ATRIBST → id = ATRIBST_TAIL */
    RULE_ATRIBST_TAIL_EXPR,         /* ATRIBST_TAIL → EXPR */
    RULE_ATRIBST_TAIL_FCALL,        /* ATRIBST_TAIL → FCALL */

    /* Regras para FCALL */
    RULE_FCALL,                     /* FCALL → id ( PARLISTCALL ) */
    RULE_PARLISTCALL,               /* PARLISTCALL → id PARLISTCALL_TAIL */
    RULE_PARLISTCALL_EPSILON,       /* PARLISTCALL → ε */
    RULE_PARLISTCALL_TAIL,          /* PARLISTCALL_TAIL → , PARLISTCALL */
    RULE_PARLISTCALL_TAIL_EPSILON,  /* PARLISTCALL_TAIL → ε */

    /* Regras para PRINTST e RETURNST */
    RULE_PRINTST,                   /* PRINTST → print EXPR */
    RULE_RETURNST,                  /* RETURNST → return RETURN_TAIL */
    RULE_RETURN_TAIL_ID,            /* RETURN_TAIL → id */
    RULE_RETURN_TAIL_EPSILON,       /* RETURN_TAIL → ε */

    /* Regras para IFSTMT */
    RULE_IFSTMT,                    /* IFSTMT → if ( EXPR ) { STMT } IF_TAIL */
    RULE_IF_TAIL_ELSE,              /* IF_TAIL → else { STMT } */
    RULE_IF_TAIL_EPSILON,           /* IF_TAIL → ε */

    /* Regras para STMTLIST */
    RULE_STMTLIST,                  /* STMTLIST → STMT STMTLIST_OPT */
    RULE_STMTLIST_OPT,              /* STMTLIST_OPT → STMT STMTLIST_OPT */
    RULE_STMTLIST_OPT_EPSILON,      /* STMTLIST_OPT → ε */

    /* Regras para EXPR */
    RULE_EXPR,                      /* EXPR → NUMEXPR EXPR_PRIME */
    RULE_EXPR_PRIME,                /* EXPR_PRIME → RELOP NUMEXPR */
    RULE_EXPR_PRIME_EPSILON,        /* EXPR_PRIME → ε */

    /* Regras para RELOP */
    RULE_RELOP_LT,                  /* RELOP → < */
    RULE_RELOP_LTE,                 /* RELOP → <= */
    RULE_RELOP_GT,                  /* RELOP → > */
    RULE_RELOP_GTE,                 /* RELOP → >= */
    RULE_RELOP_EQ,                  /* RELOP → == */
    RULE_RELOP_NEQ,                 /* RELOP → != */

    /* Regras para NUMEXPR */
    RULE_NUMEXPR,                   /* NUMEXPR → TERM NUMEXPR_PRIME */
    RULE_NUMEXPR_PRIME_ADDOP,       /* NUMEXPR_PRIME → ADDOP TERM NUMEXPR_PRIME */
    RULE_NUMEXPR_PRIME_EPSILON,     /* NUMEXPR_PRIME → ε */

    /* Regras para ADDOP */
    RULE_ADDOP_PLUS,                /* ADDOP → + */
    RULE_ADDOP_MINUS,               /* ADDOP → - */

    /* Regras para TERM */
    RULE_TERM,                      /* TERM → FACTOR TERM_PRIME */
    RULE_TERM_PRIME_MULOP,          /* TERM_PRIME → MULOP FACTOR TERM_PRIME */
    RULE_TERM_PRIME_EPSILON,        /* TERM_PRIME → ε */

    /* Regras para MULOP */
    RULE_MULOP_MULT,                /* MULOP → * */
    RULE_MULOP_DIV,                 /* MULOP → / */

    /* Regras para FACTOR */
    RULE_FACTOR_NUM,                /* FACTOR → num */
    RULE_FACTOR_PAREN,              /* FACTOR → ( NUMEXPR ) */
    RULE_FACTOR_ID                  /* FACTOR → id */
} ProductionRule;

/* ============================================================================
 * TABELA DE RECONHECIMENTO SINTÁTICO (PARSING TABLE)
 * ============================================================================
 *
 * Tabela LL(1) bidimensional onde:
 *   - Linhas = Não-terminais (31 possíveis)
 *   - Colunas = Terminais (30 tokens possíveis)
 *   - Célula[NT][T] = Regra de produção a aplicar
 *
 * ============================================================================
 */

ProductionRule parse_table[31][30];

/*
 * apply_rule(rule)
 *
 * Aplica uma regra de produção empilhando os símbolos do lado direito
 * da produção em ordem REVERSA (para que sejam processados na ordem correta).
 *
 * Exemplo: Para a regra STMT → int VARLIST ;
 * Empilha: ;, VARLIST, int (nesta ordem)
 * Assim, int será processado primeiro (topo da pilha)
 */
void apply_rule(ProductionRule rule) {
    switch (rule) {
        /* Regras MAIN */
        case RULE_MAIN_STMT:
            stack_push(create_non_terminal_symbol(NT_STMT));
            break;
        case RULE_MAIN_FLIST:
            stack_push(create_non_terminal_symbol(NT_FLIST));
            break;
        case RULE_MAIN_EPSILON:
            break; /* Produção vazia - não empilha nada */

        /* Regras STMT */
        case RULE_STMT_INT:
            stack_push(create_terminal_symbol(TOKEN_SEMICOLON));
            stack_push(create_non_terminal_symbol(NT_VARLIST));
            stack_push(create_terminal_symbol(TOKEN_INT));
            break;
        case RULE_STMT_ATRIB:
            stack_push(create_terminal_symbol(TOKEN_SEMICOLON));
            stack_push(create_non_terminal_symbol(NT_ATRIBST));
            break;
        case RULE_STMT_PRINT:
            stack_push(create_terminal_symbol(TOKEN_SEMICOLON));
            stack_push(create_non_terminal_symbol(NT_PRINTST));
            break;
        case RULE_STMT_RETURN:
            stack_push(create_terminal_symbol(TOKEN_SEMICOLON));
            stack_push(create_non_terminal_symbol(NT_RETURNST));
            break;
        case RULE_STMT_IF:
            stack_push(create_non_terminal_symbol(NT_IFSTMT));
            break;
        case RULE_STMT_BLOCK:
            stack_push(create_terminal_symbol(TOKEN_RBRACE));
            stack_push(create_non_terminal_symbol(NT_STMTLIST));
            stack_push(create_terminal_symbol(TOKEN_LBRACE));
            break;
        case RULE_STMT_SEMICOLON:
            stack_push(create_terminal_symbol(TOKEN_SEMICOLON));
            break;

        /* Regras FLIST */
        case RULE_FLIST:
            stack_push(create_non_terminal_symbol(NT_FLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_FDEF));
            break;
        case RULE_FLIST_OPT:
            stack_push(create_non_terminal_symbol(NT_FLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_FDEF));
            break;
        case RULE_FLIST_OPT_EPSILON:
            break;

        /* Regra FDEF */
        case RULE_FDEF:
            stack_push(create_terminal_symbol(TOKEN_RBRACE));
            stack_push(create_non_terminal_symbol(NT_STMTLIST));
            stack_push(create_terminal_symbol(TOKEN_LBRACE));
            stack_push(create_terminal_symbol(TOKEN_RPAREN));
            stack_push(create_non_terminal_symbol(NT_PARLIST));
            stack_push(create_terminal_symbol(TOKEN_LPAREN));
            stack_push(create_terminal_symbol(TOKEN_ID));
            stack_push(create_terminal_symbol(TOKEN_DEF));
            break;

        /* Regras PARLIST */
        case RULE_PARLIST:
            stack_push(create_non_terminal_symbol(NT_PARLIST_TAIL));
            stack_push(create_terminal_symbol(TOKEN_ID));
            stack_push(create_terminal_symbol(TOKEN_INT));
            break;
        case RULE_PARLIST_EPSILON:
            break;
        case RULE_PARLIST_TAIL:
            stack_push(create_non_terminal_symbol(NT_PARLIST));
            stack_push(create_terminal_symbol(TOKEN_COMMA));
            break;
        case RULE_PARLIST_TAIL_EPSILON:
            break;

        /* Regras VARLIST */
        case RULE_VARLIST:
            stack_push(create_non_terminal_symbol(NT_VARLIST_PRIME));
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_VARLIST_PRIME:
            stack_push(create_non_terminal_symbol(NT_VARLIST));
            stack_push(create_terminal_symbol(TOKEN_COMMA));
            break;
        case RULE_VARLIST_PRIME_EPSILON:
            break;

        /* Regras ATRIBST */
        case RULE_ATRIBST:
            stack_push(create_non_terminal_symbol(NT_ATRIBST_TAIL));
            stack_push(create_terminal_symbol(TOKEN_ASSIGN));
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_ATRIBST_TAIL_EXPR:
            stack_push(create_non_terminal_symbol(NT_EXPR));
            break;
        case RULE_ATRIBST_TAIL_FCALL:
            stack_push(create_non_terminal_symbol(NT_FCALL));
            break;

        /* Regras FCALL */
        case RULE_FCALL:
            stack_push(create_terminal_symbol(TOKEN_RPAREN));
            stack_push(create_non_terminal_symbol(NT_PARLISTCALL));
            stack_push(create_terminal_symbol(TOKEN_LPAREN));
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_PARLISTCALL:
            stack_push(create_non_terminal_symbol(NT_PARLISTCALL_TAIL));
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_PARLISTCALL_EPSILON:
            break;
        case RULE_PARLISTCALL_TAIL:
            stack_push(create_non_terminal_symbol(NT_PARLISTCALL));
            stack_push(create_terminal_symbol(TOKEN_COMMA));
            break;
        case RULE_PARLISTCALL_TAIL_EPSILON:
            break;

        /* Regras PRINTST e RETURNST */
        case RULE_PRINTST:
            stack_push(create_non_terminal_symbol(NT_EXPR));
            stack_push(create_terminal_symbol(TOKEN_PRINT));
            break;
        case RULE_RETURNST:
            stack_push(create_non_terminal_symbol(NT_RETURN_TAIL));
            stack_push(create_terminal_symbol(TOKEN_RETURN));
            break;
        case RULE_RETURN_TAIL_ID:
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_RETURN_TAIL_EPSILON:
            break;

        /* Regras IFSTMT */
        case RULE_IFSTMT:
            stack_push(create_non_terminal_symbol(NT_IF_TAIL));
            stack_push(create_terminal_symbol(TOKEN_RBRACE));
            stack_push(create_non_terminal_symbol(NT_STMT));
            stack_push(create_terminal_symbol(TOKEN_LBRACE));
            stack_push(create_terminal_symbol(TOKEN_RPAREN));
            stack_push(create_non_terminal_symbol(NT_EXPR));
            stack_push(create_terminal_symbol(TOKEN_LPAREN));
            stack_push(create_terminal_symbol(TOKEN_IF));
            break;
        case RULE_IF_TAIL_ELSE:
            stack_push(create_terminal_symbol(TOKEN_RBRACE));
            stack_push(create_non_terminal_symbol(NT_STMT));
            stack_push(create_terminal_symbol(TOKEN_LBRACE));
            stack_push(create_terminal_symbol(TOKEN_ELSE));
            break;
        case RULE_IF_TAIL_EPSILON:
            break;

        /* Regras STMTLIST */
        case RULE_STMTLIST:
            stack_push(create_non_terminal_symbol(NT_STMTLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_STMT));
            break;
        case RULE_STMTLIST_OPT:
            stack_push(create_non_terminal_symbol(NT_STMTLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_STMT));
            break;
        case RULE_STMTLIST_OPT_EPSILON:
            break;

        /* Regras EXPR */
        case RULE_EXPR:
            stack_push(create_non_terminal_symbol(NT_EXPR_PRIME));
            stack_push(create_non_terminal_symbol(NT_NUMEXPR));
            break;
        case RULE_EXPR_PRIME:
            stack_push(create_non_terminal_symbol(NT_NUMEXPR));
            stack_push(create_non_terminal_symbol(NT_RELOP));
            break;
        case RULE_EXPR_PRIME_EPSILON:
            break;

        /* Regras RELOP */
        case RULE_RELOP_LT:
            stack_push(create_terminal_symbol(TOKEN_LT));
            break;
        case RULE_RELOP_LTE:
            stack_push(create_terminal_symbol(TOKEN_LTE));
            break;
        case RULE_RELOP_GT:
            stack_push(create_terminal_symbol(TOKEN_GT));
            break;
        case RULE_RELOP_GTE:
            stack_push(create_terminal_symbol(TOKEN_GTE));
            break;
        case RULE_RELOP_EQ:
            stack_push(create_terminal_symbol(TOKEN_EQ));
            break;
        case RULE_RELOP_NEQ:
            stack_push(create_terminal_symbol(TOKEN_NEQ));
            break;

        /* Regras NUMEXPR */
        case RULE_NUMEXPR:
            stack_push(create_non_terminal_symbol(NT_NUMEXPR_PRIME));
            stack_push(create_non_terminal_symbol(NT_TERM));
            break;
        case RULE_NUMEXPR_PRIME_ADDOP:
            stack_push(create_non_terminal_symbol(NT_NUMEXPR_PRIME));
            stack_push(create_non_terminal_symbol(NT_TERM));
            stack_push(create_non_terminal_symbol(NT_ADDOP));
            break;
        case RULE_NUMEXPR_PRIME_EPSILON:
            break;

        /* Regras ADDOP */
        case RULE_ADDOP_PLUS:
            stack_push(create_terminal_symbol(TOKEN_PLUS));
            break;
        case RULE_ADDOP_MINUS:
            stack_push(create_terminal_symbol(TOKEN_MINUS));
            break;

        /* Regras TERM */
        case RULE_TERM:
            stack_push(create_non_terminal_symbol(NT_TERM_PRIME));
            stack_push(create_non_terminal_symbol(NT_FACTOR));
            break;
        case RULE_TERM_PRIME_MULOP:
            stack_push(create_non_terminal_symbol(NT_TERM_PRIME));
            stack_push(create_non_terminal_symbol(NT_FACTOR));
            stack_push(create_non_terminal_symbol(NT_MULOP));
            break;
        case RULE_TERM_PRIME_EPSILON:
            break;

        /* Regras MULOP */
        case RULE_MULOP_MULT:
            stack_push(create_terminal_symbol(TOKEN_MULT));
            break;
        case RULE_MULOP_DIV:
            stack_push(create_terminal_symbol(TOKEN_DIV));
            break;

        /* Regras FACTOR */
        case RULE_FACTOR_NUM:
            stack_push(create_terminal_symbol(TOKEN_NUM));
            break;
        case RULE_FACTOR_ID:
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_FACTOR_PAREN:
            stack_push(create_terminal_symbol(TOKEN_RPAREN));
            stack_push(create_non_terminal_symbol(NT_NUMEXPR));
            stack_push(create_terminal_symbol(TOKEN_LPAREN));
            break;

        default:
            break;
    }
}

/*
 * initialize_parse_table()
 *
 * Inicializa a tabela de reconhecimento sintático LL(1) com as regras
 * de produção para cada combinação (não-terminal, terminal).
 *
 * Cada célula parse_table[NT][T] contém a regra a ser aplicada quando:
 *   - O topo da pilha é o não-terminal NT
 *   - O token atual da entrada é T
 *
 * Células não preenchidas contêm RULE_ERROR (erro sintático).
 */
void initialize_parse_table() {
    /* Inicializa toda a tabela com RULE_ERROR */
    for (int i = 0; i < 31; i++) {
        for (int j = 0; j < 30; j++) {
            parse_table[i][j] = RULE_ERROR;
        }
    }

    /* Preenche a tabela com as regras de produção */

    /* MAIN */
    parse_table[NT_MAIN][TOKEN_INT] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_ID] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_PRINT] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_RETURN] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_IF] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_LBRACE] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_SEMICOLON] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_DEF] = RULE_MAIN_FLIST;
    parse_table[NT_MAIN][TOKEN_EOF] = RULE_MAIN_EPSILON;

    /* STMT */
    parse_table[NT_STMT][TOKEN_INT] = RULE_STMT_INT;
    parse_table[NT_STMT][TOKEN_ID] = RULE_STMT_ATRIB;
    parse_table[NT_STMT][TOKEN_PRINT] = RULE_STMT_PRINT;
    parse_table[NT_STMT][TOKEN_RETURN] = RULE_STMT_RETURN;
    parse_table[NT_STMT][TOKEN_IF] = RULE_STMT_IF;
    parse_table[NT_STMT][TOKEN_LBRACE] = RULE_STMT_BLOCK;
    parse_table[NT_STMT][TOKEN_SEMICOLON] = RULE_STMT_SEMICOLON;

    /* FLIST */
    parse_table[NT_FLIST][TOKEN_DEF] = RULE_FLIST;
    parse_table[NT_FLIST_OPT][TOKEN_DEF] = RULE_FLIST_OPT;
    parse_table[NT_FLIST_OPT][TOKEN_EOF] = RULE_FLIST_OPT_EPSILON;

    /* FDEF */
    parse_table[NT_FDEF][TOKEN_DEF] = RULE_FDEF;

    /* PARLIST */
    parse_table[NT_PARLIST][TOKEN_INT] = RULE_PARLIST;
    parse_table[NT_PARLIST][TOKEN_RPAREN] = RULE_PARLIST_EPSILON;
    parse_table[NT_PARLIST_TAIL][TOKEN_COMMA] = RULE_PARLIST_TAIL;
    parse_table[NT_PARLIST_TAIL][TOKEN_RPAREN] = RULE_PARLIST_TAIL_EPSILON;

    /* VARLIST */
    parse_table[NT_VARLIST][TOKEN_ID] = RULE_VARLIST;
    parse_table[NT_VARLIST_PRIME][TOKEN_COMMA] = RULE_VARLIST_PRIME;
    parse_table[NT_VARLIST_PRIME][TOKEN_SEMICOLON] = RULE_VARLIST_PRIME_EPSILON;

    /* ATRIBST */
    parse_table[NT_ATRIBST][TOKEN_ID] = RULE_ATRIBST;
    parse_table[NT_ATRIBST_TAIL][TOKEN_NUM] = RULE_ATRIBST_TAIL_EXPR;
    parse_table[NT_ATRIBST_TAIL][TOKEN_LPAREN] = RULE_ATRIBST_TAIL_EXPR;
    parse_table[NT_ATRIBST_TAIL][TOKEN_ID] = RULE_ATRIBST_TAIL_EXPR;

    /* FCALL */
    parse_table[NT_FCALL][TOKEN_ID] = RULE_FCALL;
    parse_table[NT_PARLISTCALL][TOKEN_ID] = RULE_PARLISTCALL;
    parse_table[NT_PARLISTCALL][TOKEN_RPAREN] = RULE_PARLISTCALL_EPSILON;
    parse_table[NT_PARLISTCALL_TAIL][TOKEN_COMMA] = RULE_PARLISTCALL_TAIL;
    parse_table[NT_PARLISTCALL_TAIL][TOKEN_RPAREN] = RULE_PARLISTCALL_TAIL_EPSILON;

    /* PRINTST */
    parse_table[NT_PRINTST][TOKEN_PRINT] = RULE_PRINTST;

    /* RETURNST */
    parse_table[NT_RETURNST][TOKEN_RETURN] = RULE_RETURNST;
    parse_table[NT_RETURN_TAIL][TOKEN_ID] = RULE_RETURN_TAIL_ID;
    parse_table[NT_RETURN_TAIL][TOKEN_SEMICOLON] = RULE_RETURN_TAIL_EPSILON;

    /* IFSTMT */
    parse_table[NT_IFSTMT][TOKEN_IF] = RULE_IFSTMT;
    parse_table[NT_IF_TAIL][TOKEN_ELSE] = RULE_IF_TAIL_ELSE;
    parse_table[NT_IF_TAIL][TOKEN_INT] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_ID] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_PRINT] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_RETURN] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_IF] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_LBRACE] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_SEMICOLON] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_RBRACE] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_DEF] = RULE_IF_TAIL_EPSILON;
    parse_table[NT_IF_TAIL][TOKEN_EOF] = RULE_IF_TAIL_EPSILON;

    /* STMTLIST */
    parse_table[NT_STMTLIST][TOKEN_INT] = RULE_STMTLIST;
    parse_table[NT_STMTLIST][TOKEN_ID] = RULE_STMTLIST;
    parse_table[NT_STMTLIST][TOKEN_PRINT] = RULE_STMTLIST;
    parse_table[NT_STMTLIST][TOKEN_RETURN] = RULE_STMTLIST;
    parse_table[NT_STMTLIST][TOKEN_IF] = RULE_STMTLIST;
    parse_table[NT_STMTLIST][TOKEN_LBRACE] = RULE_STMTLIST;
    parse_table[NT_STMTLIST][TOKEN_SEMICOLON] = RULE_STMTLIST;
    parse_table[NT_STMTLIST_OPT][TOKEN_INT] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_ID] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_PRINT] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_RETURN] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_IF] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_LBRACE] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_SEMICOLON] = RULE_STMTLIST_OPT;
    parse_table[NT_STMTLIST_OPT][TOKEN_RBRACE] = RULE_STMTLIST_OPT_EPSILON;

    /* EXPR */
    parse_table[NT_EXPR][TOKEN_NUM] = RULE_EXPR;
    parse_table[NT_EXPR][TOKEN_LPAREN] = RULE_EXPR;
    parse_table[NT_EXPR][TOKEN_ID] = RULE_EXPR;
    parse_table[NT_EXPR_PRIME][TOKEN_LT] = RULE_EXPR_PRIME;
    parse_table[NT_EXPR_PRIME][TOKEN_LTE] = RULE_EXPR_PRIME;
    parse_table[NT_EXPR_PRIME][TOKEN_GT] = RULE_EXPR_PRIME;
    parse_table[NT_EXPR_PRIME][TOKEN_GTE] = RULE_EXPR_PRIME;
    parse_table[NT_EXPR_PRIME][TOKEN_EQ] = RULE_EXPR_PRIME;
    parse_table[NT_EXPR_PRIME][TOKEN_NEQ] = RULE_EXPR_PRIME;
    parse_table[NT_EXPR_PRIME][TOKEN_RPAREN] = RULE_EXPR_PRIME_EPSILON;
    parse_table[NT_EXPR_PRIME][TOKEN_SEMICOLON] = RULE_EXPR_PRIME_EPSILON;

    /* RELOP */
    parse_table[NT_RELOP][TOKEN_LT] = RULE_RELOP_LT;
    parse_table[NT_RELOP][TOKEN_LTE] = RULE_RELOP_LTE;
    parse_table[NT_RELOP][TOKEN_GT] = RULE_RELOP_GT;
    parse_table[NT_RELOP][TOKEN_GTE] = RULE_RELOP_GTE;
    parse_table[NT_RELOP][TOKEN_EQ] = RULE_RELOP_EQ;
    parse_table[NT_RELOP][TOKEN_NEQ] = RULE_RELOP_NEQ;

    /* NUMEXPR */
    parse_table[NT_NUMEXPR][TOKEN_NUM] = RULE_NUMEXPR;
    parse_table[NT_NUMEXPR][TOKEN_LPAREN] = RULE_NUMEXPR;
    parse_table[NT_NUMEXPR][TOKEN_ID] = RULE_NUMEXPR;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_PLUS] = RULE_NUMEXPR_PRIME_ADDOP;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_MINUS] = RULE_NUMEXPR_PRIME_ADDOP;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_LT] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_LTE] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_GT] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_GTE] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_EQ] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_NEQ] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_RPAREN] = RULE_NUMEXPR_PRIME_EPSILON;
    parse_table[NT_NUMEXPR_PRIME][TOKEN_SEMICOLON] = RULE_NUMEXPR_PRIME_EPSILON;

    /* ADDOP */
    parse_table[NT_ADDOP][TOKEN_PLUS] = RULE_ADDOP_PLUS;
    parse_table[NT_ADDOP][TOKEN_MINUS] = RULE_ADDOP_MINUS;

    /* TERM */
    parse_table[NT_TERM][TOKEN_NUM] = RULE_TERM;
    parse_table[NT_TERM][TOKEN_LPAREN] = RULE_TERM;
    parse_table[NT_TERM][TOKEN_ID] = RULE_TERM;
    parse_table[NT_TERM_PRIME][TOKEN_MULT] = RULE_TERM_PRIME_MULOP;
    parse_table[NT_TERM_PRIME][TOKEN_DIV] = RULE_TERM_PRIME_MULOP;
    parse_table[NT_TERM_PRIME][TOKEN_PLUS] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_MINUS] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_LT] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_LTE] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_GT] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_GTE] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_EQ] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_NEQ] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_RPAREN] = RULE_TERM_PRIME_EPSILON;
    parse_table[NT_TERM_PRIME][TOKEN_SEMICOLON] = RULE_TERM_PRIME_EPSILON;

    /* MULOP */
    parse_table[NT_MULOP][TOKEN_MULT] = RULE_MULOP_MULT;
    parse_table[NT_MULOP][TOKEN_DIV] = RULE_MULOP_DIV;

    /* FACTOR */
    parse_table[NT_FACTOR][TOKEN_NUM] = RULE_FACTOR_NUM;
    parse_table[NT_FACTOR][TOKEN_LPAREN] = RULE_FACTOR_PAREN;
    parse_table[NT_FACTOR][TOKEN_ID] = RULE_FACTOR_ID;
}

/* ============================================================================
 * FUNÇÃO PRINCIPAL DE PARSING
 * ============================================================================ */

/*
 * parse()
 *
 * Executa o algoritmo de análise sintática preditiva guiada por tabela.
 *
 * Algoritmo:
 * 1. Empilha EOF e símbolo inicial (NT_MAIN)
 * 2. Loop enquanto pilha não vazia:
 *    a) Desempilha X do topo
 *    b) Se X é terminal:
 *       - Compara com token atual
 *       - Se match: consome token
 *       - Senão: erro sintático
 *    c) Se X é não-terminal:
 *       - Consulta tabela[X][token_atual]
 *       - Se tem regra: aplica regra (empilha lado direito)
 *       - Senão: erro sintático
 * 3. Sucesso quando pilha vazia e EOF alcançado
 */
void parse() {
    Token currentToken = getToken();

    /* Inicializa pilha com EOF e símbolo inicial */
    stack_push(create_terminal_symbol(TOKEN_EOF));
    stack_push(create_non_terminal_symbol(NT_MAIN));

    /* Loop principal do parser */
    while (stack_top > -1) {
        StackSymbol X = stack_pop();

        if (X.is_terminal) {
            /* X é um terminal - deve coincidir com token atual */
            if (X.value.terminal == currentToken.type) {
                if (currentToken.type == TOKEN_EOF) {
                    printf("\nAnálise Sintática concluída com sucesso!\n");
                    return;
                }
                currentToken = getToken(); /* Consome token */
            } else {
                /* Erro: terminal esperado não coincide */
                fprintf(stderr, "\n--- Erro Sintático ---\n");
                fprintf(stderr, "Esperado: %s\n", token_type_to_string(X.value.terminal));
                fprintf(stderr, "Encontrado: '%s' (%s) na linha %d, coluna %d\n",
                        currentToken.lexeme, token_type_to_string(currentToken.type),
                        currentToken.line, currentToken.col);
                exit(1);
            }
        } else {
            /* X é não-terminal - consulta tabela para obter regra */
            ProductionRule rule = parse_table[X.value.non_terminal][currentToken.type];

            if (rule == RULE_ERROR) {
                /* Erro: combinação (não-terminal, terminal) inválida */
                fprintf(stderr, "\n--- Erro Sintático ---\n");
                fprintf(stderr, "Token inesperado: '%s' (%s)\n",
                        currentToken.lexeme, token_type_to_string(currentToken.type));
                fprintf(stderr, "Localização: linha %d, coluna %d\n",
                        currentToken.line, currentToken.col);
                exit(1);
            }

            /* Aplica a regra de produção (empilha lado direito) */
            apply_rule(rule);
        }
    }
}

/* ============================================================================
 * FUNÇÃO PRINCIPAL
 * ============================================================================ */

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.lsi>\n", argv[0]);
        return 1;
    }

    /* Abre arquivo de entrada */
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    /* Inicializa componentes */
    symtable_init();              /* Inicializa tabela de símbolos do lexer */
    initialize_parse_table();     /* Inicializa tabela de reconhecimento sintático */
    advance();                    /* Lê primeiro caractere */

    /* Executa análise sintática */
    parse();

    fclose(inputFile);
    return 0;
}
