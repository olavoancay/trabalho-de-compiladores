#ifndef ANALISADOR_H
#define ANALISADOR_H

#include <stdbool.h>
#include "lexer.h"

// ============================================================================
// DEFINIÇÕES E ENUMS (Tabela de Símbolos e Tipos)
// ============================================================================

// Enum unificado para Terminais e Não-Terminais
typedef enum {
    // --- TERMINAIS (Tokens) ---
    TOK_INT,        // int
    TOK_DEF,        // def
    TOK_IF,         // if
    TOK_ELSE,       // else
    TOK_RETURN,     // return
    TOK_PRINT,      // print
    TOK_ID,         // identificador
    TOK_NUM,        // constante numérica
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_MULT,       // *
    TOK_DIV,        // /
    TOK_LPAREN,     // (
    TOK_RPAREN,     // )
    TOK_LBRACE,     // {
    TOK_RBRACE,     // }
    TOK_COMMA,      // ,
    TOK_SEMI,       // ;
    TOK_ASSIGN,     // =
    TOK_EQ,         // ==
    TOK_NEQ,        // !=
    TOK_LT,         // <
    TOK_LE,         // <=
    TOK_GT,         // >
    TOK_GE,         // >=
    TOK_EOF,        // $ (Fim de arquivo)
    TOK_EPSILON,    // ε (Vazio)

    // --- NÃO-TERMINAIS (Gramática Transformada) ---
    NT_MAIN,
    NT_STMT,
    NT_FLIST,
    NT_FLIST_OPT,
    NT_FDEF,
    NT_VARLIST,
    NT_VARLIST_PRIME,
    NT_ATRIBST,
    NT_ATRIBST_TAIL,
    NT_FCALL,
    NT_PARLIST,
    NT_PARLIST_TAIL,
    NT_PARLISTCALL,      // Adicionado conforme gramática
    NT_PARLISTCALL_TAIL, // Adicionado conforme gramática
    NT_PRINTST,
    NT_RETURNST,
    NT_RETURN_TAIL,
    NT_IFSTMT,
    NT_IF_TAIL,
    NT_STMTLIST,
    NT_STMTLIST_OPT,
    NT_EXPR,
    NT_EXPR_PRIME,
    NT_NUMEXPR,
    NT_NUMEXPR_PRIME,
    NT_TERM,
    NT_TERM_PRIME,
    NT_FACTOR,
    NT_RELOP,
    NT_ADDOP,
    NT_MULOP,
    
    SYMBOL_COUNT // Apenas para contagem
} Symbol;

// Variáveis Globais (Expostas para uso em módulos se necessário, ou mantidas internas)
// Por enquanto, mantemos as funções principais expostas

// Protótipos
void parse();
const char* symbol_to_str(Symbol s);
void erro_sintatico(char *msg);

#endif
