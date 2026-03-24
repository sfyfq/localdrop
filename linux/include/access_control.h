#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H
int localdrop_dev_atomic_release(struct inode *inode, struct file *file);
int localdrop_dev_atomic_open(struct inode *inode, struct file *file);
#endif