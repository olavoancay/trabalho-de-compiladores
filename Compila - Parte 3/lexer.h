#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef enum {
    TOKEN_INT,      // int
    TOKEN_IF,       // if
    TOKEN_ELSE,     // else
    TOKEN_DEF,      // def
    TOKEN_PRINT,    // print
    TOKEN_RETURN,   // return
    TOKEN_ID,       // main, var, func1
    TOKEN_NUM,      // 123, 0, 42
    TOKEN_LT,       // <
    TOKEN_LTE,      // <=
    TOKEN_GT,       // >
    TOKEN_GTE,      // >=
    TOKEN_EQ,       // ==
    TOKEN_NEQ,      // !=
    TOKEN_PLUS,     // +
    TOKEN_MINUS,    // -
    TOKEN_MULT,     // *
    TOKEN_DIV,      // /
    TOKEN_ASSIGN,   // =
    TOKEN_LPAREN,   // (
    TOKEN_RPAREN,   // )
    TOKEN_LBRACE,   // {
    TOKEN_RBRACE,   // }
    TOKEN_COMMA,    // ,
    TOKEN_SEMICOLON, // ;
    TOKEN_EOF,      // Fim
    TOKEN_ERROR     // Erro
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int col;
} Token;

void lexer_init(FILE* f);
void symtable_init(void);
void symtable_free(void);
const char* token_type_to_string(TokenType type);
Token getToken(void);

#endif