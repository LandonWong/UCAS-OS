#include <mailbox.h>
#include <string.h>
#define MAX_NUM 256

mailbox_t mbox_open(char *name)
{
   /* int i = 0;
    for (i = 0;i < MAX_NUM; i++)
    {
        if((mbox_list[i] != NULL) && (!strcmp(name, mbox_list[i]->name))){
            mbox_list[i]->cited++;
            return mbox_list[i]->id;
        }    
    }
    mbox_t *new = (mbox_t *)kmalloc(sizeof(mbox_t));
    new->id = get_id_mbox();
    mbox_list[new->id] = new;
    strcpy(mbox_list[new->id]->name, name);
    mbox_list[new->id]->cited++;
    mbox_list[new->id]->used_size = mbox_list[new->id]->head = mbox_list[new->id]->tail = 0;
    mthread_cond_init(&mbox_list[new->id]->empty);
    mthread_cond_init(&mbox_list[new->id]->full);
    mthread_mutex_init(&mbox_list[new->id]->lock);
    return new->id;*/
    return sys_mbox_open(name);
}

void mbox_close(mailbox_t mailbox)
{
    /*mbox_list[mailbox] = NULL;*/
    sys_mbox_close(mailbox);
}

void mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    sys_mbox_send(mailbox,msg,msg_length);
    /*mbox_t *box = mbox_list[mailbox];
    mthread_mutex_lock(&box->lock);
    while(MSG_MAX_SIZE - box->used_size < msg_length)
        mthread_cond_wait(&box->empty, &box->lock);
    if(MSG_MAX_SIZE - box->tail < msg_length)
    {
        memcpy((char *)(box->buffer + box->tail), (char *)msg, MSG_MAX_SIZE - box->tail);
        box->tail = msg_length - (MSG_MAX_SIZE - box->tail);
        memcpy((char *)box->buffer, (char *)(msg + msg_length - box->tail), box->tail);
    }
    else
    {
        memcpy((char *)(box->buffer + box->tail), (char *)msg, msg_length);   
        box->tail += msg_length;
    }
    box->used_size += msg_length;
    mthread_cond_broadcast(&box->full);
    mthread_mutex_unlock(&box->lock);*/
}

void mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    sys_mbox_recv(mailbox,msg,msg_length);
    /*mbox_t *box = mbox_list[mailbox];
    mthread_mutex_lock(&box->lock);
    while(box->used_size < msg_length)
        mthread_cond_wait(&box->full, &box->lock);
    if(MSG_MAX_SIZE - box->head < msg_length)
    {
        memcpy((char *)msg, (char *)(box->buffer + box->head), MSG_MAX_SIZE - box->head);
        box->head = msg_length - (MSG_MAX_SIZE - box->head);
        memcpy((char *)(msg + msg_length - box->head), (char *)box->buffer, box->head);
    }
    else
    {
        memcpy((char *)msg, (char *)(box->buffer + box->head), msg_length);
        box->head += msg_length;
    }
    box->used_size -= msg_length;
    mthread_cond_broadcast(&box->empty);
    mthread_mutex_unlock(&box->lock);*/
}
