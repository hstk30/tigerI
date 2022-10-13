# 基本块和轨迹

> The Chomskyan view is that this ability is quite special, unique to humans. It is, in essence, what distinguishes us from other animals - the source of complex thought and communication, and thus complex mathematical, artistic and linguistic ability.


[Noam Chomsky](https://en.wikipedia.org/wiki/Noam_Chomsky) 的观点认为， 
**递归（recursion）** 这种能力非常特殊，是人类所独有的。
从本质上讲，它是我们与其他动物的区别——复杂思维和交流的来源，以及复杂的数学、艺术和语言能力。


这本书几乎所有章节的代码都是 **递归** **递归** **再递归**。
将小的东西组合成一个大的东西，将大的东西分解为小的东西进行处理。
在我工作上的业务代码上是不可能遇到的，写个 **递归** 会被认为是晦涩的代码。


`canon.h .c` 的代码没有细看，说实话`reorder` 有点麻烦。
让我有点对`Eseq` 有点怀疑：如果后面处理它这么麻烦，我完全可以在中间代码里不使用它。


## 目前的成果

```
let
    function fib(t: int): int = 
        if (t < 2)
        then t
        else fib(t - 1) + fib(t - 2)
in
    fib(5)
end
```

翻译为中间代码，再 **规范化** 后已经初具模样了：

```
LABEL fib
MOVE( TEMP t102, CONST 1)
CJUMP(LT, MEM( TEMP t100), CONST 2, L0,L1)
LABEL L1
MOVE( TEMP t102, CONST 0)
LABEL L0
CJUMP(NE, TEMP t102, CONST 0, L2,L3)
LABEL L2
MOVE( TEMP t103, MEM( TEMP t100))
JUMP( NAME L4)
LABEL L3
MOVE( TEMP t105, CALL( NAME fib, TEMP t101, BINOP(MINUS, MEM( TEMP t100), CONST 1)))
MOVE( TEMP t107, TEMP t105)
MOVE( TEMP t106, CALL( NAME fib, TEMP t101, BINOP(MINUS, MEM( TEMP t100), CONST 2)))
MOVE( TEMP t103, BINOP(PLUS, TEMP t107, TEMP t106))
LABEL L4
MOVE( TEMP t104, TEMP t103)

LABEL tiger_main
MOVE( TEMP t104, CALL( NAME fib, TEMP t101, CONST 5))
```

栈帧信息比较简单，形式参数中的第一个参数为静态链参数：

```
Frame name: fib
	formals: 
		InFrame(0)
		InReg
	locals: 
Frame size: 8

Frame name: tiger_main
	formals: 
		InFrame(0)
	locals: 
Frame size: 8

Frame name: tiger_global
	formals: 
		InFrame(0)
	locals: 
Frame size: 8
```

不错的成果🎉


## 可见的改进

- 判定语句是否可以交换，可以加入更多规则，从而加速`Eseq` 的重写。
    我对性能、时间没有具体的感觉，不过这只是系统里的很小部分，
    改进了估计不会有很大提升，有空可以试试

