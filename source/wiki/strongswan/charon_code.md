---
layout: wiki  # 使用wiki布局模板
wiki: StrongSwan # 这是项目名
title: charon解析
order: 2
---

## 带着疑问去看

* `charon`如何组织框架中各个模块？
* `charon`中各模块如何相互交叉联系？
* `charon`核心业务的启动点在什么地方？

## charon源码解析

### main

strongswan-5.9.1\src\charon\charon.c

```c
/**
 * Main function, starts the daemon.
 */
int main(int argc, char *argv[])
{
	...
	/* initialize library */
	if (!library_init(NULL, "charon"))
	{
		library_deinit();
		exit(SS_RC_LIBSTRONGSWAN_INTEGRITY);
	}

	...

	if (!libcharon_init())
	{
		dbg_stderr(DBG_DMN, 1, "initialization failed - aborting charon");
		goto deinit;
	}

	...

	/* initialize daemon */
	if (!charon->initialize(charon,
				lib->settings->get_str(lib->settings, "charon.load", PLUGINS)))
	{
		DBG1(DBG_DMN, "initialization failed - aborting charon");
		goto deinit;
	}
	lib->plugins->status(lib->plugins, LEVEL_CTRL);

	...

	/* start daemon (i.e. the threads in the thread-pool) */
	charon->start(charon);

	/* main thread goes to run loop */
	run();

	...
}

```

#### 初始化全局`library_t *lib`实例

strongswan-5.9.1\src\libstrongswan\library.c

```c
bool library_init(char *settings, const char *namespace)
{
	...

	this->public.hosts = host_resolver_create();
	this->public.proposal = proposal_keywords_create();
	this->public.caps = capabilities_create();
	this->public.crypto = crypto_factory_create();
	this->public.creds = credential_factory_create();
	this->public.credmgr = credential_manager_create();
	this->public.encoding = cred_encoding_create();
	this->public.fetcher = fetcher_manager_create();
	this->public.resolver = resolver_manager_create();
	this->public.db = database_factory_create();
	this->public.processor = processor_create();
	this->public.scheduler = scheduler_create();
	this->public.watcher = watcher_create();
	this->public.streams = stream_manager_create();
	this->public.plugins = plugin_loader_create();

	...
}
```

我们看到`library_init`并不是业务启动点，而是初始化了全局变量`library_t *lib`。该变量用来组织**框架**中的部分模块，包括：处理器（processor）/调度器（scheduler）、证书（creds）。它将这几个模块的指针进行了实例化，以备后用。

其中调度器（scheduler）在初始化过程中创建了整个进程的第一个**job**，并将其加入到处理器（processor）的队列中。注意：此时处理器还没有开始工作（线程尚未创建）。另外，我们观察调度器的执行函数

strongswan-5.9.1\src\libstrongswan\processing\scheduler.c

```c
scheduler_t * scheduler_create()
{
	...

	job = callback_job_create_with_prio((callback_job_cb_t)schedule, this,
										NULL, return_false, JOB_PRIO_CRITICAL);
	lib->processor->queue_job(lib->processor, (job_t*)job);

	return &this->public;
}

```

执行函数入口是 `static job_requeue_t schedule(private_scheduler_t * this)`，该函数就是从它的队列中取**events**，然后交给处理器（Processor）。我们注意到它的返回值始终是`JOB_REQUEUE_DIRECT`。为什么呢？这其实牵扯到charon多线程工作模型：线程一直从队列中取**job**，
根据该**job**的返回值，决定下一步如何处理该**job**，如下，

strongswan-5.9.1\src\libstrongswan\processing\jobs\job.h

* `JOB_REQUEUE_TYPE_NONE`一次性任务，执行完就从队列中移出
* `JOB_REQUEUE_TYPE_FAIR`任务执行失败，把它重新加入到队列尾部，等待下一次执行
* `JOB_REQUEUE_TYPE_DIRECT`无需重新入队，直接循环执行此任务
* `JOB_REQUEUE_TYPE_SCHEDULE`把该任务加入到调度器（Schedule）的队列后（定时任务）

#### 初始化全局`daemon_t *charon`实例

strongswan-5.9.1\src\libcharon\daemon.c

```c
bool libcharon_init()
{
	...
	this = daemon_create();
	...
}
```

```c
private_daemon_t *daemon_create()
{
	...
	this->public.kernel = kernel_interface_create();
	this->public.attributes = attribute_manager_create();
	this->public.controller = controller_create();
	this->public.eap = eap_manager_create();
	this->public.xauth = xauth_manager_create();
	this->public.backends = backend_manager_create();
	this->public.socket = socket_manager_create();
	this->public.traps = trap_manager_create();
	this->public.shunts = shunt_manager_create();
	this->public.redirect = redirect_manager_create();
	this->kernel_handler = kernel_handler_create();

	return this;
}

```

整个**libcharon_init**的过程都是针对全局变量**daemon_t *charon**，初始化了它的各个成员。其中包括框架中的模块：**kernel**、**backends**、**socket**。这里也不是进程的启动点，各个成员被初始化后，等待后面使用。

#### 守护进程charon初始化

```c
	/* initialize daemon */
	if (!charon->initialize(charon,
				lib->settings->get_str(lib->settings, "charon.load", PLUGINS)))
	{
		DBG1(DBG_DMN, "initialization failed - aborting charon");
		goto deinit;
	}
```

`charon->initialize`调用了

strongswan-5.9.1\src\libcharon\daemon.c

```c
METHOD(daemon_t, initialize, bool,
	private_daemon_t *this, char *plugins)
{
	...
		PLUGIN_CALLBACK((plugin_feature_callback_t)sender_receiver_cb, this),
	...
		PLUGIN_CALLBACK((plugin_feature_callback_t)sa_managers_cb, this),
	...
	lib->plugins->add_static_features(lib->plugins, lib->ns, features,
									  countof(features), TRUE, NULL, NULL);

	/* load plugins, further infrastructure may need it */
	if (!lib->plugins->load(lib->plugins, plugins))
	{
		return FALSE;
	}

	/* Queue start_action job */
	lib->processor->queue_job(lib->processor, (job_t*)start_action_job_create());
	...
	return TRUE;
}
```

如上看到，进程**charon**是一个统一插件的形式注册到了全局的插件管理器中，然后通过`lib->plugins->load(lib->plugins, plugins)`随着全部的插件一起进行了初始化。

比较重要的是，**charon**注册了2个插件回调函数（在插件加载过程中会被执行）`sender_receiver_cb`、`sa_managers_cb`，通过这两个回调函数，初始化了框架中的接收器（receiver）和发送器（sender）以及**IKE_SA Manager**和**CHILD_SA**模块。

然后又构建了一个**job**，加入到**处理器**的队列中，等待执行。注意：此时线程仍然没有被创建。

#### 启动守护进程charon

```c
	/* start daemon (i.e. the threads in the thread-pool) */
	charon->start(charon);
```

strongswan-5.9.1\src\libcharon\daemon.c

```c
METHOD(daemon_t, start, void,
	   private_daemon_t *this)
{
	/* start the engine, go multithreaded */
	lib->processor->set_threads(lib->processor,
						lib->settings->get_int(lib->settings, "%s.threads",
											   DEFAULT_THREADS, lib->ns));

	run_scripts(this, "start");
}

```

1. 启动多线程

我们看到**charon->start**函数比较简单，实际只有区区的2个函数调用，但也正是这两个调用，正式开启了守护进程**charon**的魔幻之旅。

`lib->processor->set_threads`根据配置的线程数目创建出多线程，并启动各个线程。这里不再粘贴具体代码，只罗列一下多线程的创建及执行的调用栈：

strongswan-5.9.1\src\libstrongswan\processing\processor.c

```c
set_threads
	thread_create
		process_jobs
			get_job
				process_job
					worker->job->execute(worker->job)
					lib->scheduler->schedule_job(...)
```

2. 执行启动脚本

`run_scripts(this, "start")`的操作比较简单，获取配置文件**strongswan.conf**配置文件中的`start_scripts`配置，并执行其中的脚本。这里主要是方面使用者方便执行一些准备工作。

#### 结尾--主进程开启守护

```c
	/* main thread goes to run loop */
	run();
```

```c
static void run()
{
	...
	while (TRUE)
	{
		...
		switch (sig)
		{
			case SIGHUP:
			{
				DBG1(DBG_DMN, "signal of type SIGHUP received. Reloading "
					 "configuration");
				...
				break;
			}
			case SIGINT:
			case SIGTERM:
			{
				DBG1(DBG_DMN, "%s received, shutting down",
					 sig == SIGINT ? "SIGINT" : "SIGTERM");
				charon->bus->alert(charon->bus, ALERT_SHUTDOWN_SIGNAL, sig);
				return;
			}
		}
	}
}
```

`run()`开启一个死循环，设置几个信号。像忠诚的机器人一样等待使用者的信号召唤（命令行管理工具等），收到召唤（信号）后执行**重新加载配置**或者**退出**的操作。

至此，**charon**就开始工作了。


## 前提疑问解答

### `charon`如何组织框架中各个模块？

我们通过上面分析看到，整个charon的启动过程主要是围绕两个初始化工作进行的，当初始化完成后，直接调用一个**start**就启动了核心功能。而各个模块也是在初始化过程中通过**library_t *lib** 和 **daemon_t *charon**两个全局变量给组织起来，每个模块都实现一组抽象出来的特定功能，然后由核心业务逻辑在执行过程中根据需要去调用。

### `charon`中各模块如何相互交叉联系？

因为各个模块的实例都保存在上面提到的两个全局变量中，根据每个模块的功能不通，在使用时，可以直接通过这两个全局变量创建模块的对象，并使用。至于模块间的关系，这个需要结合业务逻辑来分析。

### `charon`核心业务的启动点在什么地方？

在上面的源码解析中，我们看到在线程创建之前已经至少有2个**job**加入了处理器的队列中，一旦多线程启动，那么这两个**job**就会被立即执行。而我们核心业务的启动点就在这2个**job**中，就是[守护进程charon初始化](#守护进程charon初始化)中初始化的**job**：`start_action_job_create`，我们针对这个**job**来具体分析一下：

strongswan-5.9.1\src\libcharon\processing\jobs\start_action_job.c

```c
start_action_job_t *start_action_job_create(void)
{
	private_start_action_job_t *this;

	INIT(this,
		.public = {
			.job_interface = {
				.execute = _execute,
				.get_priority = _get_priority,
				.destroy = _destroy,
			},
		},
	);
	return &this->public;
}
```

我们看到这个函数创建了一个**job**对象`job_t`，每个**job**都有一个 `execute`成员，也就是它实际执行任务的函数。我们再分析一下这个启动函数

```c
METHOD(job_t, execute, job_requeue_t,
	private_start_action_job_t *this)
{
	...
	enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends,
											NULL, NULL, NULL, NULL, IKE_ANY);
	while (enumerator->enumerate(enumerator, &peer_cfg))
	{
		children = peer_cfg->create_child_cfg_enumerator(peer_cfg);
		while (children->enumerate(children, &child_cfg))
		{
			name = child_cfg->get_name(child_cfg);

			switch (child_cfg->get_start_action(child_cfg))
			{
				case ACTION_RESTART:
					DBG1(DBG_JOB, "start action: initiate '%s'", name);
					charon->controller->initiate(charon->controller,
												 peer_cfg->get_ref(peer_cfg),
												 child_cfg->get_ref(child_cfg),
												 NULL, NULL, 0, FALSE);
					break;
				case ACTION_ROUTE:
					DBG1(DBG_JOB, "start action: route '%s'", name);
					mode = child_cfg->get_mode(child_cfg);
					if (mode == MODE_PASS || mode == MODE_DROP)
					{
						charon->shunts->install(charon->shunts,
												peer_cfg->get_name(peer_cfg),
												child_cfg);
					}
					else
					{
						charon->traps->install(charon->traps, peer_cfg,
											   child_cfg);
					}
					break;
				case ACTION_NONE:
					break;
			}
		}
		children->destroy(children);
	}
	enumerator->destroy(enumerator);
	return JOB_REQUEUE_NONE;
}

```

* `execute`函数首先创建一个ipsec配置的迭代器
* 迭代处理每组配置，根据配置的当前状态执行相应的操作
	* ACTION_RESTART
		* 执行配置初始化 `charon->controller->initiate(...)`
	* ACTION_ROUTE
		* **MODE_PASS**/**MODE_DROP**模式，执行`charon->shunts->install(...)`
		* **MODE_TRANSPORT**/**MODE_TUNNEL**模式，执行`charon->traps->install(...)`

这里也对应了我们再[charon框架综述](index#charon框架综述)中提到的**charon**的工作机制。

## 结束语

StrongSwan的核心**charon**的实现还是很复杂的，通过上面的分析我们可以从整体上理解它的设计框架和核心思想。如果想要驾驭并修改**charon**的功能，那么还要更加深入的从细节上分析。

另外，以面向对象的思想来理解整个代码实现会比较好一些。

更加具体的分析，请看其它文章....




