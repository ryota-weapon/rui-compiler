#include <stdlib.h>
#include <string.h>
#include "9cc.h"

#include <stdio.h> // debug


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

// LVarの連結リストから探し出す
LVar *find_lvar(Token *tok) {
    for (LVar *var=locals; var; var = var->next) {
        if (tok->len == var->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}


Node *program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *stmt() {
    Node *node;

    // if (consume(TK_RETURN)) {
    // consumeは現状Returnに対応していない
    if (consume("return")) { // i think this should be like this??
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else if (consume("while")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->cond = expr();
        expect(")");
        node->body = stmt();
        return node; // 下にある;の存在チェックと重複してしまって2個必要になってしまうため
    } else if (consume("for")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("(");
        node->init = expr();
        expect(";");
        node->cond = expr();
        expect(";");
        node->inc = expr();
        expect(")");
        node->body = stmt();

        return node;
    } else if (consume("if")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->cond = expr();
        expect(")");
        node->body = stmt();

        if (consume("else")) {
            node->_else = stmt();
        }
        return node;
    } else if (consume("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;

        Node **node_buf = calloc(1, sizeof(Node)*10); // 一旦固定で10行までサポート
        int i = 0;
        while (!consume("}")) {
            node_buf[i++] = stmt();
            if (i==10)
                error("too bigなブロックは今サポートされていない.");
        }
        node->stmt_len = i;
        node->stmt = node_buf;
        return node;
    } else {
        node = expr();
    }

    if (!consume(";"))
        error_at(token->str, "';'ではないトークンです");

    return node;
}

Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        // node->kind = ND_LVAR; // 誤植？　これ必要では？
        // 現状だけかも a=b=cみたいなことができちゃうよ
        node = new_node(ND_ASSIGN, node, assign()); // というか、ここassignが適切なのか？？
    }
    
    return node;
}

Node *expr() {
    return assign();
}

Node *relational() {
    Node *node = add();
    for (;;) {
        if (consume("<=")) {
            node = new_node(ND_LTE, node, add());
        }
        if (consume(">=")) {
            node = new_node(ND_LTE, add(), node);
        }
        if (consume("<")) {
            node = new_node(ND_LT, node, add());
        }
        if (consume(">")) {
            node = new_node(ND_LT, add(), node);
        }

        return node;
    }
}

Node *equality() {
    Node *node = relational();
    for (;;) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        }
        if (consume("!=")) {
            node = new_node(ND_NEQ, node, relational());
        }
        return node;
    }
}

Node *add() {
    Node *node = mul();
    
    for (;;) {
        if (consume("+")) 
            node = new_node(ND_ADD, node, mul());
        else if (consume("-")) 
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

//  tokenを構成する文字か否か？ 
// (returnかどうかの判定に使う、returnの後ろを見る)
int is_alnum(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

// この関数が使われているところから、推察して実装
// おそらく変数だったら、そのトークンのポインタを返せばよい
Token *consume_ident() {
    extern Token *token;
    // // このtokenは、グローバル変数

    // 未登録ならとりあえず作るという仕様にしようかな
    if (token->kind != TK_IDENT) 
        return NULL;

    return token;
}

Node *primary() {
    if (consume("(")) {
        Node *expr_node =  expr();
        expect(")");
        return expr_node;
    }
    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->offset = lvar->offset;
        } else {
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            // localsリストが空であり、localsポインタがNULLの時にバグる
            if (locals) {
                lvar->offset = locals->offset + 8;
            } else {
                lvar->offset = 8;
            }
            locals = lvar;
            node->offset = lvar->offset;
        }
    
        token = token->next;
        // 関数callかもしれない
        if (consume("(")) {
            int count = 0;
            Node **args = calloc(1, sizeof(Node)*10);
            Node *first_arg = consume_arg();
            if (first_arg) 
                args[0] = first_arg;

            while (!consume(")")) {
                expect(",");
                args[count++] = consume_arg();
            }
            
            //　非効率なコードだけどうわがきする
            node = (Node *)calloc(1, sizeof(Node));
            node->kind = ND_FUNC_CALL;
            node->offset = locals->offset;
            node->args = args;
            node->arg_len = count;
        }
        // TODO: align RSP to 16byte due to the ABI
        return node;
    }

    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+")) {
        return primary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    }
    return primary();
}

// mulを先に定義したい, 循環依存がある
Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume("*")) 
            node = new_node(ND_MUL, node, unary());
        else if (consume("/")) 
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

