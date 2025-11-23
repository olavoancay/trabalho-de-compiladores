/*
 * ============================================================================
 * ANALISADOR LÉXICO PARA A LINGUAGEM LSI-2025-2
 * ============================================================================
 *
 * Integrantes: Eduardo Boçon, Darnley Ribeiro, Jiliard Peifer e Olavo Ançay
 *
 * Este analisador léxico implementa:
 *   - Leitura caractere por caractere
 *   - Reconhecimento baseado em diagramas de transição (autômatos)
 *   - Tabela de símbolos com técnica "maximal munch"
 *   - Detecção de erros léxicos com linha e coluna
 *
 * ============================================================================
 */

#define _DEFAULT_SOURCE

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define SYMBOL_TABLE_SIZE 100
#define LEXEME_BUFFER_SIZE 256

/* ============================================================================
 * TABELA DE SÍMBOLOS
 * ============================================================================ */

typedef struct Symbol {
    char* lexeme;
    TokenType type;
    struct Symbol* next;
} Symbol;

static Symbol* symbol_table[SYMBOL_TABLE_SIZE];

/*
 * hash(str)
 *
 * Função hash para strings usando o método shift-add.
 * Distribui chaves de forma uniforme entre os buckets da tabela.
 */
static unsigned int hash(const char* str) {
    unsigned int hash_value = 0;
    while (*str) {
        hash_value = (hash_value << 5) + *str++;
    }
    return hash_value % SYMBOL_TABLE_SIZE;
}

/*
 * symtable_init()
 *
 * Inicializa a tabela de símbolos com as palavras-chave da linguagem.
 * As palavras-chave são inseridas diretamente na tabela durante a inicialização.
 */
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

/*
 * symtable_lookup_insert(lexeme, line, col)
 *
 * Consulta a tabela de símbolos procurando pelo lexema.
 * Se encontrado como palavra-chave, retorna seu tipo.
 * Se não encontrado, insere como identificador (TOKEN_ID).
 *
 * Implementa a técnica "maximal munch": reconhece o identificador
 * e consulta a tabela para verificar se é palavra-chave.
 */
Token symtable_lookup_insert(const char* lexeme, int line, int col) {
    unsigned int index = hash(lexeme);
    Symbol* current = symbol_table[index];

    while (current != NULL) {
        if (strcmp(current->lexeme, lexeme) == 0) {
            return (Token){current->type, strdup(lexeme), line, col};
        }
        current = current->next;
    }

    Symbol* new_symbol = (Symbol*)malloc(sizeof(Symbol));
    new_symbol->lexeme = strdup(lexeme);
    new_symbol->type = TOKEN_ID;
    new_symbol->next = symbol_table[index];
    symbol_table[index] = new_symbol;

    return (Token){TOKEN_ID, strdup(lexeme), line, col};
}

/*
 * symtable_print()
 *
 * Imprime o conteúdo da tabela de símbolos para fins de debug.
 * Exibe cada bucket e seus símbolos encadeados.
 */
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

/* ============================================================================
 * VARIÁVEIS GLOBAIS
 * ============================================================================ */

FILE* inputFile;
int line = 1;
int col = 1;
char currentChar;

/* ============================================================================
 * FUNÇÕES DE LEITURA DO ARQUIVO
 * ============================================================================ */

/*
 * advance()
 *
 * Lê o próximo caractere do arquivo e atualiza linha e coluna.
 * Trata corretamente diferentes tipos de quebra de linha:
 *   - Unix: \n
 *   - Windows: \r\n
 *   - Antigo (Mac): \r
 */
void advance() {
    currentChar = fgetc(inputFile);
    if (currentChar == '\r') {
        int next = fgetc(inputFile);
        if (next == '\n') {
            currentChar = '\n';
            line++;
            col = 1;
        } else {
            ungetc(next, inputFile);
            currentChar = '\n';
            line++;
            col = 1;
        }
    } else if (currentChar == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
}

/*
 * peek()
 *
 * Espira o próximo caractere sem consumi-lo.
 * Retorna o caractere e repositiona o ponteiro do arquivo.
 */
char peek() {
    char c = fgetc(inputFile);
    ungetc(c, inputFile);
    return c;
}

/* ============================================================================
 * FUNÇÕES DE TOKENIZAÇÃO
 * ============================================================================ */

/*
 * create_error_token(message)
 *
 * Cria um token de erro com a mensagem especificada.
 * Retorna um token do tipo TOKEN_ERROR com o prefixo "ERRO: ".
 */
Token create_error_token(const char* message) {
    char* errorLexeme = (char*)malloc(LEXEME_BUFFER_SIZE);
    snprintf(errorLexeme, LEXEME_BUFFER_SIZE, "ERRO: %s", message);
    return (Token){TOKEN_ERROR, errorLexeme, line, col - 1};
}

static char lexemeBuffer[LEXEME_BUFFER_SIZE];

/*
 * getToken()
 *
 * Lê um token do arquivo de entrada.
 * Segue diagramas de transição para reconhecer:
 *   - Números (sequência de dígitos)
 *   - Identificadores e palavras-chave (letra/underscore seguido de letras, dígitos ou underscores)
 *   - Operadores simples: +, -, *, /, (, ), {, }, ,, ;
 *   - Operadores compostos: ==, !=, <=, >=
 *   - Atribuição: =
 *   - Comparação: <, >, =, !
 *
 * Retorna o token encontrado ou TOKEN_ERROR se caractere inválido.
 */
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

    /*
     * Reconhecimento de números.
     */
    if (isdigit(currentChar)) {
        while (isdigit(currentChar)) {
            lexemeBuffer[pos++] = currentChar;
            advance();
        }
        lexemeBuffer[pos] = '\0';
        return (Token){TOKEN_NUM, strdup(lexemeBuffer), startLine, startCol};
    }

    /*
     * Reconhecimento de identificadores e palavras-chave.
     */
    if (isalpha(currentChar) || currentChar == '_') {
        while (isalnum(currentChar) || currentChar == '_') {
            lexemeBuffer[pos++] = currentChar;
            advance();
        }
        lexemeBuffer[pos] = '\0';
        return symtable_lookup_insert(lexemeBuffer, startLine, startCol);
    }

    /*
     * Reconhecimento de operadores compostos e atribuição.
     */
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

    /*
     * Reconhecimento de operadores simples e símbolos especiais.
     */
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

    /*
     * Caractere inválido não reconhecido por nenhum diagrama de transição.
     */
    char errorMsg[50];
    snprintf(errorMsg, 50, "Caractere inválido: '%c'", currentChar);
    advance();
    return create_error_token(errorMsg);
}

/*
 * token_type_to_string(type)
 *
 * Converte um tipo de token em sua representação textual.
 * Usada para mensagens de erro e debug.
 */
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
