#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cred.h> /* capable, current_uid */
#include <linux/atomic.h>
#include "../include/access_control.h"

#define LOCALDROP_DAEMON_PATH "/usr/sbin/localdropd"
static atomic_t localdrop_open_count = ATOMIC_INIT(0);

static int localdrop_check_exe(void)
{
    struct file *exe;
    struct path daemon_path;
    struct mm_struct *mm;
    int ret;

    /* Look up the expected daemon path */
    ret = kern_path(LOCALDROP_DAEMON_PATH, LOOKUP_FOLLOW, &daemon_path);
    if (ret)
        return ret;

    /* Access exe_file through mm directly — no unexported helper needed */
    mm = current->mm;
    if (!mm)
    {
        path_put(&daemon_path);
        return -EACCES;
    }

    /* Lock mm to safely read exe_file */
    mmap_read_lock(mm);
    exe = mm->exe_file;
    if (!exe)
    {
        mmap_read_unlock(mm);
        path_put(&daemon_path);
        return -EACCES;
    }

    /* Get a reference before releasing the lock */
    get_file(exe);
    mmap_read_unlock(mm);

    /* Compare inodes */
    if (exe->f_path.dentry->d_inode != daemon_path.dentry->d_inode)
        ret = -EPERM;

    fput(exe);
    path_put(&daemon_path);
    return ret;
}

int localdrop_dev_atomic_open(struct inode *inode, struct file *file)
{
    int ret;

    /* Must be root */
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    /* Must be the localdrop daemon binary */
    ret = localdrop_check_exe();
    if (ret)
    {
        pr_warn("localdrop: open attempt from unexpected executable\n");
        return ret;
    }

    /* Only one opener at a time */
    if (atomic_inc_return(&localdrop_open_count) != 1)
    {
        atomic_dec(&localdrop_open_count);
        return -EBUSY;
    }

    pr_info("localdrop: device opened by pid %d\n", current->pid);
    return 0;
}

int localdrop_dev_atomic_release(struct inode *inode, struct file *file)
{
    atomic_set(&localdrop_open_count, 0);
    return 0;
}