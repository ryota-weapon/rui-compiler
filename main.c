#include <stdio.h>
#include "9cc.h"


Token *token; // 現在のtoken pointer
char *user_input; // ユーザの入力したプログラム

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

    printf("\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n"); // スタックは実行であるとリンカに伝える
    return 0;
}
