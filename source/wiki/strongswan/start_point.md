---
layout: wiki  # 使用wiki布局模板
wiki: StrongSwan # 这是项目名
title: charon启动过程分析
order: 3
---

根据上个章节[charon解析](charon_code#charon核心业务的启动点在什么地方)中的分析，我们知道整个核心进程charon的启动点在`start_action_job_create`，那么本节我们就来具体展开分析一下这个启动过程。

## charon业务启动流程

我们首先把启动过程的函数调用栈粘贴出来

```c
charon.c 				main
							charon->initialize(...);
daemon.c							METHOD(daemon_t, initialize, bool,private_daemon_t *this, char *plugins)
start_action_job.c						start_action_job_create
											METHOD(job_t, execute, job_requeue_t,private_start_action_job_t *this)
```

我们看到整个启动过程最终落在了**start_action_job.c**的`execute`函数，下面就分析一下这个函数做了什么事情：

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

我们逐行分析

### 魔幻迭代器

```
	enumerator = charon->backends->create_peer_cfg_enumerator(charon->backends,
```

起初，看到`enumerator`这个东西的时候整个人都蒙了，满脑子疑问：这是什么东西？它在干啥？为什么这样写？为什么几乎每个结构体都有这个东西？啊啊啊啊啊~~

但是看的多了我发现，这个东西的实现细节压根勿需关注，我们只要知道它用来做什么就行了，举个简单的例子：

假设有这样一个逗号分隔字符串

`aes,3des,sha`

那么这样一个字符串在被使用`enumerator`的地方处理后，我们可以通过类似上面`while (enumerator->enumerate(enumerator, &peer_cfg))`的形式，迭代处理每个单独的字符串:`aes`、`3des`、`sha`。就是这么简单，这么魔幻。

同样道理，我们这里的这个用法是针对 `peer_cfg`创建了一个迭代器。（在[charon框架-各模块功能](index#各个模块功能)中我们介绍过，**backends**是一个可插拔的配置管理模块）这里的`peer_cfg`就是backends管理的配置。那么，新的疑问来了，backends里面的配置在什么时候创建的呢？这里简单说一下：charon在各种系统中有不通的启动方法，参见[charon服务](https://docs.strongswan.org/docs/5.9/daemons/charon-systemd.html)，还有一种已经被新版本弃用（但仍然支持）的方法：通过starter进程。

启动进程和charon大概是这样一个关系：

```
	starter	------------->|							+++++++++++++++++++++
						  |							+		charon		+
	charon-systemd ------>|		/-[stroke]->\		+		   |		+
						  |---->------------->[backends] <---->|		+
	charon-svc ---------->|		\-[vici]->/			+					+
						  |							+					+
	charon-cmd----------->|							+++++++++++++++++++++
```

上面的**stroke**和**vici**是配置管理的两个插件，启动进程可以使用插件的接口向charon发送配置。其中**vici**是当前官方新的推荐使用的，**stroke**插件已经不推荐使用。

### 流水线

```c
switch (child_cfg->get_start_action(child_cfg))
```

每个**child_cfg**都被维护2个状态**ACTION_RESTART**和**ACTION_ROUTE**，像流水线一样，每个配置的隧道都要经过这两步的处理才行。

### 复杂的初始化SA

```c
				case ACTION_RESTART:
					DBG1(DBG_JOB, "start action: initiate '%s'", name);
					charon->controller->initiate(charon->controller,
												 peer_cfg->get_ref(peer_cfg),
												 child_cfg->get_ref(child_cfg),
												 NULL, NULL, 0, FALSE);
```

在这里我们又看到了charon的另一个模块**controller**，该模块的功能时提供各种API，那么我们就看看这里调用的`initiate`干了什么事情。

调用栈如下：

```c
controller.c +498			charon->controller->initiate(...)
								METHOD(controller_t, initiate, status_t,...)
									METHOD(job_t, initiate_execute, job_requeue_t,...)
ike_sa_manager.c +1425					haron->ike_sa_manager->checkout_by_config(...)
											ike_sa->initiate(ike_sa, listener->child_cfg, 0, NULL, NULL)
ike_sa.c +1501									METHOD(ike_sa_t, initiate, status_t,...)
													task_manager->initiate(this->task_manager)
task_manager_v1.c +482									METHOD(task_manager_t, initiate, status_t,private_task_manager_t *this)
										charon->ike_sa_manager->checkin(...)
```

最终程序执行都落在了 task_manager_v1.c 的`initiate`函数上，在这里完成了IKEv1协议的的协商过程...




