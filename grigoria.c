/*
* elevator grigora
*
* ideas:
* - reads before writes
* - writes to same lba block should be bundled
*
*/
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

static const unsigned int write_expire = 0.5 * HZ;
static const unsigned int lba_factor = 4; // for 4MB logical block adressing size
static const unsigned int max_bundle_check = 5; //number of write requests, that are checked for bundling
static const unsigned int lba_check = 1; // true/false

struct grigora_data {
		/* read and write fifo lists */
		struct list_head fifo_list[2];

		/* expire time for write requests */
		unsigned int write_expire;
};


static void
grigora_merged_requests(struct request_queue *q, struct request *rq, struct request *next)
{
		list_del_init(&next->queuelist);
}

/* checks if rq is in the logical block of current_rq for bundling request */
static int
check_lb(struct request *current_rq,struct request *next_rq)
{
		unsigned long long int lba_block, start_block, end_block, next_block;

		/* Kernel Sector size: 512 kb */
		/* Logigal Block size: 1Mb * lba_factor */
		/* So for LB-size of 4Mb a lba block has 8196 block sectors */

		/* get current rq block position number */
		lba_block = blk_rq_pos(current_rq)/lba_factor >> 10;
		//printk(KERN_ALERT "lba_block: %u", lba_block);

		/* current lba block start position */
		start_block = lba_block * lba_factor << 10;
		//printk(KERN_ALERT "start block: %u", start_block);
		/* current lba block end position */
		end_block = start_block + 2048 * lba_factor;
		//printk(KERN_ALERT "end block: %u", end_block);

		next_block = blk_rq_pos(next_rq);
		//printk(KERN_ALERT "next: %u", next_block);

		if(start_block <= blk_rq_pos(next_rq) && blk_rq_pos(next_rq) < end_block)
				return 1;

		else
				return 0;
}

/* add requests to dispatch queue */
static int grigora_dispatch(struct request_queue *q, int force)
{
		struct grigora_data *gd = q->elevator->elevator_data;
		const int reads = !list_empty(&gd->fifo_list[READ]);
		const int writes = !list_empty(&gd->fifo_list[WRITE]);

		/* write requests */
		if(writes)
		{
				struct request *current_rq, *next_rq;
				current_rq = list_entry(gd->fifo_list[WRITE].next, struct request, queuelist);

				/* do write request if expired or read queue empty */
				if(time_after_eq(jiffies, current_rq->fifo_time) || list_empty(&gd->fifo_list[READ]))
				{

						list_del_init(&current_rq->queuelist);
						elv_dispatch_sort(q, current_rq);

						/* bundle write requests if convenient */
						if (lba_check)
						{
								struct list_head *pos, *n;
								int i;

								/* iterate safe through write queue */
								for (pos = gd->fifo_list[WRITE].next->next, n = pos->next, i = 0;
								pos != &gd->fifo_list[WRITE] && i < max_bundle_check;
								pos = n, n = pos->next, i++)
								{
										next_rq = list_entry(pos, struct request, queuelist);

										/*  if next_rq is in same lba block as current_rq -> add to dispatch */
										//printk(KERN_ALERT "iterating, i: %i, check_lb: %i \n", i, check_lb(current_rq, next_rq));
										if(check_lb(current_rq, next_rq))
										{
												list_del_init(&next_rq->queuelist);
												elv_dispatch_sort(q, next_rq);
										}
								}
						}
						return 1;
				}
		}

		/* read requests */
		if (reads)
		{
				/* just fifo */
				struct request *rq;
				rq = list_entry(gd->fifo_list[READ].next, struct request, queuelist);
				list_del_init(&rq->queuelist);
				elv_dispatch_sort(q, rq);
				return 1;
		}
		return 0;
}

/*
* add rq to fifo list
*/
static void
grigora_add_request(struct request_queue *q, struct request *rq)
{
		struct grigora_data *gd = q->elevator->elevator_data;

		/* get request type (write or read) */
		const int request_type = rq_data_dir(rq);

		/* read 0, write 1 */
		if(request_type) //if write
		{
				/* set expire time */
				rq->fifo_time = jiffies + gd->write_expire;

				list_add_tail(&rq->queuelist, &gd->fifo_list[WRITE]);
		}
		else
				list_add_tail(&rq->queuelist, &gd->fifo_list[READ]);
}

/* get prev rq */
static struct request *
grigora_former_request(struct request_queue *q, struct request *rq)
{
		struct grigora_data *gd = q->elevator->elevator_data;
		int request_type = rq_data_dir(rq);

		if (rq->queuelist.prev == &gd->fifo_list[request_type])
				return NULL;

		return list_prev_entry(rq, queuelist);
}

/* get next rq */
static struct request *
grigora_latter_request(struct request_queue *q, struct request *rq)
{
		struct grigora_data *gd = q->elevator->elevator_data;
		int request_type = rq_data_dir(rq);

		if (rq->queuelist.next == &gd->fifo_list[request_type])
				return NULL;

		return list_next_entry(rq, queuelist);
}

/* Init elevator private data */
static int grigora_init_queue(struct request_queue *q, struct elevator_type *e)
{
		struct grigora_data *gd;
		struct elevator_queue *eq;

		eq = elevator_alloc(q, e);
		if (!eq)
				return -ENOMEM;

		gd = kmalloc_node(sizeof(*gd), GFP_KERNEL, q->node);
		if (!gd) {
				kobject_put(&eq->kobj);
				return -ENOMEM;
		}
		eq->elevator_data = gd;

		INIT_LIST_HEAD(&gd->fifo_list[READ]);
		INIT_LIST_HEAD(&gd->fifo_list[WRITE]);
		gd->write_expire = write_expire;

		spin_lock_irq(q->queue_lock);
		q->elevator = eq;
		spin_unlock_irq(q->queue_lock);

		return 0;
}

static void grigora_exit_queue(struct elevator_queue *e)
{
		struct grigora_data *gd = e->elevator_data;

		BUG_ON(!list_empty(&gd->fifo_list[READ]));
		BUG_ON(!list_empty(&gd->fifo_list[WRITE]));

		kfree(gd);
}

static struct elevator_type elevator_grigora = {
		.ops = {
			.elevator_merge_req_fn		= grigora_merged_requests,
			.elevator_dispatch_fn			= grigora_dispatch,
			.elevator_add_req_fn			= grigora_add_request,
			.elevator_former_req_fn		= grigora_former_request,
			.elevator_latter_req_fn		= grigora_latter_request,
			.elevator_init_fn					= grigora_init_queue,
			.elevator_exit_fn					= grigora_exit_queue,
		},
		.elevator_name = "grigora",
		.elevator_owner = THIS_MODULE,
};

static int __init grigora_init(void)
{
		return elv_register(&elevator_grigora);
}

static void __exit grigora_exit(void)
{
		elv_unregister(&elevator_grigora);
}

module_init(grigora_init);
module_exit(grigora_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("grigora IO scheduler");
