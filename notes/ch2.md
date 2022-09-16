## 词法分析

### 处理字符串和注释

#### 方案一: 正则，集中处理整个匹配到的数据

一个正则强上，按这种模式构造一个

```
{START} ( {SIMPLE} | {COMPLEX}  ) * {END}
```

形式的正则，不过可能不好处理转义字符，考虑的东西多了正则会非常复杂，后期加新东西会变的很麻烦。
匹配到字符串后走一个`read_string(char *s)` 的函数集中处理字符串。


#### 方案二: Start Condition

设置开始条件(Start Condition)，如果`字符串` 的转义规则很简单的话，这种方法比较简单，
应该也是`lex` 推荐的，但如果规则复杂，则每个规则都只处理一个或几个字符，
会有很多规则，甚至不止有个状态。


#### 方案三: 手写词法分析

如果转义规则比较复杂，手写其实这反而是最简单的，可以减少学习`flex` 的时间，
性能也可以自己控制。


#### 我的方案

我没有使用Start Condition，直接在`Action` 里处理了字符串和注释，
所以才会感觉比如手写词法分析。当然，这样代码的缩进有的爆炸。


#### 一些问题或bug

- 报错的时候`token` 的位置可能会有1、2 位的偏差
- `End-Of-File` 的时候，未结束的字符串和注释可能会报错，
    因为`flex` 不像不能在`Action` 中正确的处理`End-Of-File` 
    [Can't handle EOF in Action, or how to?](https://github.com/westes/flex/issues/540)


### 参考

- [Start Conditions](https://westes.github.io/flex/manual/Start-Conditions.html)
- [Recognising strings and/or comments](https://www.cs.man.ac.uk/~pjj/cs212/ex2_str_comm.html)
- [The Fundamentals of lex Rules](https://docs.oracle.com/cd/E19504-01/802-5880/6i9k05dgk/index.html#lex-36741)
- [regular-expression-for-a-string-literal-in-flex-lex](https://stackoverflow.com/questions/2039795/regular-expression-for-a-string-literal-in-flex-lex)


