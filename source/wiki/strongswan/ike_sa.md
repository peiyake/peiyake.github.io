---
layout: wiki  # 使用wiki布局模板
wiki: StrongSwan # 这是项目名
title: IKE_SA
order: 5
---

**IKE_SA**是charon依据IKE协议对SA管理的最小单元，每个**IKE_SA**对应一个配置的隧道，该隧道所有相关的东西都在数据结构**IKE_SA**中进行管理。---- `ike_sa.c`

## IKE_SA初始化

每个**IKE_SA**在[复杂的SA初始化](start_point/#复杂的初始化sa)被初始化，最终程序落在 `IKE_SA`-> `task_manager->initiate(..)`

* IKEv1:`task_manager_v1.c`
* IKEv2:`task_manager_v2.c`

## IKE_SA的TASKs

以IKEv1为例，我们知道V1的主模式下，IKE的协商需要6个报文交互，每个报文都发送或处理IKE协议的特定信息。在**IKE_SA**中，它把IKE协议的处理过程分为一个一个的小的TASK：比如构建SA报文、构建证书报文等等。这些TASK被放在该**IKE_SA**的TASK_QUEUE中，根据不同阶段来激活不同的TASK，当该阶段所有TASK都完成后（报文构建、解析）进行下一步处理。

### TASK入队

strongswan-5.9.1\src\libcharon\sa\ikev1\task_manager_v1.c +1593

```c
METHOD(task_manager_t, queue_ike, void,
	private_task_manager_t *this)
{
	peer_cfg_t *peer_cfg;

	if (!has_queued(this, TASK_ISAKMP_VENDOR))
	{
		queue_task(this, (task_t*)isakmp_vendor_create(this->ike_sa, TRUE));
	}
	if (!has_queued(this, TASK_ISAKMP_CERT_PRE))
	{
		queue_task(this, (task_t*)isakmp_cert_pre_create(this->ike_sa, TRUE));
	}
	peer_cfg = this->ike_sa->get_peer_cfg(this->ike_sa);
	if (peer_cfg->use_aggressive(peer_cfg))
	{
		if (!has_queued(this, TASK_AGGRESSIVE_MODE))
		{
			queue_task(this, (task_t*)aggressive_mode_create(this->ike_sa, TRUE));
		}
	}
	else
	{
		if (!has_queued(this, TASK_MAIN_MODE))
		{
			queue_task(this, (task_t*)main_mode_create(this->ike_sa, TRUE));
		}
	}
	if (!has_queued(this, TASK_ISAKMP_CERT_POST))
	{
		queue_task(this, (task_t*)isakmp_cert_post_create(this->ike_sa, TRUE));
	}
	if (!has_queued(this, TASK_ISAKMP_NATD))
	{
		queue_task(this, (task_t*)isakmp_natd_create(this->ike_sa, TRUE));
	}
}
```

### IKE_SA状态机

strongswan-5.9.1\src\libcharon\sa\ike_sa.h +268

```c
/**
 * State of an IKE_SA.
 *
 * An IKE_SA passes various states in its lifetime. A newly created
 * SA is in the state CREATED.
 * @verbatim
                 +----------------+
                 ¦   SA_CREATED   ¦
                 +----------------+
                         ¦
    on initiate()--->    ¦   <----- on IKE_SA_INIT received
                         V
                 +----------------+
                 ¦ SA_CONNECTING  ¦
                 +----------------+
                         ¦
                         ¦   <----- on IKE_AUTH successfully completed
                         V
                 +----------------+
                 ¦ SA_ESTABLISHED ¦-------------------------+ <-- on rekeying
                 +----------------+                         ¦
                         ¦                                  V
    on delete()--->      ¦   <----- on IKE_SA        +-------------+
                         ¦          delete request   ¦ SA_REKEYING ¦
                         ¦          received         +-------------+
                         V                                  ¦
                 +----------------+                         ¦
                 ¦  SA_DELETING   ¦<------------------------+ <-- after rekeying
                 +----------------+
                         ¦
                         ¦   <----- after delete() acknowledged
                         ¦
                        \V/
                         X
                        / \
   @endverbatim
 */
```

## IKE_SA代码分析


主要是4个过程，已经在代码中注释：

1. IKE_SA 状态机转换
2. 激活的TASK迭代处理
3. 构建报文并发送
4. 进入状态机下一个阶段

strongswan-5.9.1\src\libcharon\sa\ikev1\task_manager_v1.c +482

```c
METHOD(task_manager_t, initiate, status_t,
	private_task_manager_t *this)
{
	enumerator_t *enumerator;
	task_t *task;
	message_t *message;
	host_t *me, *other;
	exchange_type_t exchange = EXCHANGE_TYPE_UNDEFINED;
	bool new_mid = FALSE, expect_response = FALSE, cancelled = FALSE, keep = FALSE;

	if (this->initiating.type != EXCHANGE_TYPE_UNDEFINED &&
		this->initiating.type != INFORMATIONAL_V1)
	{
		DBG2(DBG_IKE, "delaying task initiation, %N exchange in progress",
				exchange_type_names, this->initiating.type);
		/* do not initiate if we already have a message in the air */
		return SUCCESS;
	}

	if (this->active_tasks->get_count(this->active_tasks) == 0)
	{
		// IKE_SA 状态机转换
		DBG2(DBG_IKE, "activating new tasks");
		switch (this->ike_sa->get_state(this->ike_sa))
		{
			case IKE_CREATED:
				activate_task(this, TASK_ISAKMP_VENDOR);
				activate_task(this, TASK_ISAKMP_CERT_PRE);
				if (activate_task(this, TASK_MAIN_MODE))
				{
					exchange = ID_PROT;
				}
				else if (activate_task(this, TASK_AGGRESSIVE_MODE))
				{
					exchange = AGGRESSIVE;
				}
				activate_task(this, TASK_ISAKMP_CERT_POST);
				activate_task(this, TASK_ISAKMP_NATD);
				break;
			case IKE_CONNECTING:
				if (activate_task(this, TASK_ISAKMP_DELETE))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_XAUTH))
				{
					exchange = TRANSACTION;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_INFORMATIONAL))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				break;
			case IKE_ESTABLISHED:
				if (activate_task(this, TASK_MODE_CONFIG))
				{
					exchange = TRANSACTION;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_QUICK_DELETE))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_ISAKMP_DELETE))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_ISAKMP_DPD))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				if (!mode_config_expected(this) &&
					activate_task(this, TASK_QUICK_MODE))
				{
					exchange = QUICK_MODE;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_INFORMATIONAL))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				break;
			case IKE_REKEYING:
				if (activate_task(this, TASK_ISAKMP_DELETE))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				if (activate_task(this, TASK_ISAKMP_DPD))
				{
					exchange = INFORMATIONAL_V1;
					new_mid = TRUE;
					break;
				}
				break;
			default:
				break;
		}
	}
	else
	{
		DBG2(DBG_IKE, "reinitiating already active tasks");
		enumerator = this->active_tasks->create_enumerator(this->active_tasks);
		while (enumerator->enumerate(enumerator, (void**)&task))
		{
			DBG2(DBG_IKE, "  %N task", task_type_names, task->get_type(task));
			switch (task->get_type(task))
			{
				case TASK_MAIN_MODE:
					exchange = ID_PROT;
					break;
				case TASK_AGGRESSIVE_MODE:
					exchange = AGGRESSIVE;
					break;
				case TASK_QUICK_MODE:
					exchange = QUICK_MODE;
					break;
				case TASK_XAUTH:
					exchange = TRANSACTION;
					new_mid = TRUE;
					break;
				default:
					continue;
			}
			break;
		}
		enumerator->destroy(enumerator);
	}

	if (exchange == EXCHANGE_TYPE_UNDEFINED)
	{
		DBG2(DBG_IKE, "nothing to initiate");
		/* nothing to do yet... */
		return SUCCESS;
	}

	me = this->ike_sa->get_my_host(this->ike_sa);
	other = this->ike_sa->get_other_host(this->ike_sa);

	if (new_mid)
	{
		if (!this->rng->get_bytes(this->rng, sizeof(this->initiating.mid),
								 (void*)&this->initiating.mid))
		{
			DBG1(DBG_IKE, "failed to allocate message ID, destroying IKE_SA");
			flush(this);
			return DESTROY_ME;
		}
	}
	message = message_create(IKEV1_MAJOR_VERSION, IKEV1_MINOR_VERSION);
	message->set_message_id(message, this->initiating.mid);
	message->set_source(message, me->clone(me));
	message->set_destination(message, other->clone(other));
	message->set_exchange_type(message, exchange);
	this->initiating.type = exchange;
	this->initiating.retransmitted = 0;

	// IKE_SA 激活的TASK迭代处理
	enumerator = this->active_tasks->create_enumerator(this->active_tasks);
	while (enumerator->enumerate(enumerator, (void*)&task))
	{
		switch (task->build(task, message))
		{
			case SUCCESS:
				/* task completed, remove it */
				this->active_tasks->remove_at(this->active_tasks, enumerator);
				if (task->get_type(task) == TASK_AGGRESSIVE_MODE ||
					task->get_type(task) == TASK_QUICK_MODE)
				{	/* last message of three message exchange */
					keep = TRUE;
				}
				task->destroy(task);
				continue;
			case NEED_MORE:
				expect_response = TRUE;
				/* processed, but task needs another exchange */
				continue;
			case ALREADY_DONE:
				cancelled = TRUE;
				break;
			case FAILED:
			default:
				if (this->ike_sa->get_state(this->ike_sa) != IKE_CONNECTING)
				{
					charon->bus->ike_updown(charon->bus, this->ike_sa, FALSE);
				}
				/* FALL */
			case DESTROY_ME:
				/* critical failure, destroy IKE_SA */
				enumerator->destroy(enumerator);
				message->destroy(message);
				flush(this);
				return DESTROY_ME;
		}
		break;
	}
	enumerator->destroy(enumerator);

	if (this->active_tasks->get_count(this->active_tasks) == 0 &&
		(exchange == QUICK_MODE || exchange == AGGRESSIVE))
	{	/* tasks completed, no exchange active anymore */
		this->initiating.type = EXCHANGE_TYPE_UNDEFINED;
	}
	if (cancelled)
	{
		message->destroy(message);
		return initiate(this);
	}

	// 构建报文
	clear_packets(this->initiating.packets);
	if (!generate_message(this, message, &this->initiating.packets))
	{
		/* message generation failed. There is nothing more to do than to
		 * close the SA */
		message->destroy(message);
		flush(this);
		charon->bus->ike_updown(charon->bus, this->ike_sa, FALSE);
		return DESTROY_ME;
	}

	this->initiating.seqnr++;
	if (expect_response)
	{
		message->destroy(message);
		return retransmit(this, this->initiating.seqnr);
	}

	// 发送报文
	send_packets(this, this->initiating.packets);
	if (!keep)
	{
		clear_packets(this->initiating.packets);
	}
	message->destroy(message);

	if (exchange == INFORMATIONAL_V1)
	{
		switch (this->ike_sa->get_state(this->ike_sa))
		{
			case IKE_CONNECTING:
				/* close after sending an INFORMATIONAL when unestablished */
				charon->bus->ike_updown(charon->bus, this->ike_sa, FALSE);
				return FAILED;
			case IKE_DELETING:
				/* close after sending a DELETE */
				return DESTROY_ME;
			default:
				break;
		}
	}

	// 进入状态机下一个阶段
	return initiate(this);
}
```


