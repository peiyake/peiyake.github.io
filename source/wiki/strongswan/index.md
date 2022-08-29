---
layout: wiki  # 使用wiki布局模板
wiki: StrongSwan # 这是项目名
title: 核心进程charon
order: 1
---


## 介绍

StrongSwan支持在网络中创建基于IKEv1/IKEv2协议的IPSec隧道。其核心功能由守护进程`charon`实现。

* 版本: StrongSwan-5.9.1

## charon框架

[官方介绍](https://docs.strongswan.org/docs/5.9/daemons/charon.html)

>The charon daemon was built from scratch to implement the IKEv2 protocol for the strongSwan project. Most of its code is located in the libcharon library making the IKE daemon core available to other programs such as charon-systemd, charon-svc, charon-cmd or the Android app.

### charon核心框架

```
      +---------------------------------+       +----------------------------+
      |          Credentials            |       |          Backends          |
      +---------------------------------+       +----------------------------+

       +------------+    +-----------+        +------+            +----------+
       |  receiver  |    |           |        |      |  +------+  | CHILD_SA |
       +----+-------+    | Scheduler |        | IKE- |  | IKE- |--+----------+
            |            |           |        | SA   |--| SA   |  | CHILD_SA |
    +-------+--+         +-----------+        |      |  +------+  +----------+
 <->|  socket  |               |              | Man- |
    +-------+--+         +-----------+        | ager |  +------+  +----------+
            |            |           |        |      |  | IKE- |--| CHILD_SA |
       +----+-------+    | Processor |--------|      |--| SA   |  +----------+
       |   sender   |    |           |        |      |  +------+
       +------------+    +-----------+        +------+

      +---------------------------------+       +----------------------------+
      |               Bus               |       |      Kernel Interface      |
      +---------------------------------+       +----------------------------+
             |                    |                           |
      +-------------+     +-------------+                     V
      | File-Logger |     |  Sys-Logger |                  //////
      +-------------+     +-------------+
```

### 各个模块功能

1. **Processor**

处理器（Processor）创建了一个线程池，线程数量可以根据配置修改。其它模块通过将**job**加入到**Processor**的方式来异步执行业务功能。

The threading is realized with the help of a thread pool (called processor) which contains a fixed amount of precreated threads. All threads in the daemon originate from the processor. To delegate work to a thread, jobs are queued to the processor for asynchronous execution

2. **Scheduler**

调度器（Schdeuler）。调度器用来执行一些定时任务，例如：rekeying。调度器本身并不执行**job**，而是通过把**job**加入到**Processor**的队列中由线程执行。

The scheduler is responsible to execute timed events. Jobs may be queued to the scheduler to get executed at a defined time (e.g. rekeying). The scheduler does not execute the jobs itself, it queues them to the processor

3. **IKE_SA Manager**

IKE_SA管理员（IKE_SA Manager）。其管理所有的IKE_SAs。它进一步同步执行：每个IKE_SA必须严格执行**check_out**，同时在IKE_SA被使用后必须严格执行**check_in**。管理员确保单个IKE_SA只被一个线程处理。这将使我们可以简单化的操作负载的IKE_SA.

The IKE_SA manager manages all IKE_SAs. It further handles the synchronization: Each IKE_SA must be checked out strictly and checked in again after use. The manager guarantees that only one thread may check out a single IKE_SA. This allows us to write the (complex) IKE_SAs routines as non-threadsave

4. **IKE_SA**

熟悉IPSec的都知道，在建立IPSec隧道前要首先协商出两个SA用来交换秘钥数据，指的就是这里的 **IKE_SA**。每个**IKE_SA**包含它的状态、逻辑处理已经相关数据信息。在charon中，IPSec隧道的每个SA对应一个 **IKE_SA**对象。

The IKE_SA contain the state and the logic of each IKE_SA and handle the messages

5. **CHILD_SA**

我的理解是：**IKE_SA**是IPSec隧道的前提，它用来确保端-端数据交换的安全性；**CHILD_SA**则对应隧道建立后，在隧道内传输数据的合法性。（例如：
我们建立了隧道，但是我们可能只允许指定的子网在隧道内进行数据传输，我们称之为兴趣流）。每个隧道可以管理多对兴趣流（每个**IKE_SA**可以包含多个
**CHILD_SA**）。 兴趣流的参数往往需要告知内核（**CHILD_SA**需要和内核交互，通过**kernel interface**）

The CHILD_SA contains state about an IPsec security association and manages them. An IKE_SA may have multiple CHILD_SAs. Communication to the kernel takes place here through the kernel interface

6. **Kernel Interface**

内核接口来安装IPSec的SA、Policy、Route、VIP等。它提供一组方法来访问接口，同时底层状态变化时向守护进程发出通知。

The kernel interface installs IPsec security associations, policies, routes and virtual addresses. It further provides methods to enumerate interfaces and may notify the daemon about state changes at lower layers

7. **Bus**

总线（Bus）接收来自不同线程的信息，并把它们转发给对此信号感兴趣的监听者。

The bus receives signals from the different threads and relays them to interested listeners. Debugging signals, but also important state changes or error messages are sent over the bus

8. **Controller**

控制器（Controller）提供了简单的API，供各种插件访问和控制守护进程（例如：初始化IKE_SA等）

The controller provides a simple API for plugins to control the daemon (e.g. initiate IKE_SA, close IKE_SA, …​)

9. **Backends**

后端（Backends）是一个提供配置信息的可插拔模块。它必须提供有用守护进程用来获取配置的接口。

Backends are pluggable modules which provide configuration. They have to implement an API which the daemon core uses to get configuration

10. Credentials

证书（Credentials）提供证书相关的服务。

Provides trustchain verification and credential serving using registered plugins

## charon框架综述

了解了上面各个模块的功能，那么大概可以理解**charon**的工作机制：

**IKE_SA Manager**从后端**Backends**获取配置信息，并根据配置信息初始化**IKE_SA**。**IKE_SA**的初始化包括：构建IKE_SA实例，交付给**处理器（Processor）**、**调度器（Scheduler）**，根据IKE协议执行报文构建**发送（sender）**或者**接收（receiver）**报文并解析。**IKE_SA**协商成功后，创建对应的**CHILD_SA**，同时调用**内核接口（Kernel Interface）**将相关信息下发到内核，隧道建立成功后，根据配置转入状态监控（**Rekey**等）。以上所有过程对各个阶段的事件交付**总线（Bus）**，由总线确定事件的流向。

总之，**charon**的核心任务是管理IPSec的**SA**，所以业务逻辑核心是**IKE_SA Manager**，其它的模块都是为其服务的。

