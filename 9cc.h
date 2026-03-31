#ifndef NINECC_H
#define NINECC_H

#include <stdbool.h>

typedef enum {
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
} TokenKind;

// 自己参照に使う構文らしい, (TokenがTokenを参照しているでしょ、このstruct)
typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len; // 記号トークンの長さが1文字から可変長になった
};

// ASTのノードの種類
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_LT,
    ND_LTE,
    // ND_GT,
    // ND_GTE,
    ND_EQ,
    ND_NEQ,
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs; // left-hand side
    Node *rhs; // right-hand side
    int val; // kindがND_NUMの時だけ
};

// parser
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// general
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
bool consume(char *op);
void expect(char *op);
int expect_number();


// gen
void gen(Node *node);

// tokenizer
Token *new_token(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *p);


// global
extern char *user_input;
extern Token *token;

// util
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);


#endif
