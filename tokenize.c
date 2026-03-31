#include <stdlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "9cc.h"


// 期待と合致しているか？ それなら読み進めてtrue
bool consume(char *op) {
    if (token->kind != TK_RESERVED 
        || strlen(op) != token->len
        || memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

void expect(char *op) {
    if (token->kind != TK_RESERVED || 
    strlen(op) != token->len ||
    memcmp(token->str, op, token->len)) {
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

        if (strncmp(p, "==", 2)==0 || strncmp(p, "!=", 2)==0 || strncmp(p, ">=", 2)==0 || strncmp(p, "<=", 2)==0) {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 2;
            p++;
            continue;
        }
        
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' ||
            *p == '<' || *p == '>'
        ) {
            cur = new_token(TK_RESERVED, cur, p++);
            cur->len = 1;
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
