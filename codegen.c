#include <stdio.h>
#include "9cc.h"


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
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n"); // seteは8ビットレジスタしか受け取れない
        printf("  movzb rax, al\n"); // 上位56ビットをゼロクリアしながら、raxを更新
        break;
    case ND_NEQ:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
    case ND_LTE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
    // case ND_GT:
    // case ND_GTE: これらはパーサー側で処理するよ
    }
    printf("  push rax\n");
}
