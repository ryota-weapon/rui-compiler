#include <stdlib.h>
#include "9cc.h"


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


Node *expr() {
    return equality();
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

Node *primary() {
    if (consume("(")) {
        Node *expr_node =  expr();
        expect(")");
        return expr_node;
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
