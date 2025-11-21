// Integrantes: Eduardo Boçon, Darnley Ribeiro, Jiliard Peifer e Olavo Ançay.

#define _DEFAULT_SOURCE

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SYMBOL_TABLE_SIZE 100

typedef struct Symbol {
    char* lexeme;
    TokenType type;
    struct Symbol* next;
} Symbol;

Symbol* symbol_table[SYMBOL_TABLE_SIZE];

unsigned int hash(const char* str) {
    unsigned int hash_value = 0;
    while (*str) {
        hash_value = (hash_value << 5) + *str++;
    }
    return hash_value % SYMBOL_TABLE_SIZE;
}

void symtable_init() {
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        symbol_table[i] = NULL;
    }

    const char* keywords[] = {"int", "if", "else", "def", "print", "return"};
    TokenType types[] = {TOKEN_INT, TOKEN_IF, TOKEN_ELSE, TOKEN_DEF, TOKEN_PRINT, TOKEN_RETURN};
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

    for (int i = 0; i < num_keywords; i++) {
        unsigned int index = hash(keywords[i]);
        Symbol* new_symbol = (Symbol*)malloc(sizeof(Symbol));
        new_symbol->lexeme = strdup(keywords[i]);
        new_symbol->type = types[i];
        new_symbol->next = symbol_table[index];
        symbol_table[index] = new_symbol;
    }
}

Token symtable_lookup_insert(const char* lexeme, int line, int col) {
    unsigned int index = hash(lexeme);
    Symbol* current = symbol_table[index];

    while (current != NULL) {
        if (strcmp(current->lexeme, lexeme) == 0) {
            return (Token){current->type, current->lexeme, line, col};
        }
        current = current->next;
    }

    Symbol* new_symbol = (Symbol*)malloc(sizeof(Symbol));
    new_symbol->lexeme = strdup(lexeme);
    new_symbol->type = TOKEN_ID;
    new_symbol->next = symbol_table[index];
    symbol_table[index] = new_symbol;

    return (Token){TOKEN_ID, new_symbol->lexeme, line, col};
}

void symtable_print() {
    printf("\n--- Tabela de Símbolos ---\n");
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol* current = symbol_table[i];
        if (current) {
            printf("Bucket[%d]: ", i);
            while (current != NULL) {
                printf("('%s', %s) -> ", current->lexeme, token_type_to_string(current->type));
                current = current->next;
            }
            printf("NULL\n");
        }
    }
}

FILE* inputFile;
int line = 1;
int col = 1;
char currentChar;

void advance() {
    currentChar = fgetc(inputFile);
    if (currentChar == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
}

char peek() {
    char c = fgetc(inputFile);
    ungetc(c, inputFile);
    return c;
}

#define LEXEME_BUFFER_SIZE 256
char lexemeBuffer[LEXEME_BUFFER_SIZE];

Token create_error_token(const char* message) {
    char* errorLexeme = (char*)malloc(LEXEME_BUFFER_SIZE);
    snprintf(errorLexeme, LEXEME_BUFFER_SIZE, "ERRO: %s", message);
    return (Token){TOKEN_ERROR, errorLexeme, line, col - 1};
}

Token getToken() {
    int pos = 0;

    while (isspace(currentChar)) {
        advance();
    }

    int startLine = line;
    int startCol = col - 1;

    if (currentChar == EOF) {
        return (Token){TOKEN_EOF, "EOF", startLine, startCol};
    }

    if (isdigit(currentChar)) {
        while (isdigit(currentChar)) {
            lexemeBuffer[pos++] = currentChar;
            advance();
        }
        lexemeBuffer[pos] = '\0';
        return (Token){TOKEN_NUM, strdup(lexemeBuffer), startLine, startCol};
    }

    if (isalpha(currentChar) || currentChar == '_') {
        while (isalnum(currentChar) || currentChar == '_') {
            lexemeBuffer[pos++] = currentChar;
            advance();
        }
        lexemeBuffer[pos] = '\0';
        return symtable_lookup_insert(lexemeBuffer, startLine, startCol);
    }

    if (currentChar == '=') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_EQ, "==", startLine, startCol};
        }
        return (Token){TOKEN_ASSIGN, "=", startLine, startCol};
    }
    if (currentChar == '!') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_NEQ, "!=", startLine, startCol};
        }
        return create_error_token("Caractere '!' inesperado. Esperava '!='?");
    }
    if (currentChar == '<') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_LTE, "<=", startLine, startCol};
        }
        return (Token){TOKEN_LT, "<", startLine, startCol};
    }
    if (currentChar == '>') {
        advance();
        if (currentChar == '=') {
            advance();
            return (Token){TOKEN_GTE, ">=", startLine, startCol};
        }
        return (Token){TOKEN_GT, ">", startLine, startCol};
    }

    switch (currentChar) {
        case '+': advance(); return (Token){TOKEN_PLUS, "+", startLine, startCol};
        case '-': advance(); return (Token){TOKEN_MINUS, "-", startLine, startCol};
        case '*': advance(); return (Token){TOKEN_MULT, "*", startLine, startCol};
        case '/': advance(); return (Token){TOKEN_DIV, "/", startLine, startCol};
        case '(': advance(); return (Token){TOKEN_LPAREN, "(", startLine, startCol};
        case ')': advance(); return (Token){TOKEN_RPAREN, ")", startLine, startCol};
        case '{': advance(); return (Token){TOKEN_LBRACE, "{", startLine, startCol};
        case '}': advance(); return (Token){TOKEN_RBRACE, "}", startLine, startCol};
        case ',': advance(); return (Token){TOKEN_COMMA, ",", startLine, startCol};
        case ';': advance(); return (Token){TOKEN_SEMICOLON, ";", startLine, startCol};
    }

    char errorMsg[50];
    snprintf(errorMsg, 50, "Caractere inválido: '%c'", currentChar);
    advance();
    return create_error_token(errorMsg);
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
    advance();

    Token token;
    int hasError = 0;

    printf("--- Lista de Tokens ---\n");
    do {
        token = getToken();
        printf("[Linha %d, Col %d]\t %-12s \t '%s'\n",
               token.line, token.col,
               token_type_to_string(token.type), token.lexeme);

        if (token.type == TOKEN_ERROR) {
            fprintf(stderr, "\nERRO LÉXICO: %s na linha %d, coluna %d\n",
                   token.lexeme, token.line, token.col);
            hasError = 1;
            break;
        }
    } while (token.type != TOKEN_EOF);

    if (!hasError) {
        symtable_print();
    }

    fclose(inputFile);
    
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol* current = symbol_table[i];
        while (current != NULL) {
            Symbol* temp = current;
            current = current->next;
            free(temp->lexeme);
            free(temp);
        }
    }

    return hasError;
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_IF: return "TOKEN_IF";
        case TOKEN_ELSE: return "TOKEN_ELSE";
        case TOKEN_DEF: return "TOKEN_DEF";
        case TOKEN_PRINT: return "TOKEN_PRINT";
        case TOKEN_RETURN: return "TOKEN_RETURN";
        case TOKEN_ID: return "TOKEN_ID";
        case TOKEN_NUM: return "TOKEN_NUM";
        case TOKEN_LT: return "TOKEN_LT";
        case TOKEN_LTE: return "TOKEN_LTE";
        case TOKEN_GT: return "TOKEN_GT";
        case TOKEN_GTE: return "TOKEN_GTE";
        case TOKEN_EQ: return "TOKEN_EQ";
        case TOKEN_NEQ: return "TOKEN_NEQ";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_MULT: return "TOKEN_MULT";
        case TOKEN_DIV: return "TOKEN_DIV";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        case TOKEN_LPAREN: return "TOKEN_LPAREN";
        case TOKEN_RPAREN: return "TOKEN_RPAREN";
        case TOKEN_LBRACE: return "TOKEN_LBRACE";
        case TOKEN_RBRACE: return "TOKEN_RBRACE";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_ERROR: return "TOKEN_ERROR";
        default: return "TOKEN_UNKNOWN";
    }
}