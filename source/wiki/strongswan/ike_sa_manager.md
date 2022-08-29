---
layout: wiki  # 使用wiki布局模板
wiki: StrongSwan # 这是项目名
title: IKE_SA Manager
order: 4
---


IKE_SA 管理和维护是整个charon的核心，在具体实现上charon有一套独特的机制，下面我们就一起看看。

## IKE_SA Manager

**IKE_SA Manager**作为charon的一个独立模块，对应源文件`ike_sa_manager.c`。在框架介绍中，我们说：__每个IKE_SA必须严格执行**check_out**，同时在IKE_SA被使用后必须严格执行**check_in**，每个SA同一时间只能被一个线程处理。__ 我看了下代码，大概的知道了它的实现机制。

具体的`check_out`和`check_in`动作，是在上章节[复杂的初始化SA](start_point/#复杂的初始化sa)中的调用栈上有指明：

```c
	...
ike_sa_manager.c +1425					haron->ike_sa_manager->checkout_by_config(...)
	...
										charon->ike_sa_manager->checkin(...)
```

通过追踪代码发现，check_out动作的最终都会执行：

```c
	...
	charon->bus->set_sa(charon->bus, ike_sa);
```

check_in动作最终都会执行

```c
	...
	charon->bus->set_sa(charon->bus, NULL);
```

再继续追踪会发现，`check_out`是把**ike_sa**的对象地址赋值给了线程的私有变量**pthread_key**，`check_in`则是把线程私有变量赋值为**NULL**。

通过上面的过程分析，我们有理由相信：

charon通过使用线程私有变量**pthread_key**来保存某个**IKE_SA**对象，实现线程和SA处理的绑定。当某个线程的私有变量不为**NULL**时，说明，该线程正在处理某个SA。如果所有线程的私有变量都不为NULL，那么当新的SA被初始化时就要进行等待，直到某个SA协商完毕被线程**check_in**后线程空闲了，才能处理这个新的SA。

