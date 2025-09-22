#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/anon_inodes.h>
#include <linux/random.h>

#define DEVICE_NAME "ai_accel"
#define CLASS_NAME "ai_accel"

#define MAX_JOBS 0x50
#define MAX_BATCH_SIZE 1024
#define MAX_INPUT_SIZE 0x1000

#define CREATE_JOB   0X1337001
#define RUN_JOB      0X1337002
#define GET_FEEDBACK 0X1337003
#define STOP_JOB     0x1337004

#define READY 1
#define DONE  2

struct ai_job {
  uint32_t accel_id;
  uint32_t model_type;
  uint32_t batch_size;
  uint32_t input_length;
  void* buffer;
  struct task_struct *kthread;
  int32_t status;
} __packed;

struct ai_job_params {
  uint32_t model_type;
  uint32_t batch_size;
  
  uint32_t accel_id;
  void* buffer;
  uint32_t length;
};

static struct ai_job *jobs[MAX_JOBS];

static int major;
static struct class* mhc_class = NULL;
static struct device* mhc_device = NULL;

static long mhc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    long ret = -1;
    char user_buffer[0x500];
    struct ai_job *job = 0;
    struct ai_job_params* params = (struct ai_job_params*)&user_buffer;


    if (copy_from_user(params, (void*)arg, sizeof(struct ai_job_params)))
      goto leave;

    if (params->accel_id >= MAX_JOBS)
        goto leave;

    switch (cmd) {
      case CREATE_JOB: {
        if (jobs[params->accel_id]) {
          printk(KERN_ERR "ai accel: Job is busy\n");
          goto leave;
        }

        if (params->batch_size == 0 || params->batch_size > MAX_BATCH_SIZE)
          goto leave;

        job = (struct ai_job*)kmalloc(sizeof(struct ai_job), GFP_KERNEL_ACCOUNT);
        if (job == 0)
          goto leave;

        job->accel_id = params->accel_id;
        job->model_type = params->model_type;
        job->batch_size = params->batch_size;
        job->kthread = (void*)current;
        job->buffer = NULL;
        job->status = READY;

        jobs[job->accel_id] = job;
        ret = job->accel_id;

        break;
      }
      case RUN_JOB: {
        if (!jobs[params->accel_id]) {
          printk(KERN_ERR "ai accel: Job doesn't exist\n");
          goto leave;
        }

        if (params->length == 0 || params->length > MAX_INPUT_SIZE) {
          printk(KERN_INFO "params->length %d\n", params->length);
          goto leave;
        }

        job = jobs[params->accel_id];
        job->input_length = params->length;

        if (!job->buffer)
          job->buffer = kmalloc(job->input_length, GFP_KERNEL_ACCOUNT);

        if (params->buffer) {
          if (copy_from_user(job->buffer, params->buffer, job->input_length)) {
            kfree(job->buffer);
            goto leave;
          }

          job->status = DONE;
          goto leave;
        }

        // The run job operation is "under development"

        break;
      }
      case GET_FEEDBACK: {
        if (!jobs[params->accel_id]) {
          printk(KERN_ERR "ai accel: Job doesn't exist\n");
          goto leave;
        }

        job = jobs[params->accel_id];

        if (params->length > MAX_INPUT_SIZE) {
          goto leave;
        }

        if (copy_to_user(params->buffer, job->buffer, params->length)) {
          printk(KERN_ERR "ai accel: Copy failed\n");
        }

        break;
      }
      case STOP_JOB: {
        if (!jobs[params->accel_id]) {
          printk(KERN_ERR "ai accel: Job doesn't exist\n");
          goto leave;
        }

        job = jobs[params->accel_id];

        if (job->buffer)
          kfree(job->buffer);
        kfree(job);
        break;
      }
      default:
        printk(KERN_ERR "ai accel: Invalid command\n");
    }

leave:
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = mhc_ioctl,
};


static const struct proc_ops proc_fops = {
    .proc_ioctl = mhc_ioctl,
};


static int __init mhc_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "[mhc] Failed to register char device\n");
        return major;
    }

    mhc_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(mhc_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "[mhc] Failed to create class\n");
        return PTR_ERR(mhc_class);
    }


    mhc_device = device_create(mhc_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mhc_device)) {
        class_destroy(mhc_class);
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "[mhc] Failed to create device\n");
        return PTR_ERR(mhc_device);
    }


    printk(KERN_INFO "[mhc] Module loaded: /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit mhc_exit(void) {
    device_destroy(mhc_class, MKDEV(major, 0));
    class_unregister(mhc_class);
    class_destroy(mhc_class);
    unregister_chrdev(major, DEVICE_NAME);

    printk(KERN_INFO "[mhc] Module unloaded\n");
}

module_init(mhc_init);
module_exit(mhc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hack the Kernel");
MODULE_DESCRIPTION("Hack All the Kernels");
