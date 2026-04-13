#!/bin/sh

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if  [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

# assert 0 0
# assert 42 42

# assert 21 "5+20-4"

# assert 21 " 5 + 20 -  4"
# assert 41 " 12 + 34 - 5 "

# assert 47 '5+6*7'
# assert 15 '5*(9-6)'
# assert 4 '(3+5)/2'
# assert 6 '3+(3+6*7)/(1+2*2)-2*3'

# assert 3 '+3'
# assert 10 "-10+20"

# assert 1 '3==3'
# assert 0 '2>=3'
# assert 1 '2<3'
# assert 0 '2!=2'
# assert 1 '2!=2+1'
# assert 1 '1*3+2!=2*3'

# assert 2 '2;'
# assert 3 'return 3;'
# assert 5 'a=1;b=4;return a+b;'
# assert 15 'a=3;b=4;return a*b+3;'

# assert 2 'a=2; a;'
# assert 3 'a=1; a=a+2; a;'
# assert 6 'a=1; b=2; c=3; return a+b+c;'
# assert 11 'a=3; b=4; a+b*2;'
# assert 14 'a=2; b=3; return (a+b)*2+4;'
# assert 3 'a=1; b=a+2; return b;'
# assert 7 'a=3; b=4; a=a+b; return a;'
# assert 4 'a=1; b=2; a=b=4; return a;'
# assert 4 'abc=1; xyz=2; abc=xyz=4; return xyz;'
# assert 1 'a=3; b=4; return a<b;'
# assert 1 'hoge=3; fuga=3; return hoge==fuga;'
# assert 0 'hoge=3; fuga=4; return hoge==fuga;'


# assert 11 'a=0;while (a<=5)a=a+1; a = a+5; return a;'
# assert 128 'a=2;while (a<=100)a=a*2; return a;'

# assert 2 'a=2;if (a<=10) return 2; return 10;'
# assert 10 'a=2;if (a>10) return 2; return 10;'
# assert 2 'a=2;if (a==2) return 2; else return 10;'
# assert 10 'a=2;if (a==10) return 2; else return 10;'
# assert 3 'a=2;if (a-2) return 2; else a=a+1; return a;'
# assert 5 'hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;'

# assert 45 'hoge=0; for (i=0; i<10; i=i+1) hoge = hoge + i; return hoge;'
# assert 10 'hoge=0; for (i=0; i<10; i=i+1) hoge = hoge + 1; return hoge;'
# assert 12 'hoge=5; for (i=3; i<10; i=i+1) hoge = hoge + 1; return hoge;'

# assert 6 'a=2;if (a-2) {return 2;} else {a=a+1; a=a+3;} return a;'


# assert 2 'a=1; if (a) { return 2; } return 3;'
# assert 3 'a=0; if (a) { return 2; } return 3;'
# assert 2 'a=1; if (a) { return 2; } else { return 3; }'
# assert 3 'a=0; if (a) { return 2; } else { return 3; }'
# assert 2 '{ a=2; } return a;'
# assert 3 'a=1; { a=a+2; } return a;'
# assert 6 'a=1; { b=2; c=3; } return a+b+c;'

# # func
# assert 0 'func();'
# assert 0 'func(2);'
# assert 0 'func(3, 4);'
# assert 0 'a=1; b=2; func(a, b);'
# assert 0 'func(1);'

# assert 2 'int main() { 2;}'
# assert 3 'int main() { return 3;}'
# assert 5 'int main() { a=1;b=4;return a+b;}'
# assert 15 'int main() { a=3;b=4;return a*b+3;}'

# assert 2 'int main() { a=2; a;}'
# assert 3 'int main() { a=1; a=a+2; a;}'
# assert 6 'int main() { a=1; b=2; c=3; return a+b+c;}'
# assert 11 'int main() { a=3; b=4; a+b*2;}'
# assert 14 'int main() { a=2; b=3; return (a+b)*2+4;}'
# assert 3 'int main() { a=1; b=a+2; return b;}'
# assert 7 'int main() { a=3; b=4; a=a+b; return a;}'
# assert 4 'int main() { a=1; b=2; a=b=4; return a;}'
# assert 4 'int main() { abc=1; xyz=2; abc=xyz=4; return xyz;}'
# assert 1 'int main() { a=3; b=4; return a<b;}'
# assert 1 'int main() { hoge=3; fuga=3; return hoge==fuga;}'
# assert 0 'int main() { hoge=3; fuga=4; return hoge==fuga;}'


# assert 11 'int main() { a=0;while (a<=5)a=a+1; a = a+5; return a;}'
# assert 128 'int main() { a=2;while (a<=100)a=a*2; return a;}'

# assert 2 'int main() { a=2;if (a<=10) return 2; return 10;}'
# assert 10 'int main() { a=2;if (a>10) return 2; return 10;}'
# assert 2 'int main() { a=2;if (a==2) return 2; else return 10;}'
# assert 10 'int main() { a=2;if (a==10) return 2; else return 10;}'
# assert 3 'int main() { a=2;if (a-2) return 2; else a=a+1; return a;}'
# assert 5 'int main() { hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'

# assert 45 'int main() { hoge=0; for (i=0; i<10; i=i+1) hoge = hoge + i; return hoge;}'
# assert 10 'int main() { hoge=0; for (i=0; i<10; i=i+1) hoge = hoge + 1; return hoge;}'
# assert 12 'int main() { hoge=5; for (i=3; i<10; i=i+1) hoge = hoge + 1; return hoge;}'

# assert 6 'int main() { a=2;if (a-2) {return 2;} else {a=a+1; a=a+3;} return a;}'


# assert 2 'int main() { a=1; if (a) { return 2; } return 3;}'
# assert 3 'int main() { a=0; if (a) { return 2; } return 3;}'
# assert 2 'int main() { a=1; if (a) { return 2; } else { return 3; }}'
# assert 3 'int main() { a=0; if (a) { return 2; } else { return 3; }}'
# assert 2 'int main() { { a=2; } return a;}'
# assert 3 'int main() { a=1; { a=a+2; } return a;}'
# assert 6 'int main() { a=1; { b=2; c=3; } return a+b+c;}'

assert 3 'int main() { return 3;}'
assert 5 'int main() { int a; int b; a=1;b=4;return a+b;}'
assert 15 'int main() { int a; int b; a=3;b=4;return a*b+3;}'

assert 6 'int main() { int a,b,c; a=1; b=2; c=3; return a+b+c;}'
assert 11 'int main() { int a,b;a=3; b=4; a+b*2;}'
assert 14 'int main() { int a,b; a=2; b=3; return (a+b)*2+4;}'
assert 3 'int main() { int a,b; a=1; b=a+2; return b;}'
assert 7 'int main() { int a,b; a=3; b=4; a=a+b; return a;}'
assert 4 'int main() { int a,b; a=1; b=2; a=b=4; return a;}'
assert 4 'int main() { int abc, xyz; abc=1; xyz=2; abc=xyz=4; return xyz;}'
assert 1 'int main() { int a,b; a=3; b=4; return a<b;}'
assert 1 'int main() { int hoge; int fuga; hoge=3; fuga=3; return hoge==fuga;}'
assert 0 'int main() { int hoge; int fuga; hoge=3; fuga=4; return hoge==fuga;}'


assert 11 'int main() { int a; a=0;while (a<=5)a=a+1; a = a+5; return a;}'
assert 128 'int main() { int a; a=2;while (a<=100)a=a*2; return a;}'

assert 2 'int main() { int a,b;a=2;if (a<=10) return 2; return 10;}'
assert 10 'int main() { int a,b;a=2;if (a>10) return 2; return 10;}'
assert 2 'int main() { int a,b;a=2;if (a==2) return 2; else return 10;}'
assert 10 'int main() { int a,b;a=2;if (a==10) return 2; else return 10;}'
assert 3 'int main() { int a,b;a=2;if (a-2) return 2; else a=a+1; return a;}'
assert 5 'int main() { int hoge; hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'
# dummy variable
assert 5 'int main() { int a,b,c,d; int hoge; hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'
assert 5 'int main() { int a,b,c; int hoge; hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'
assert 5 'int main() { int a; int hoge; int fuga; hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'
# assert 5 'int main() { int a,b; int hoge; hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'
assert 5 'int main() { int a, hoge,fuga; hoge=10; if (0) hoge = 3; else hoge = 5; return hoge;}'

# TODO: We dont support the (int i=0;) which declares and initializes a var at the same time.
# assert 45 'int main() { int hoge; hoge=0; for (int i=0; i<10; i=i+1) hoge = hoge + i; return hoge;}'
# assert 10 'int main() { int hoge; hoge=0; for (int i=0; i<10; i=i+1) hoge = hoge + 1; return hoge;}'
# assert 12 'int main() { int hoge; hoge=5; for (int i=3; i<10; i=i+1) hoge = hoge + 1; return hoge;}'

assert 6 'int main() { int a; a=2;if (a-2) {return 2;} else {a=a+1; a=a+3;} return a;}'


assert 2 'int main() { int a; a=1; if (a) { return 2; } return 3;}'
assert 3 'int main() { int a; a=0; if (a) { return 2; } return 3;}'
assert 2 'int main() { int a; a=1; if (a) { return 2; } else { return 3; }}'
assert 3 'int main() { int a; a=0; if (a) { return 2; } else { return 3; }}'
assert 2 'int main() { int a; { a=2; } return a;}'
assert 3 'int main() { int a; a=1; { a=a+2; } return a;}'
assert 6 'int main() { int a, b, c; a=1; { b=2; c=3; } return a+b+c;}'

# func
# assert 2 'int func1() { return 2; } int main() { func1(); }'
# assert 3 'int func2(int a) { return 1+a; } int main() { func2(2); }'
# assert 7 'int func3(int a, int b) { return a+b; } int main() { func3(3, 4); }'
# assert 0 'int main() { int a = 1; int b = 2; func(a, b); }'
# assert 0 'int main() { func(1); }'
# 現状の構文だと、関数定義と関数コールを区別できなくない？

assert 3 'a=3; b=&a; return *b;'
assert 5 '
int foo() { return 1; }
int main() { return foo(); }
'

echo OK
