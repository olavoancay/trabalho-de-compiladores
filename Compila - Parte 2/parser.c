#include <stdio.h>
#include <stdlib.h>
#include "lexer.h" 

extern FILE* inputFile;
extern int line;
extern int col;
extern char currentChar;
extern void advance(void);
extern Token getToken(void);
extern void symtable_init(void);

typedef enum {
    NT_MAIN, NT_STMT, NT_FLIST, NT_FLIST_OPT, NT_FDEF,
    NT_PARLIST, NT_PARLIST_TAIL, NT_VARLIST, NT_VARLIST_PRIME,
    NT_ATRIBST, NT_ATRIBST_TAIL, NT_FCALL, NT_PARLISTCALL, NT_PARLISTCALL_TAIL,
    NT_PRINTST, NT_RETURNST, NT_RETURN_TAIL, NT_IFSTMT, NT_IF_TAIL,
    NT_STMTLIST, NT_STMTLIST_OPT, NT_EXPR, NT_EXPR_PRIME, NT_RELOP,
    NT_NUMEXPR, NT_NUMEXPR_PRIME, NT_ADDOP, NT_TERM, NT_TERM_PRIME,
    NT_MULOP, NT_FACTOR
} NonTerminal;

typedef struct {
    int is_terminal;
    union {
        TokenType terminal;
        NonTerminal non_terminal;
    } value;
} StackSymbol;

#define STACK_SIZE 200
StackSymbol parse_stack[STACK_SIZE];
int stack_top = -1;

void stack_push(StackSymbol symbol) {
    if (stack_top >= STACK_SIZE - 1) {
        fprintf(stderr, "Erro fatal: Estouro da pilha do parser.\n");
        exit(1);
    }
    parse_stack[++stack_top] = symbol;
}

StackSymbol stack_pop() {
    if (stack_top < 0) {
        fprintf(stderr, "Erro fatal: Pilha do parser vazia.\n");
        exit(1);
    }
    return parse_stack[stack_top--];
}

StackSymbol create_terminal_symbol(TokenType type) {
    return (StackSymbol){.is_terminal = 1, .value.terminal = type};
}

StackSymbol create_non_terminal_symbol(NonTerminal type) {
    return (StackSymbol){.is_terminal = 0, .value.non_terminal = type};
}

typedef enum {
    RULE_ERROR = 0,
    RULE_MAIN_STMT, RULE_MAIN_FLIST, RULE_MAIN_EPSILON,
    RULE_STMT_INT, RULE_STMT_ATRIB, RULE_STMT_PRINT, RULE_STMT_RETURN,
    RULE_STMT_IF, RULE_STMT_BLOCK, RULE_STMT_SEMICOLON,
    RULE_FLIST, RULE_FLIST_OPT, RULE_FLIST_OPT_EPSILON,
    RULE_FDEF,
    RULE_PARLIST, RULE_PARLIST_EPSILON,
    RULE_PARLIST_TAIL, RULE_PARLIST_TAIL_EPSILON,
    RULE_VARLIST, RULE_VARLIST_PRIME, RULE_VARLIST_PRIME_EPSILON,
    RULE_ATRIBST, RULE_ATRIBST_TAIL_EXPR, RULE_ATRIBST_TAIL_FCALL,
    RULE_FCALL, RULE_PARLISTCALL, RULE_PARLISTCALL_EPSILON,
    RULE_PARLISTCALL_TAIL, RULE_PARLISTCALL_TAIL_EPSILON,
    RULE_PRINTST, RULE_RETURNST,
    RULE_RETURN_TAIL_ID, RULE_RETURN_TAIL_EPSILON,
    RULE_IFSTMT, RULE_IF_TAIL_ELSE, RULE_IF_TAIL_EPSILON,
    RULE_STMTLIST, RULE_STMTLIST_OPT, RULE_STMTLIST_OPT_EPSILON,
    RULE_EXPR, RULE_EXPR_PRIME, RULE_EXPR_PRIME_EPSILON,
    RULE_RELOP_LT, RULE_RELOP_LTE, RULE_RELOP_GT, RULE_RELOP_GTE,
    RULE_RELOP_EQ, RULE_RELOP_NEQ,
    RULE_NUMEXPR, RULE_NUMEXPR_PRIME_ADDOP, RULE_NUMEXPR_PRIME_EPSILON,
    RULE_ADDOP_PLUS, RULE_ADDOP_MINUS,
    RULE_TERM, RULE_TERM_PRIME_MULOP, RULE_TERM_PRIME_EPSILON,
    RULE_MULOP_MULT, RULE_MULOP_DIV,
    RULE_FACTOR_NUM, RULE_FACTOR_PAREN, RULE_FACTOR_ID
} ProductionRule;

ProductionRule parse_table[31][30]; 

void apply_rule(ProductionRule rule) {
    switch (rule) {
        case RULE_MAIN_STMT: stack_push(create_non_terminal_symbol(NT_STMT)); break;
        case RULE_MAIN_FLIST: stack_push(create_non_terminal_symbol(NT_FLIST)); break;
        case RULE_MAIN_EPSILON: break; 
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
        case RULE_STMT_IF: stack_push(create_non_terminal_symbol(NT_IFSTMT)); break;
        case RULE_STMT_BLOCK:
            stack_push(create_terminal_symbol(TOKEN_RBRACE));
            stack_push(create_non_terminal_symbol(NT_STMTLIST));
            stack_push(create_terminal_symbol(TOKEN_LBRACE));
            break;
        case RULE_STMT_SEMICOLON: stack_push(create_terminal_symbol(TOKEN_SEMICOLON)); break;
        case RULE_FLIST:
            stack_push(create_non_terminal_symbol(NT_FLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_FDEF));
            break;
        case RULE_FLIST_OPT:
            stack_push(create_non_terminal_symbol(NT_FLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_FDEF));
            break;
        case RULE_FLIST_OPT_EPSILON: break; 
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
        case RULE_PARLIST:
            stack_push(create_non_terminal_symbol(NT_PARLIST_TAIL));
            stack_push(create_terminal_symbol(TOKEN_ID));
            stack_push(create_terminal_symbol(TOKEN_INT));
            break;
        case RULE_PARLIST_EPSILON: break; 
        case RULE_PARLIST_TAIL:
            stack_push(create_non_terminal_symbol(NT_PARLIST));
            stack_push(create_terminal_symbol(TOKEN_COMMA));
            break;
        case RULE_PARLIST_TAIL_EPSILON: break; 
        case RULE_VARLIST:
            stack_push(create_non_terminal_symbol(NT_VARLIST_PRIME));
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_VARLIST_PRIME:
            stack_push(create_non_terminal_symbol(NT_VARLIST));
            stack_push(create_terminal_symbol(TOKEN_COMMA));
            break;
        case RULE_VARLIST_PRIME_EPSILON: break; 
        case RULE_ATRIBST:
            stack_push(create_non_terminal_symbol(NT_ATRIBST_TAIL));
            stack_push(create_terminal_symbol(TOKEN_ASSIGN));
            stack_push(create_terminal_symbol(TOKEN_ID));
            break;
        case RULE_ATRIBST_TAIL_EXPR: stack_push(create_non_terminal_symbol(NT_EXPR)); break;
        case RULE_ATRIBST_TAIL_FCALL: stack_push(create_non_terminal_symbol(NT_FCALL)); break;
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
        case RULE_PARLISTCALL_EPSILON: break; 
        case RULE_PARLISTCALL_TAIL:
            stack_push(create_non_terminal_symbol(NT_PARLISTCALL));
            stack_push(create_terminal_symbol(TOKEN_COMMA));
            break;
        case RULE_PARLISTCALL_TAIL_EPSILON: break; 
        case RULE_PRINTST:
            stack_push(create_non_terminal_symbol(NT_EXPR));
            stack_push(create_terminal_symbol(TOKEN_PRINT));
            break;
        case RULE_RETURNST:
            stack_push(create_non_terminal_symbol(NT_RETURN_TAIL));
            stack_push(create_terminal_symbol(TOKEN_RETURN));
            break;
        case RULE_RETURN_TAIL_ID: stack_push(create_terminal_symbol(TOKEN_ID)); break;
        case RULE_RETURN_TAIL_EPSILON: break; 
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
        case RULE_IF_TAIL_EPSILON: break; 
        case RULE_STMTLIST:
            stack_push(create_non_terminal_symbol(NT_STMTLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_STMT));
            break;
        case RULE_STMTLIST_OPT:
            stack_push(create_non_terminal_symbol(NT_STMTLIST_OPT));
            stack_push(create_non_terminal_symbol(NT_STMT));
            break;
        case RULE_STMTLIST_OPT_EPSILON: break; 
        case RULE_EXPR:
            stack_push(create_non_terminal_symbol(NT_EXPR_PRIME));
            stack_push(create_non_terminal_symbol(NT_NUMEXPR));
            break;
        case RULE_EXPR_PRIME:
            stack_push(create_non_terminal_symbol(NT_NUMEXPR));
            stack_push(create_non_terminal_symbol(NT_RELOP));
            break;
        case RULE_EXPR_PRIME_EPSILON: break; 
        case RULE_RELOP_LT: stack_push(create_terminal_symbol(TOKEN_LT)); break;
        case RULE_RELOP_LTE: stack_push(create_terminal_symbol(TOKEN_LTE)); break;
        case RULE_RELOP_GT: stack_push(create_terminal_symbol(TOKEN_GT)); break;
        case RULE_RELOP_GTE: stack_push(create_terminal_symbol(TOKEN_GTE)); break;
        case RULE_RELOP_EQ: stack_push(create_terminal_symbol(TOKEN_EQ)); break;
        case RULE_RELOP_NEQ: stack_push(create_terminal_symbol(TOKEN_NEQ)); break;
        case RULE_NUMEXPR:
            stack_push(create_non_terminal_symbol(NT_NUMEXPR_PRIME));
            stack_push(create_non_terminal_symbol(NT_TERM));
            break;
        case RULE_NUMEXPR_PRIME_ADDOP:
            stack_push(create_non_terminal_symbol(NT_NUMEXPR_PRIME));
            stack_push(create_non_terminal_symbol(NT_TERM));
            stack_push(create_non_terminal_symbol(NT_ADDOP));
            break;
        case RULE_NUMEXPR_PRIME_EPSILON: break; 
        case RULE_ADDOP_PLUS: stack_push(create_terminal_symbol(TOKEN_PLUS)); break;
        case RULE_ADDOP_MINUS: stack_push(create_terminal_symbol(TOKEN_MINUS)); break;
        case RULE_TERM:
            stack_push(create_non_terminal_symbol(NT_TERM_PRIME));
            stack_push(create_non_terminal_symbol(NT_FACTOR));
            break;
        case RULE_TERM_PRIME_MULOP:
            stack_push(create_non_terminal_symbol(NT_TERM_PRIME));
            stack_push(create_non_terminal_symbol(NT_FACTOR));
            stack_push(create_non_terminal_symbol(NT_MULOP));
            break;
        case RULE_TERM_PRIME_EPSILON: break; 
        case RULE_MULOP_MULT: stack_push(create_terminal_symbol(TOKEN_MULT)); break;
        case RULE_MULOP_DIV: stack_push(create_terminal_symbol(TOKEN_DIV)); break;
        case RULE_FACTOR_NUM: stack_push(create_terminal_symbol(TOKEN_NUM)); break;
        case RULE_FACTOR_ID: stack_push(create_terminal_symbol(TOKEN_ID)); break;
        case RULE_FACTOR_PAREN:
            stack_push(create_terminal_symbol(TOKEN_RPAREN));
            stack_push(create_non_terminal_symbol(NT_NUMEXPR));
            stack_push(create_terminal_symbol(TOKEN_LPAREN));
            break;
        
        default: break; 
    }
}

void initialize_parse_table() {
    for (int i = 0; i < 31; i++) {
        for (int j = 0; j < 30; j++) {
            parse_table[i][j] = RULE_ERROR;
        }
    }

    parse_table[NT_MAIN][TOKEN_INT] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_ID] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_PRINT] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_RETURN] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_IF] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_LBRACE] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_SEMICOLON] = RULE_MAIN_STMT;
    parse_table[NT_MAIN][TOKEN_DEF] = RULE_MAIN_FLIST;
    parse_table[NT_MAIN][TOKEN_EOF] = RULE_MAIN_EPSILON; 
    parse_table[NT_STMT][TOKEN_INT] = RULE_STMT_INT;
    parse_table[NT_STMT][TOKEN_ID] = RULE_STMT_ATRIB;
    parse_table[NT_STMT][TOKEN_PRINT] = RULE_STMT_PRINT;
    parse_table[NT_STMT][TOKEN_RETURN] = RULE_STMT_RETURN;
    parse_table[NT_STMT][TOKEN_IF] = RULE_STMT_IF;
    parse_table[NT_STMT][TOKEN_LBRACE] = RULE_STMT_BLOCK;
    parse_table[NT_STMT][TOKEN_SEMICOLON] = RULE_STMT_SEMICOLON;
    parse_table[NT_FLIST][TOKEN_DEF] = RULE_FLIST;
    parse_table[NT_FLIST_OPT][TOKEN_DEF] = RULE_FLIST_OPT;
    parse_table[NT_FLIST_OPT][TOKEN_EOF] = RULE_FLIST_OPT_EPSILON; 
    parse_table[NT_FDEF][TOKEN_DEF] = RULE_FDEF;
    parse_table[NT_PARLIST][TOKEN_INT] = RULE_PARLIST;
    parse_table[NT_PARLIST][TOKEN_RPAREN] = RULE_PARLIST_EPSILON; 
    parse_table[NT_PARLIST_TAIL][TOKEN_COMMA] = RULE_PARLIST_TAIL;
    parse_table[NT_PARLIST_TAIL][TOKEN_RPAREN] = RULE_PARLIST_TAIL_EPSILON; 
    parse_table[NT_VARLIST][TOKEN_ID] = RULE_VARLIST;
    parse_table[NT_VARLIST_PRIME][TOKEN_COMMA] = RULE_VARLIST_PRIME;
    parse_table[NT_VARLIST_PRIME][TOKEN_SEMICOLON] = RULE_VARLIST_PRIME_EPSILON; 
    parse_table[NT_ATRIBST][TOKEN_ID] = RULE_ATRIBST;
    parse_table[NT_ATRIBST_TAIL][TOKEN_NUM] = RULE_ATRIBST_TAIL_EXPR;
    parse_table[NT_ATRIBST_TAIL][TOKEN_LPAREN] = RULE_ATRIBST_TAIL_EXPR;
    parse_table[NT_ATRIBST_TAIL][TOKEN_ID] = RULE_ATRIBST_TAIL_EXPR; 

    parse_table[NT_FCALL][TOKEN_ID] = RULE_FCALL;
    parse_table[NT_PARLISTCALL][TOKEN_ID] = RULE_PARLISTCALL;
    parse_table[NT_PARLISTCALL][TOKEN_RPAREN] = RULE_PARLISTCALL_EPSILON; 
    parse_table[NT_PARLISTCALL_TAIL][TOKEN_COMMA] = RULE_PARLISTCALL_TAIL;
    parse_table[NT_PARLISTCALL_TAIL][TOKEN_RPAREN] = RULE_PARLISTCALL_TAIL_EPSILON; 
    parse_table[NT_PRINTST][TOKEN_PRINT] = RULE_PRINTST;
    parse_table[NT_RETURNST][TOKEN_RETURN] = RULE_RETURNST;
    parse_table[NT_RETURN_TAIL][TOKEN_ID] = RULE_RETURN_TAIL_ID;
    parse_table[NT_RETURN_TAIL][TOKEN_SEMICOLON] = RULE_RETURN_TAIL_EPSILON; 
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
    parse_table[NT_RELOP][TOKEN_LT] = RULE_RELOP_LT;
    parse_table[NT_RELOP][TOKEN_LTE] = RULE_RELOP_LTE;
    parse_table[NT_RELOP][TOKEN_GT] = RULE_RELOP_GT;
    parse_table[NT_RELOP][TOKEN_GTE] = RULE_RELOP_GTE;
    parse_table[NT_RELOP][TOKEN_EQ] = RULE_RELOP_EQ;
    parse_table[NT_RELOP][TOKEN_NEQ] = RULE_RELOP_NEQ;
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
    parse_table[NT_ADDOP][TOKEN_PLUS] = RULE_ADDOP_PLUS;
    parse_table[NT_ADDOP][TOKEN_MINUS] = RULE_ADDOP_MINUS;
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
    parse_table[NT_MULOP][TOKEN_MULT] = RULE_MULOP_MULT;
    parse_table[NT_MULOP][TOKEN_DIV] = RULE_MULOP_DIV;
    parse_table[NT_FACTOR][TOKEN_NUM] = RULE_FACTOR_NUM;
    parse_table[NT_FACTOR][TOKEN_LPAREN] = RULE_FACTOR_PAREN;
    parse_table[NT_FACTOR][TOKEN_ID] = RULE_FACTOR_ID;
}

void parse() {
    Token currentToken = getToken();

    stack_push(create_terminal_symbol(TOKEN_EOF));
    stack_push(create_non_terminal_symbol(NT_MAIN));

    while (stack_top > -1) {
        StackSymbol X = stack_pop();

        if (X.is_terminal) {
            if (X.value.terminal == currentToken.type) {
                if (currentToken.type == TOKEN_EOF) {
                    printf("\nAnálise Sintática concluída com sucesso.\n");
                    return; 
                }
                currentToken = getToken(); 
            } else {
                fprintf(stderr, "\n--- Erro Sintático ---\n");
                fprintf(stderr, "Token inesperado na linha %d, coluna %d.\n", currentToken.line, currentToken.col);
                fprintf(stderr, "Esperava: '%s'\n", token_type_to_string(X.value.terminal));
                fprintf(stderr, "Encontrou: '%s' (%s)\n", currentToken.lexeme, token_type_to_string(currentToken.type));
                return; 
            }
        } else {
            ProductionRule rule = parse_table[X.value.non_terminal][currentToken.type];

            if (rule == RULE_ERROR) {
                fprintf(stderr, "\n--- Erro Sintático ---\n");
                fprintf(stderr, "Token inesperado na linha %d, coluna %d.\n", currentToken.line, currentToken.col);
                fprintf(stderr, "Não esperava '%s' (%s) neste ponto.\n", currentToken.lexeme, token_type_to_string(currentToken.type));
                return; 
            }
            
            apply_rule(rule);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.lsi>\n", argv[0]);
        return 1;
    }

    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    symtable_init();
    initialize_parse_table();
    advance(); 

    parse();

    fclose(inputFile);
    return 0;
}