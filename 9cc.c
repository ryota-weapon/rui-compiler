#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>


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
};

// ASTのノードの種類
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs; // left-hand side
    Node *rhs; // right-hand side
    int val; // kindがND_NUMの時だけ
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// 現在のtoken pointer
Token *token;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}


char *user_input; // ユーザの入力したプログラム
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "X"); // pos個の空白になる
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}


// 期待と合致しているか？ それなら読み進めてtrue
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) {
        return false;
    }
    token = token->next;
    return true;
}

bool expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) {
        error("'%c'ではありません, ", op);
    }
    token = token->next;
}

int expect_number() {
    if (token -> kind != TK_NUM) 
        error("数ではない");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token)); // callocとは？
    // callocは、mallocと同じようにメモリ割り当てをするが、mallocと違い割り当てたバッファのゼロクリアをしてくれる
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    
    return tok;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}


Node *mul();

Node *expr() {
    Node *node = mul();
    
    for (;;) {
        if (consume('+')) 
            node = new_node(ND_ADD, node, mul());
        else if (consume('-')) 
            node = new_node(ND_SUB, node, mul());
        return node;
    }
}

Node *primary() {
    if (consume('(')) {
        Node *expr_node =  expr();
        expect(')');
        return expr_node;
    }

    return new_node_num(expect_number());
}

// mulを先に定義したい, 循環依存がある
Node *mul() {
    Node *node = primary();
    for (;;) {
        if (consume('*')) 
            node = new_node(ND_MUL, node, primary());
        else if (consume('/')) 
            node = new_node(ND_DIV, node, primary());
        else
            return node;
    }
}

void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %d\n", node->val); // 最後にスタックに1つ返り値が乗っているという状況にするのね
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    // やりたかった演算をやるよー

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    }

    printf("  push rax\n");
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が違うよ\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);
    
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}