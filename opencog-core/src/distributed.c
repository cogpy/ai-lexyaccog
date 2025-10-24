/*
 * OpenCog Distributed OS Primitives
 * Inter-process communication and coordination
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include "../include/distributed.h"

/* Heartbeat interval in milliseconds */
#define HEARTBEAT_INTERVAL_MS 1000
#define NODE_TIMEOUT_MS 5000

/* Heartbeat thread function */
static void* heartbeat_thread_func(void* arg) {
    distributed_ctx_t* ctx = (distributed_ctx_t*)arg;
    
    while (ctx->running) {
        /* Create heartbeat message */
        message_t msg;
        msg.type = MSG_TYPE_HEARTBEAT;
        msg.source_node = ctx->this_node_id;
        msg.dest_node = 0; /* Broadcast */
        
        struct timeval tv;
        gettimeofday(&tv, NULL);
        msg.timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        msg.payload_size = 0;
        msg.payload = NULL;
        
        /* Send heartbeat */
        distributed_send_message(ctx, &msg);
        
        /* Sleep */
        usleep(HEARTBEAT_INTERVAL_MS * 1000);
    }
    
    return NULL;
}

/* Message handler thread function */
static void* message_handler_thread_func(void* arg) {
    distributed_ctx_t* ctx = (distributed_ctx_t*)arg;
    
    while (ctx->running) {
        /* Receive message with timeout */
        message_t* msg = distributed_receive_message(ctx, 100);
        
        if (msg) {
            /* Handle message based on type */
            switch (msg->type) {
                case MSG_TYPE_HEARTBEAT:
                    /* Update node heartbeat */
                    for (size_t i = 0; i < ctx->node_count; i++) {
                        if (ctx->nodes[i]->node_id == msg->source_node) {
                            ctx->nodes[i]->last_heartbeat = msg->timestamp;
                            ctx->nodes[i]->is_active = true;
                            break;
                        }
                    }
                    break;
                    
                case MSG_TYPE_NODE_JOIN:
                    if (ctx->on_node_join) {
                        /* Extract node info from payload */
                        node_info_t* node = (node_info_t*)msg->payload;
                        ctx->on_node_join(node, ctx->user_data);
                    }
                    break;
                    
                case MSG_TYPE_NODE_LEAVE:
                    if (ctx->on_node_leave) {
                        node_info_t* node = (node_info_t*)msg->payload;
                        ctx->on_node_leave(node, ctx->user_data);
                    }
                    break;
                    
                default:
                    /* Call user-defined message handler */
                    if (ctx->on_message) {
                        ctx->on_message(msg, ctx->user_data);
                    }
                    break;
            }
            
            distributed_free_message(msg);
        }
    }
    
    return NULL;
}

/* Distributed context operations */
distributed_ctx_t* distributed_create(uint32_t node_id, const char* hostname, uint16_t port) {
    distributed_ctx_t* ctx = calloc(1, sizeof(distributed_ctx_t));
    ctx->this_node_id = node_id;
    ctx->running = false;
    
    /* Create message queue */
    char mq_name[256];
    snprintf(mq_name, sizeof(mq_name), "/opencog_node_%u", node_id);
    ctx->mq = message_queue_create(mq_name, 100, 65536);
    
    /* Create shared memory */
    ctx->shm = shared_memory_create(1024 * 1024); /* 1MB */
    
    return ctx;
}

void distributed_destroy(distributed_ctx_t* ctx) {
    if (!ctx) return;
    
    if (ctx->running) {
        distributed_stop(ctx);
    }
    
    /* Free nodes */
    for (size_t i = 0; i < ctx->node_count; i++) {
        free(ctx->nodes[i]);
    }
    free(ctx->nodes);
    
    /* Destroy communication channels */
    if (ctx->mq) message_queue_destroy(ctx->mq);
    if (ctx->shm) shared_memory_destroy(ctx->shm);
    
    free(ctx);
}

int distributed_start(distributed_ctx_t* ctx) {
    if (!ctx || ctx->running) return -1;
    
    ctx->running = true;
    
    /* Start heartbeat thread */
    if (pthread_create(&ctx->heartbeat_thread, NULL, heartbeat_thread_func, ctx) != 0) {
        ctx->running = false;
        return -1;
    }
    
    /* Start message handler thread */
    if (pthread_create(&ctx->message_handler_thread, NULL, message_handler_thread_func, ctx) != 0) {
        ctx->running = false;
        pthread_cancel(ctx->heartbeat_thread);
        pthread_join(ctx->heartbeat_thread, NULL);
        return -1;
    }
    
    return 0;
}

int distributed_stop(distributed_ctx_t* ctx) {
    if (!ctx || !ctx->running) return -1;
    
    ctx->running = false;
    
    /* Wait for threads to finish */
    pthread_join(ctx->heartbeat_thread, NULL);
    pthread_join(ctx->message_handler_thread, NULL);
    
    return 0;
}

/* Node management */
int distributed_add_node(distributed_ctx_t* ctx, uint32_t node_id, const char* hostname, uint16_t port) {
    if (!ctx) return -1;
    
    node_info_t* node = malloc(sizeof(node_info_t));
    node->node_id = node_id;
    strncpy(node->hostname, hostname, sizeof(node->hostname) - 1);
    node->hostname[sizeof(node->hostname) - 1] = '\0';
    node->port = port;
    node->is_active = false;
    node->last_heartbeat = 0;
    
    ctx->nodes = realloc(ctx->nodes, sizeof(node_info_t*) * (ctx->node_count + 1));
    ctx->nodes[ctx->node_count++] = node;
    
    return 0;
}

int distributed_remove_node(distributed_ctx_t* ctx, uint32_t node_id) {
    if (!ctx) return -1;
    
    for (size_t i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i]->node_id == node_id) {
            free(ctx->nodes[i]);
            
            /* Shift remaining nodes */
            for (size_t j = i; j < ctx->node_count - 1; j++) {
                ctx->nodes[j] = ctx->nodes[j + 1];
            }
            ctx->node_count--;
            return 0;
        }
    }
    
    return -1;
}

/* Message operations - simplified implementation using System V message queues */
int distributed_send_message(distributed_ctx_t* ctx, message_t* msg) {
    if (!ctx || !msg) return -1;
    
    /* Serialize message */
    size_t total_size = sizeof(message_t) + msg->payload_size;
    char* buffer = malloc(total_size);
    memcpy(buffer, msg, sizeof(message_t));
    if (msg->payload_size > 0 && msg->payload) {
        memcpy(buffer + sizeof(message_t), msg->payload, msg->payload_size);
    }
    
    /* Send via message queue */
    int result = message_queue_send(ctx->mq, buffer, total_size, 0);
    free(buffer);
    
    return result;
}

message_t* distributed_receive_message(distributed_ctx_t* ctx, int timeout_ms) {
    if (!ctx) return NULL;
    
    char buffer[65536];
    int priority;
    
    /* Receive from message queue */
    int size = message_queue_receive(ctx->mq, buffer, sizeof(buffer), &priority, timeout_ms);
    if (size <= 0) return NULL;
    
    /* Deserialize message */
    message_t* msg = malloc(sizeof(message_t));
    memcpy(msg, buffer, sizeof(message_t));
    
    if (msg->payload_size > 0) {
        msg->payload = malloc(msg->payload_size);
        memcpy(msg->payload, buffer + sizeof(message_t), msg->payload_size);
    } else {
        msg->payload = NULL;
    }
    
    return msg;
}

void distributed_free_message(message_t* msg) {
    if (!msg) return;
    if (msg->payload) free(msg->payload);
    free(msg);
}

/* Shared memory operations */
shared_memory_t* shared_memory_create(size_t size) {
    shared_memory_t* shm = malloc(sizeof(shared_memory_t));
    
    /* Create shared memory segment */
    shm->shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    if (shm->shm_id < 0) {
        free(shm);
        return NULL;
    }
    
    /* Attach to shared memory */
    shm->shm_addr = shmat(shm->shm_id, NULL, 0);
    if (shm->shm_addr == (void*)-1) {
        shmctl(shm->shm_id, IPC_RMID, NULL);
        free(shm);
        return NULL;
    }
    
    shm->shm_size = size;
    
    /* Initialize mutex in shared memory */
    shm->lock = (pthread_mutex_t*)shm->shm_addr;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(shm->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    
    return shm;
}

void shared_memory_destroy(shared_memory_t* shm) {
    if (!shm) return;
    
    pthread_mutex_destroy(shm->lock);
    shmdt(shm->shm_addr);
    shmctl(shm->shm_id, IPC_RMID, NULL);
    free(shm);
}

void* shared_memory_lock(shared_memory_t* shm) {
    if (!shm) return NULL;
    pthread_mutex_lock(shm->lock);
    return (char*)shm->shm_addr + sizeof(pthread_mutex_t);
}

void shared_memory_unlock(shared_memory_t* shm) {
    if (!shm) return;
    pthread_mutex_unlock(shm->lock);
}

/* Message queue operations */
message_queue_t* message_queue_create(const char* name, size_t max_messages, size_t max_message_size) {
    message_queue_t* mq = malloc(sizeof(message_queue_t));
    
    /* Create message queue */
    key_t key = ftok(name, 'O');
    mq->mq_id = msgget(key, IPC_CREAT | 0666);
    if (mq->mq_id < 0) {
        free(mq);
        return NULL;
    }
    
    mq->max_messages = max_messages;
    mq->max_message_size = max_message_size;
    
    return mq;
}

void message_queue_destroy(message_queue_t* mq) {
    if (!mq) return;
    msgctl(mq->mq_id, IPC_RMID, NULL);
    free(mq);
}

int message_queue_send(message_queue_t* mq, const void* data, size_t size, int priority) {
    if (!mq || !data || size > mq->max_message_size) return -1;
    
    /* System V message structure */
    struct {
        long mtype;
        char mtext[65536];
    } msg_buf;
    
    msg_buf.mtype = priority + 1; /* Type must be > 0 */
    memcpy(msg_buf.mtext, data, size);
    
    return msgsnd(mq->mq_id, &msg_buf, size, IPC_NOWAIT);
}

int message_queue_receive(message_queue_t* mq, void* buffer, size_t size, int* priority, int timeout_ms) {
    if (!mq || !buffer) return -1;
    
    struct {
        long mtype;
        char mtext[65536];
    } msg_buf;
    
    /* Receive with timeout (simplified - using non-blocking) */
    int flags = (timeout_ms == 0) ? IPC_NOWAIT : 0;
    ssize_t received = msgrcv(mq->mq_id, &msg_buf, size, 0, flags);
    
    if (received > 0) {
        memcpy(buffer, msg_buf.mtext, received);
        if (priority) *priority = (int)(msg_buf.mtype - 1);
        return (int)received;
    }
    
    return -1;
}

/* Consensus operations - simplified Paxos-like implementation */
consensus_t* consensus_create(void* proposal_data, size_t size, size_t required_votes) {
    consensus_t* consensus = malloc(sizeof(consensus_t));
    consensus->proposal_id = time(NULL);
    consensus->phase = CONSENSUS_PROPOSE;
    consensus->proposal_data = malloc(size);
    memcpy(consensus->proposal_data, proposal_data, size);
    consensus->proposal_size = size;
    consensus->voted_nodes = calloc(required_votes, sizeof(uint32_t));
    consensus->vote_count = 0;
    consensus->required_votes = required_votes;
    return consensus;
}

void consensus_destroy(consensus_t* consensus) {
    if (!consensus) return;
    free(consensus->proposal_data);
    free(consensus->voted_nodes);
    free(consensus);
}

int consensus_propose(distributed_ctx_t* ctx, consensus_t* consensus) {
    /* TODO: Implement proposal distribution */
    return 0;
}

int consensus_vote(distributed_ctx_t* ctx, consensus_t* consensus, bool accept) {
    /* TODO: Implement voting mechanism */
    return 0;
}

bool consensus_is_committed(consensus_t* consensus) {
    return consensus->phase == CONSENSUS_COMMIT && 
           consensus->vote_count >= consensus->required_votes;
}

/* Stub functions for cluster operations */
int distributed_join_cluster(distributed_ctx_t* ctx, const char* coordinator_host, uint16_t coordinator_port) {
    /* TODO: Implement cluster join protocol */
    return 0;
}

int distributed_leave_cluster(distributed_ctx_t* ctx) {
    /* TODO: Implement cluster leave protocol */
    return 0;
}
