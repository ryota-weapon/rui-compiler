.intel_syntax noprefix
.global main
main:
  push 1
  push 3
  imul rax, rdi
  push rax
  push 2
  push 5
  imul rax, rdi
  push rax
  add rax, rdi
  push rax
  pop rax
  ret
