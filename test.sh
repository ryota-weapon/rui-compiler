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

assert 2 '2;'
assert 3 'return 3;'
assert 5 'a=1;b=4;return a+b;'
assert 15 'a=3;b=4;return a*b+3;'

assert 2 'a=2; a;'
assert 3 'a=1; a=a+2; a;'
assert 6 'a=1; b=2; c=3; return a+b+c;'
assert 11 'a=3; b=4; a+b*2;'
assert 14 'a=2; b=3; return (a+b)*2+4;'
assert 3 'a=1; b=a+2; return b;'
assert 7 'a=3; b=4; a=a+b; return a;'
assert 4 'a=1; b=2; a=b=4; return a;'
assert 4 'a=1; b=2; a=b=4; return b;'
assert 1 'a=3; b=4; return a<b;'
assert 1 'a=3; b=3; return a==b;'
assert 0 'a=3; b=4; return a==b;'

echo OK
