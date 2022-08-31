---
layout: wiki  # 使用wiki布局模板
wiki: StrongSwan # 这是项目名
title: 关键代码调用栈
order: 6
---

* StrongSwan-5.9.1

## ISAKMP--报文发送P1

```c
//start_action_job.c
METHOD(job_t, execute, job_requeue_t,private_start_action_job_t *this)
charon->controller->initiate(...)
//controller.c +498
	METHOD(controller_t, initiate, status_t,...)
	METHOD(job_t, initiate_execute,...)
	charon->ike_sa_manager->checkout_by_config(...)
//ike_sa_manager.c +1452
	METHOD(ike_sa_manager_t, checkout_by_config,...)
	ike_sa->initiate(...)
//ike_sa.c +1501
		METHOD(ike_sa_t, initiate,...)
		this->task_manager->queue_ike(...)
//task_manager_v1.c +1594
			METHOD(task_manager_t, queue_ike,...)
			this->task_manager->initiate(...)
//task_manager_v1.c +482
				METHOD(task_manager_t, initiate,...)
				task->build(...)
					@main_mode.c
					@isakmp_vendor.c
					@isakmp_cert_post.c
				generate_message(...)
					this->ike_sa->generate_message_fragmented(...)
//ike_sa.c +1258
						METHOD(ike_sa_t, generate_message_fragmented,...)
							generate_message(...)
//ike_sa.c +1219
								METHOD(ike_sa_t, generate_message,...)
								message->generate(...)
//message.c +1861
									METHOD(message_t, generate,...)
									generate_message(...)
										order_payloads(...)
									finalize_message(...)
				send_packets(...)

```

## ISAKMP--报文接收P1

```c
//receiver.c +414
receive_packets(...)
	message_create_from_packet(...)
	lib->processor->queue_job(lib->processor, (job_t*)process_message_job_create(message));
//process_message_job.c +121
		process_message_job_create(...)
			METHOD(job_t, execute,...)
			charon->ike_sa_manager->checkout_by_message(...)
			ike_sa->process_message(...)
//ike_sa.c +1597
			METHOD(ike_sa_t, process_message,...)
				this->task_manager->process_message(...)
//task_manager_v1.c +1361
				METHOD(task_manager_t, process_message, ...)
					process_request(...)
						task->process(...)
						@main_mode.c
						@isakmp_vendor.c
						@isakmp_cert_post.c
						build_response(...)
							task->build(...)
							@main_mode.c
							@isakmp_vendor.c
							@isakmp_cert_post.c
							generate_message(...)
							send_packets(...)
//process_message_job.c +81
			charon->ike_sa_manager->checkin(...)

```