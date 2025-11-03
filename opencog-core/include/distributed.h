#ifndef OPENCOG_DISTRIBUTED_H
#define OPENCOG_DISTRIBUTED_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Distributed OS primitives for OpenCog */

/* Node information in distributed system */
typedef struct {
    uint32_t node_id;
    char hostname[256];
    uint16_t port;
    bool is_active;
    uint64_t last_heartbeat;
} node_info_t;

/* Message types for inter-node communication */
typedef enum {
    MSG_TYPE_ATOM_CREATE,
    MSG_TYPE_ATOM_UPDATE,
    MSG_TYPE_ATOM_DELETE,
    MSG_TYPE_ATOM_QUERY,
    MSG_TYPE_ATOM_RESPONSE,
    MSG_TYPE_SYNC_REQUEST,
    MSG_TYPE_SYNC_RESPONSE,
    MSG_TYPE_HEARTBEAT,
    MSG_TYPE_NODE_JOIN,
    MSG_TYPE_NODE_LEAVE
} message_type_t;

/* Message structure for distributed communication */
typedef struct {
    message_type_t type;
    uint32_t source_node;
    uint32_t dest_node;
    uint64_t timestamp;
    size_t payload_size;
    void* payload;
} message_t;

/* Shared memory segment for IPC */
typedef struct {
    int shm_id;
    void* shm_addr;
    size_t shm_size;
    pthread_mutex_t* lock;
} shared_memory_t;

/* Message queue for async communication */
typedef struct {
    int mq_id;
    size_t max_messages;
    size_t max_message_size;
} message_queue_t;

/* Distributed coordination context */
typedef struct {
    uint32_t this_node_id;
    node_info_t** nodes;
    size_t node_count;
    
    /* Communication channels */
    message_queue_t* mq;
    shared_memory_t* shm;
    
    /* Synchronization */
    pthread_t heartbeat_thread;
    pthread_t message_handler_thread;
    bool running;
    
    /* Callbacks */
    void (*on_message)(message_t* msg, void* user_data);
    void (*on_node_join)(node_info_t* node, void* user_data);
    void (*on_node_leave)(node_info_t* node, void* user_data);
    void* user_data;
} distributed_ctx_t;

/* Distributed context operations */
distributed_ctx_t* distributed_create(uint32_t node_id, const char* hostname, uint16_t port);
void distributed_destroy(distributed_ctx_t* ctx);
int distributed_start(distributed_ctx_t* ctx);
int distributed_stop(distributed_ctx_t* ctx);

/* Node management */
int distributed_join_cluster(distributed_ctx_t* ctx, const char* coordinator_host, uint16_t coordinator_port);
int distributed_leave_cluster(distributed_ctx_t* ctx);
int distributed_add_node(distributed_ctx_t* ctx, uint32_t node_id, const char* hostname, uint16_t port);
int distributed_remove_node(distributed_ctx_t* ctx, uint32_t node_id);

/* Message operations */
int distributed_send_message(distributed_ctx_t* ctx, message_t* msg);
message_t* distributed_receive_message(distributed_ctx_t* ctx, int timeout_ms);
void distributed_free_message(message_t* msg);

/* Shared memory operations */
shared_memory_t* shared_memory_create(size_t size);
void shared_memory_destroy(shared_memory_t* shm);
void* shared_memory_lock(shared_memory_t* shm);
void shared_memory_unlock(shared_memory_t* shm);

/* Message queue operations */
message_queue_t* message_queue_create(const char* name, size_t max_messages, size_t max_message_size);
void message_queue_destroy(message_queue_t* mq);
int message_queue_send(message_queue_t* mq, const void* data, size_t size, int priority);
int message_queue_receive(message_queue_t* mq, void* buffer, size_t size, int* priority, int timeout_ms);

/* Distributed consensus operations */
typedef enum {
    CONSENSUS_PROPOSE,
    CONSENSUS_ACCEPT,
    CONSENSUS_REJECT,
    CONSENSUS_COMMIT
} consensus_phase_t;

typedef struct {
    uint64_t proposal_id;
    consensus_phase_t phase;
    void* proposal_data;
    size_t proposal_size;
    uint32_t* voted_nodes;
    size_t vote_count;
    size_t required_votes;
} consensus_t;

consensus_t* consensus_create(void* proposal_data, size_t size, size_t required_votes);
void consensus_destroy(consensus_t* consensus);
int consensus_propose(distributed_ctx_t* ctx, consensus_t* consensus);
int consensus_vote(distributed_ctx_t* ctx, consensus_t* consensus, bool accept);
bool consensus_is_committed(consensus_t* consensus);

#ifdef __cplusplus
}
#endif

#endif /* OPENCOG_DISTRIBUTED_H */
