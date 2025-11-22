/*
 * ============================================================================ 
 * ANALISADOR SINTATICO
 * ============================================================================ 
 *
 * Integrantes: Eduardo Boçon, Darnley Ribeiro, Jiliard Peifer e Olavo Ançay
 *
 * Este analisador sintatico implementa:
 *   - Analise preditiva tabular LL(1)
 *
 * ============================================================================ 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "analisador.h" 
#include "lexer.h"

#define MAX_STACK 1000

// Variáveis Globais do Lexer e Parser
Token token_atual;
Symbol current_symbol;

void erro_lexico(char *msg);

// ============================================================================ 
// FUNÇÕES AUXILIARES
// ============================================================================ 

// Mapeia TokenType (do lexer.h) para Symbol (do parser)
Symbol map_token_type(TokenType type) {
    switch(type) {
        case TOKEN_INT: return TOK_INT;
        case TOKEN_DEF: return TOK_DEF;
        case TOKEN_IF: return TOK_IF;
        case TOKEN_ELSE: return TOK_ELSE;
        case TOKEN_RETURN: return TOK_RETURN;
        case TOKEN_PRINT: return TOK_PRINT;
        case TOKEN_ID: return TOK_ID;
        case TOKEN_NUM: return TOK_NUM;
        case TOKEN_PLUS: return TOK_PLUS;
        case TOKEN_MINUS: return TOK_MINUS;
        case TOKEN_MULT: return TOK_MULT;
        case TOKEN_DIV: return TOK_DIV;
        case TOKEN_LPAREN: return TOK_LPAREN;
        case TOKEN_RPAREN: return TOK_RPAREN;
        case TOKEN_LBRACE: return TOK_LBRACE;
        case TOKEN_RBRACE: return TOK_RBRACE;
        case TOKEN_COMMA: return TOK_COMMA;
        case TOKEN_SEMICOLON: return TOK_SEMI;
        case TOKEN_ASSIGN: return TOK_ASSIGN;
        case TOKEN_EQ: return TOK_EQ;
        case TOKEN_NEQ: return TOK_NEQ;
        case TOKEN_LT: return TOK_LT;
        case TOKEN_LTE: return TOK_LE;
        case TOKEN_GT: return TOK_GT;
        case TOKEN_GTE: return TOK_GE;
        case TOKEN_EOF: return TOK_EOF;
        case TOKEN_ERROR:
            erro_lexico(token_atual.lexeme);
            return TOK_EOF; // Unreachable because erro_lexico exits
        default: 
            fprintf(stderr, "Erro: Token desconhecido ou não mapeado: %d\n", type);
            exit(1);
    }
}

// Wrapper para obter o próximo token e atualizar o estado global
void advance_token() {
    token_atual = getToken();
    current_symbol = map_token_type(token_atual.type);
}

// Converte Enum para String (Para Debug/Impressão)
const char* symbol_to_str(Symbol s) {
    switch(s) {
        case TOK_INT: return "int";
        case TOK_DEF: return "def";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_RETURN: return "return";
        case TOK_PRINT: return "print";
        case TOK_ID: return "id";
        case TOK_NUM: return "num";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_MULT: return "*";
        case TOK_DIV: return "/";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_COMMA: return ",";
        case TOK_SEMI: return ";";
        case TOK_ASSIGN: return "=";
        case TOK_EQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_LT: return "<";
        case TOK_LE: return "<=";
        case TOK_GT: return ">";
        case TOK_GE: return ">=";
        case TOK_EOF: return "$";
        case TOK_EPSILON: return "ε";
        
        case NT_MAIN: return "MAIN";
        case NT_STMT: return "STMT";
        case NT_FLIST: return "FLIST";
        case NT_FLIST_OPT: return "FLIST_OPT";
        case NT_FDEF: return "FDEF";
        case NT_VARLIST: return "VARLIST";
        case NT_VARLIST_PRIME: return "VARLIST'";
        case NT_ATRIBST: return "ATRIBST";
        case NT_ATRIBST_TAIL: return "ATRIBST_TAIL";
        case NT_FCALL: return "FCALL";
        case NT_PARLIST: return "PARLIST";
        case NT_PARLIST_TAIL: return "PARLIST_TAIL";
        case NT_PARLISTCALL: return "PARLISTCALL";
        case NT_PARLISTCALL_TAIL: return "PARLISTCALL_TAIL";
        case NT_PRINTST: return "PRINTST";
        case NT_RETURNST: return "RETURNST";
        case NT_RETURN_TAIL: return "RETURN_TAIL";
        case NT_IFSTMT: return "IFSTMT";
        case NT_IF_TAIL: return "IF_TAIL";
        case NT_STMTLIST: return "STMTLIST";
        case NT_STMTLIST_OPT: return "STMTLIST_OPT";
        case NT_EXPR: return "EXPR";
        case NT_EXPR_PRIME: return "EXPR'";
        case NT_NUMEXPR: return "NUMEXPR";
        case NT_NUMEXPR_PRIME: return "NUMEXPR'";
        case NT_TERM: return "TERM";
        case NT_TERM_PRIME: return "TERM'";
        case NT_FACTOR: return "FACTOR";
        case NT_RELOP: return "RELOP";
        case NT_ADDOP: return "ADDOP";
        case NT_MULOP: return "MULOP";
        default: return "UNKNOWN";
    }
}

void erro_lexico(char *msg) {
    fprintf(stderr, "ERRO LÉXICO [Linha %d, Coluna %d]: %s\n", token_atual.line, token_atual.col, msg);
    exit(1);
}

void erro_sintatico(char *msg) {
    fprintf(stderr, "\nERRO SINTÁTICO [Linha %d, Coluna %d]: %s\n", token_atual.line, token_atual.col, msg);
    fprintf(stderr, "Encontrado: '%s' (%s)\n", token_atual.lexeme, symbol_to_str(current_symbol));
    exit(1);
}


// ============================================================================ 
// PARTE 3: ANALISADOR SINTÁTICO PREDITIVO (TABULAR)
// ============================================================================ 

// Pilha do Parser
Symbol stack[MAX_STACK];
int top = -1;

void push(Symbol s) {
    if (top < MAX_STACK - 1) stack[++top] = s;
    else {
        fprintf(stderr, "Estouro de pilha do parser!\n");
        exit(1);
    }
}

Symbol pop() {
    if (top >= 0) return stack[top--];
    return TOK_EOF;
}

// Função auxiliar para verificar se é terminal
bool is_terminal(Symbol s) {
    return s < NT_MAIN; // Baseado na ordem do enum
}

/*
 * TABELA DE PARSING IMPLEMENTADA COMO FUNÇÃO
 * Retorna um array estático de símbolos que representa a produção (lado direito).
 * Retorna NULL se for erro.
 * O array termina com -1 (sentinela).
 * As produções devem ser empilhadas da direita para a esquerda (reverse),
 * mas aqui retornamos na ordem normal e o parser inverte.
 */
Symbol* get_production(Symbol nao_terminal, Symbol token_entrada) {
    static Symbol prod[20]; // Buffer estático reusável

    // Limpa buffer
    memset(prod, -1, sizeof(prod)); 
    int p = 0;

    // Mapeamento baseado no PDF (FIRST e FOLLOW sets)
    switch(nao_terminal) {
        case NT_MAIN:
            if (token_entrada == TOK_INT || token_entrada == TOK_ID || 
                token_entrada == TOK_PRINT || token_entrada == TOK_RETURN || 
                token_entrada == TOK_IF || token_entrada == TOK_LBRACE || 
                token_entrada == TOK_SEMI) {
                // MAIN -> STMT
                prod[p++] = NT_STMT;
            } else if (token_entrada == TOK_DEF) {
                // MAIN -> FLIST
                prod[p++] = NT_FLIST;
            } else if (token_entrada == TOK_EOF) {
                // MAIN -> ε
                prod[p++] = TOK_EPSILON;
            } else return NULL;
            break;

        case NT_STMT:
            if (token_entrada == TOK_INT) { prod[p++] = TOK_INT; prod[p++] = NT_VARLIST; prod[p++] = TOK_SEMI; }
            else if (token_entrada == TOK_ID) { prod[p++] = NT_ATRIBST; prod[p++] = TOK_SEMI; }
            else if (token_entrada == TOK_PRINT) { prod[p++] = NT_PRINTST; prod[p++] = TOK_SEMI; }
            else if (token_entrada == TOK_RETURN) { prod[p++] = NT_RETURNST; prod[p++] = TOK_SEMI; }
            else if (token_entrada == TOK_IF) { prod[p++] = NT_IFSTMT; }
            else if (token_entrada == TOK_LBRACE) { prod[p++] = TOK_LBRACE; prod[p++] = NT_STMTLIST; prod[p++] = TOK_RBRACE; }
            else if (token_entrada == TOK_SEMI) { prod[p++] = TOK_SEMI; }
            else return NULL;
            break;

        case NT_FLIST:
            if (token_entrada == TOK_DEF) { prod[p++] = NT_FDEF; prod[p++] = NT_FLIST_OPT; }
            else return NULL;
            break;

        case NT_FLIST_OPT:
            if (token_entrada == TOK_DEF) { prod[p++] = NT_FDEF; prod[p++] = NT_FLIST_OPT; }
            else if (token_entrada == TOK_EOF) { prod[p++] = TOK_EPSILON; }
            else return NULL;
            break;

        case NT_FDEF:
            if (token_entrada == TOK_DEF) { 
                prod[p++] = TOK_DEF; prod[p++] = TOK_ID; prod[p++] = TOK_LPAREN; 
                prod[p++] = NT_PARLIST; prod[p++] = TOK_RPAREN; prod[p++] = TOK_LBRACE; 
                prod[p++] = NT_STMTLIST; prod[p++] = TOK_RBRACE; 
            } else return NULL;
            break;

        case NT_VARLIST:
            if (token_entrada == TOK_ID) { prod[p++] = TOK_ID; prod[p++] = NT_VARLIST_PRIME; }
            else return NULL;
            break;

        case NT_VARLIST_PRIME:
            if (token_entrada == TOK_COMMA) { prod[p++] = TOK_COMMA; prod[p++] = NT_VARLIST; }
            else if (token_entrada == TOK_SEMI) { prod[p++] = TOK_EPSILON; }
            else return NULL;
            break;
            
        case NT_PARLIST:
            if (token_entrada == TOK_INT) { prod[p++] = TOK_INT; prod[p++] = TOK_ID; prod[p++] = NT_PARLIST_TAIL; }
            else if (token_entrada == TOK_RPAREN) { prod[p++] = TOK_EPSILON; }
            else return NULL;
            break;
            
        case NT_PARLIST_TAIL:
            if (token_entrada == TOK_COMMA) { prod[p++] = TOK_COMMA; prod[p++] = NT_PARLIST; }
            else if (token_entrada == TOK_RPAREN) { prod[p++] = TOK_EPSILON; }
            else return NULL;
            break;

        case NT_ATRIBST:
            if (token_entrada == TOK_ID) { prod[p++] = TOK_ID; prod[p++] = TOK_ASSIGN; prod[p++] = NT_ATRIBST_TAIL; }
            else return NULL;
            break;

        case NT_ATRIBST_TAIL:
            // ATRIBST_TAIL -> EXPR | FCALL
            // Simplificação assumindo EXPR para operações
            if (token_entrada == TOK_NUM || token_entrada == TOK_LPAREN || token_entrada == TOK_ID) {
                prod[p++] = NT_EXPR; 
            } else return NULL;
            break;
            
        case NT_EXPR:
             if (token_entrada == TOK_NUM || token_entrada == TOK_LPAREN || token_entrada == TOK_ID) {
                 prod[p++] = NT_NUMEXPR; prod[p++] = NT_EXPR_PRIME;
             } else return NULL;
             break;
             
        case NT_EXPR_PRIME:
             if (token_entrada == TOK_LT || token_entrada == TOK_LE || token_entrada == TOK_GT || 
                 token_entrada == TOK_GE || token_entrada == TOK_EQ || token_entrada == TOK_NEQ) {
                 prod[p++] = NT_RELOP; prod[p++] = NT_NUMEXPR;
             } else if (token_entrada == TOK_RPAREN || token_entrada == TOK_SEMI || token_entrada == TOK_COMMA) {
                 prod[p++] = TOK_EPSILON;
             } else return NULL;
             break;

        case NT_NUMEXPR:
             if (token_entrada == TOK_NUM || token_entrada == TOK_LPAREN || token_entrada == TOK_ID) {
                 prod[p++] = NT_TERM; prod[p++] = NT_NUMEXPR_PRIME;
             } else return NULL;
             break;
             
        case NT_NUMEXPR_PRIME:
             if (token_entrada == TOK_PLUS || token_entrada == TOK_MINUS) {
                 prod[p++] = NT_ADDOP; prod[p++] = NT_TERM; prod[p++] = NT_NUMEXPR_PRIME;
             } else if (token_entrada == TOK_LT || token_entrada == TOK_LE || token_entrada == TOK_GT ||
                        token_entrada == TOK_GE || token_entrada == TOK_EQ || token_entrada == TOK_NEQ ||
                        token_entrada == TOK_RPAREN || token_entrada == TOK_SEMI || token_entrada == TOK_COMMA) {
                 prod[p++] = TOK_EPSILON; // FOLLOW de NUMEXPR'
             } else return NULL;
             break;

        case NT_TERM:
             if (token_entrada == TOK_NUM || token_entrada == TOK_LPAREN || token_entrada == TOK_ID) {
                 prod[p++] = NT_FACTOR; prod[p++] = NT_TERM_PRIME;
             } else return NULL;
             break;

        case NT_TERM_PRIME:
             if (token_entrada == TOK_MULT || token_entrada == TOK_DIV) {
                 prod[p++] = NT_MULOP; prod[p++] = NT_FACTOR; prod[p++] = NT_TERM_PRIME;
             } else if (token_entrada == TOK_PLUS || token_entrada == TOK_MINUS ||
                        token_entrada == TOK_LT || token_entrada == TOK_LE || token_entrada == TOK_GT ||
                        token_entrada == TOK_GE || token_entrada == TOK_EQ || token_entrada == TOK_NEQ ||
                        token_entrada == TOK_RPAREN || token_entrada == TOK_SEMI || token_entrada == TOK_COMMA) {
                 prod[p++] = TOK_EPSILON; // FOLLOW de TERM'
             } else return NULL;
             break;

        case NT_FACTOR:
             if (token_entrada == TOK_NUM) prod[p++] = TOK_NUM;
             else if (token_entrada == TOK_ID) prod[p++] = TOK_ID;
             else if (token_entrada == TOK_LPAREN) { prod[p++] = TOK_LPAREN; prod[p++] = NT_NUMEXPR; prod[p++] = TOK_RPAREN; }
             else return NULL;
             break;

        case NT_ADDOP:
             if (token_entrada == TOK_PLUS) prod[p++] = TOK_PLUS;
             else if (token_entrada == TOK_MINUS) prod[p++] = TOK_MINUS;
             else return NULL;
             break;
             
        case NT_MULOP:
             if (token_entrada == TOK_MULT) prod[p++] = TOK_MULT;
             else if (token_entrada == TOK_DIV) prod[p++] = TOK_DIV;
             else return NULL;
             break;
             
        case NT_RELOP:
             if (token_entrada == TOK_LT) prod[p++] = TOK_LT;
             else if (token_entrada == TOK_LE) prod[p++] = TOK_LE;
             else if (token_entrada == TOK_GT) prod[p++] = TOK_GT;
             else if (token_entrada == TOK_GE) prod[p++] = TOK_GE;
             else if (token_entrada == TOK_EQ) prod[p++] = TOK_EQ;
             else if (token_entrada == TOK_NEQ) prod[p++] = TOK_NEQ;
             else return NULL;
             break;

        case NT_IFSTMT:
             if (token_entrada == TOK_IF) {
                 prod[p++] = TOK_IF; prod[p++] = TOK_LPAREN; prod[p++] = NT_EXPR;
                 prod[p++] = TOK_RPAREN; prod[p++] = TOK_LBRACE; prod[p++] = NT_STMTLIST;
                 prod[p++] = TOK_RBRACE; prod[p++] = NT_IF_TAIL;
             } else return NULL;
             break;

        case NT_IF_TAIL:
             if (token_entrada == TOK_ELSE) {
                 prod[p++] = TOK_ELSE; prod[p++] = TOK_LBRACE; prod[p++] = NT_STMTLIST; prod[p++] = TOK_RBRACE;
             } else if (token_entrada == TOK_SEMI || token_entrada == TOK_RBRACE || token_entrada == TOK_DEF || token_entrada == TOK_EOF ||
                        token_entrada == TOK_INT || token_entrada == TOK_ID || token_entrada == TOK_PRINT || 
                        token_entrada == TOK_RETURN || token_entrada == TOK_IF || token_entrada == TOK_LBRACE) {
                 // FOLLOW de IF_TAIL
                 prod[p++] = TOK_EPSILON;
             } else return NULL;
             break;
             
        case NT_STMTLIST:
             // STMTLIST -> STMT STMTLIST_OPT
             if (token_entrada == TOK_INT || token_entrada == TOK_ID || token_entrada == TOK_PRINT || 
                 token_entrada == TOK_RETURN || token_entrada == TOK_IF || token_entrada == TOK_LBRACE || 
                 token_entrada == TOK_SEMI) {
                 prod[p++] = NT_STMT; prod[p++] = NT_STMTLIST_OPT;
             } else if (token_entrada == TOK_RBRACE) {
                 // FOLLOW de STMTLIST (se lista vazia)
                 prod[p++] = TOK_EPSILON; 
             } else return NULL;
             break;
             
        case NT_STMTLIST_OPT:
             if (token_entrada == TOK_INT || token_entrada == TOK_ID || token_entrada == TOK_PRINT || 
                 token_entrada == TOK_RETURN || token_entrada == TOK_IF || token_entrada == TOK_LBRACE || 
                 token_entrada == TOK_SEMI) {
                 prod[p++] = NT_STMT; prod[p++] = NT_STMTLIST_OPT;
             } else if (token_entrada == TOK_RBRACE) {
                 prod[p++] = TOK_EPSILON;
             } else return NULL;
             break;
        
        case NT_PRINTST:
             if (token_entrada == TOK_PRINT) { prod[p++] = TOK_PRINT; prod[p++] = NT_EXPR; }
             else return NULL;
             break;

        case NT_RETURNST:
             if (token_entrada == TOK_RETURN) { prod[p++] = TOK_RETURN; prod[p++] = NT_RETURN_TAIL; }
             else return NULL;
             break;

        case NT_RETURN_TAIL:
             if (token_entrada == TOK_ID) { prod[p++] = TOK_ID; }
             else if (token_entrada == TOK_SEMI) { prod[p++] = TOK_EPSILON; }
             else return NULL;
             break;

        default:
            return NULL;
    }
    
    prod[p] = -1; // Sentinela fim do array
    return prod;
}

// Função Principal do Parser
void parse() {
    // Inicializa pilha
    top = -1;
    push(TOK_EOF); // $
    push(NT_MAIN); // Símbolo Inicial

    // Leitura inicial
    advance_token();

    printf("%-""20s | %-""20s | %s\n", "PILHA (Topo)", "ENTRADA", "AÇÃO");
    printf("----------------------------------------------------------------\n");

    while (top >= 0) {
        Symbol topo = stack[top]; // Peek

        printf("%-""20s | %-""20s | ", symbol_to_str(topo), token_atual.lexeme); 

        // CASO 1: Topo é ε (Epsilon)
        if (topo == TOK_EPSILON) {
            printf("Desempilha Vazio\n");
            pop();
            continue;
        }

        // CASO 2: Topo é Terminal
        if (is_terminal(topo)) {
            if (topo == current_symbol) {
                printf("Match %s\n", symbol_to_str(topo));
                pop();
                if (topo != TOK_EOF) {
                    advance_token();
                }
            } else {
                char msg[100];
                sprintf(msg, "Esperado terminal '%s', encontrado '%s'", symbol_to_str(topo), token_atual.lexeme);
                erro_sintatico(msg);
            }
        } 
        // CASO 3: Topo é Não-Terminal
        else {
            Symbol* prod = get_production(topo, current_symbol);
            if (prod == NULL) {
                char msg[100];
                sprintf(msg, "Nenhuma regra para %s com entrada %s", symbol_to_str(topo), token_atual.lexeme);
                erro_sintatico(msg);
            }

            printf("Produção Aplicada\n");
            pop(); // Remove Não-Terminal

            // Empilha a produção ao contrário (para o 1º símbolo ficar no topo)
            int len = 0;
            while(prod[len] != -1) len++;

            for (int i = len - 1; i >= 0; i--) {
                if (prod[i] != TOK_EPSILON) {
                    push(prod[i]);
                }
            }
        }
    }
    
    printf("\n----------------------------------------------------------------\n");
    printf("SUCESSO: Arquivo reconhecido pela gramática LSI-2025-2.\n");
}

// ============================================================================ 
// MAIN
// ============================================================================ 

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_entrada.lsi>\n", argv[0]);
        return 1;
    }

    FILE *arquivo_entrada = fopen(argv[1], "r");
    if (!arquivo_entrada) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    // Inicializa o lexer
    symtable_init();
    lexer_init(arquivo_entrada);

    printf("Iniciando Análise LSI-2025-2...\n\n");
    parse();

    symtable_free();
    fclose(arquivo_entrada);
    return 0;
}