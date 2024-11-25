# Lua4文档补充

现在的互联网上已经基本上没有lua4的相关资料的，最详细的就是lua4中的文档了，但其中有些细节不太明朗，这里是针对lua4文档的一些补充

## 概念

+ 关于userdata

    实际上就是指针，类似于epoll中的userdata的设定。可以用它指向一块用户自定义的内存，可以是cpp申请的，又或者是lua_newuserdata()创建的都可以，其目的是作为上下文数据，当回调发生时，回调函数能够拿到这部分上下文数据

+ 关于tag方法

    lua4中没有元表的设定，但是有类似功能的东西，即为tag method
    基本概念类似于分类的标签，通过lua_newtag()创建一个标签，并通过lua_settagmethod()给标签设置方法，再由lua_settag()给table或者userdata设置tag
    这样当对table和userdata进行操作时，即可触发对应的tag method

## 函数说明

+ lua_pushuserdata

    用于push一个现有的指针到lua栈

+ lua_pushcclosure

    push一个lua风格的c函数，可以附带一些额外值，被称之为upvalue，最后一个参数n用来指明栈上往前数n个值都是upvalue，在执行lua_pushcclosure函数的时候，upvalue值都会被退栈
    在被lua调用的c函数中，可以通过`#define lua_upvalueindex(index, n) (index-(n+1))`拿到upvalue值的索引，配合lua_to*系列函数即可取出upvalue值

    > lua_upvalueindex index为想要获取的upvalue索引，n为upvalues的数量
