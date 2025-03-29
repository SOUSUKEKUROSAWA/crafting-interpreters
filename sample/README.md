# Sample of Crafting Interpreters

完成版 CLOX，JLOX での動作確認用のディレクトリ

## Setup

```sh
./run.sh
```

## Usage

```sh
$ ./clox
> print "Hello";
Hello

$ ./jlox
> print "Hello";
Hello

$ echo "print \"Hello\" ;" > sample.lox

$ ./clox sample.lox
Hello

$ ./jlox sample.lox
Hello
```

## The Answer of Challenge

```sh
cd craftinginterpreters/note/answers/*
```

## Lox Syntax

```lox
// This is Lox comment
print "# Sample of Lox Program";

print "## Type";

print "### Booleans";
print true;
print false;

print "### Numbers";
print 1234;
print 12.34;

print "### Strings";
print "I am a string";
print "";
print "123";

print "### Nil";

print "## Expressions";

print "### 算術演算";
print 1 + 2;
print 1 - 2;
print 1 * 2;
print 1 / 2;
print 1 / 3;
print 1 / 0;
print -4;
print "Hello" + " World";

print "### 大小比較と等式";
print 2 < 1;
print 2 <= 1;
print 2 > 1;
print 2 >= 1;
print 2 == 1;
print "cat" != "dog";
print "pi" == 3.14;
print "3.14" == 3.14;

print "### 論理演算子";
print !true;
print !false;
print true and false;
print true and true;
print false or false;
print true or false;

print "### 優先順位とグループ化";
print  1 + 2 / 3;
print  (1 + 2) / 3;

print "## Variables";

var imAVariable = "here is my value";
print imAVariable;
var imANil;
print imANil;
imANil = "bagels";
print imANil;

print "## Control Flow...";

var condition = 2 > 1;
if (condition) {
    print "yes";
} else {
    print "no";
}

var a = 1;
while (a < 10) {
    print a;
    a = a + 1;
}

for (var a = 1; a < 10; a = a + 1) {
    print a;
}

print "## Functions";

fun printSum(a, b) {
    print a + b;
}
printSum(1, 2);
print printSum;

var result = printSum(1, 2);
print result;

fun returnSum(a, b) {
    return a + b;
}
print returnSum(1, 2);
print returnSum;

print "### Closure";

fun addPair(a, b) {
    return a + b;
}

fun identity(a) {
    return a;
}

print identity(addPair)(1, 2);

fun outerFunction() {
    fun localFunction() {
        print "I'm local!";
    }

    localFunction();
}

outerFunction();

fun returnFunction() {
    var outside = "outside";

    fun inner() {
        print outside;
    }

    return inner;
}

var fn = returnFunction();
fn(); // outside -> inner 関数が，その外側にある変数を，例え外側の関数がリターンした後でも使えるようにするために，変数への参照を閉じ込めて（closure）いる．

print "## Classes";

class Breakfast {
    cook() {
        print "Eggs a-fryin'!";
    }

    serve(who) {
        print "Enjoy your breakfast, " + who + ".";
    }
}

var breakfast = Breakfast(); // クラス自身がインスタンスを作るファクトリ関数になっている．
print breakfast;
breakfast.cook();
breakfast.serve("Alice");

print "### 実体化と初期化";

class Lunch {
    serve(who) {
        print "Enjoy your " + this.meat + " and " + this.bread + ", " + who + ".";
    }
}

var lunch = Lunch();
lunch.time = "noon";
print lunch.time;
lunch.meat = "ham";
lunch.bread = "wheat";
lunch.serve("Alice");

class Dinner {
    init(meat, bread) {
        this.meat = meat;
        this.bread = bread;
    }

    serve(who) {
        print "Enjoy your " + this.meat + " and " + this.bread + ", " + who + ".";
    }
}

var dinner = Dinner("beef", "rye");
dinner.serve("Bob");

print "### 継承";

class Brunch < Dinner {
    init(meat, bread, drink) {
        super.init(meat, bread);
        this.drink = drink;
    }

    recommend() {
        print "How about a " + this.drink + "?";
    }
}

var brunch = Brunch("sausage", "sourdough", "coffee");
brunch.recommend();
brunch.serve("Alice");

print clock(); // Benchmark
```
