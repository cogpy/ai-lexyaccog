/*
 * OpenCog AtomSpace Implementation
 * Distributed knowledge representation with cognitive capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "../include/atom.h"

/* Thread-safe ID generator */
static pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t next_atom_id = 1;

static uint64_t generate_atom_id(void) {
    pthread_mutex_lock(&id_mutex);
    uint64_t id = next_atom_id++;
    pthread_mutex_unlock(&id_mutex);
    return id;
}

/* Hash table for atom lookup - simplified implementation */
#define HASH_TABLE_SIZE 10007

typedef struct hash_entry {
    uint64_t key;
    atom_handle_t* value;
    struct hash_entry* next;
} hash_entry_t;

typedef struct {
    hash_entry_t* buckets[HASH_TABLE_SIZE];
    pthread_rwlock_t lock;
} hash_table_t;

static hash_table_t* hash_table_create(void) {
    hash_table_t* table = calloc(1, sizeof(hash_table_t));
    pthread_rwlock_init(&table->lock, NULL);
    return table;
}

static void hash_table_insert(hash_table_t* table, uint64_t key, atom_handle_t* value) {
    pthread_rwlock_wrlock(&table->lock);
    size_t bucket = key % HASH_TABLE_SIZE;
    hash_entry_t* entry = malloc(sizeof(hash_entry_t));
    entry->key = key;
    entry->value = value;
    entry->next = table->buckets[bucket];
    table->buckets[bucket] = entry;
    pthread_rwlock_unlock(&table->lock);
}

static atom_handle_t* hash_table_lookup(hash_table_t* table, uint64_t key) {
    pthread_rwlock_rdlock(&table->lock);
    size_t bucket = key % HASH_TABLE_SIZE;
    hash_entry_t* entry = table->buckets[bucket];
    while (entry) {
        if (entry->key == key) {
            atom_handle_t* result = entry->value;
            pthread_rwlock_unlock(&table->lock);
            return result;
        }
        entry = entry->next;
    }
    pthread_rwlock_unlock(&table->lock);
    return NULL;
}

static void hash_table_destroy(hash_table_t* table) {
    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        hash_entry_t* entry = table->buckets[i];
        while (entry) {
            hash_entry_t* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    pthread_rwlock_destroy(&table->lock);
    free(table);
}

/* AtomSpace implementation */
atomspace_t* atomspace_create(uint32_t node_id) {
    atomspace_t* space = calloc(1, sizeof(atomspace_t));
    space->node_id = node_id;
    space->atom_capacity = 1024;
    space->atoms = calloc(space->atom_capacity, sizeof(atom_handle_t*));
    space->lookup_table = hash_table_create();
    return space;
}

void atomspace_destroy(atomspace_t* space) {
    if (!space) return;
    
    /* Release all atoms */
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i]) {
            atom_release(space->atoms[i]);
        }
    }
    
    free(space->atoms);
    hash_table_destroy((hash_table_t*)space->lookup_table);
    free(space);
}

/* Atom creation */
atom_handle_t* atom_create(atomspace_t* space, atom_type_t type, const char* name) {
    if (!space) return NULL;
    
    /* Allocate atom */
    atom_t* atom = calloc(1, sizeof(atom_t));
    atom->id = generate_atom_id();
    atom->type = type;
    atom->name = name ? strdup(name) : NULL;
    atom->tv.strength = 1.0;
    atom->tv.confidence = 0.0;
    atom->av.sti = 0;
    atom->av.lti = 0;
    atom->av.vlti = 0;
    atom->creation_time = time(NULL);
    atom->last_access_time = atom->creation_time;
    
    /* Create handle */
    atom_handle_t* handle = malloc(sizeof(atom_handle_t));
    handle->id = atom->id;
    handle->atom = atom;
    handle->ref_count = 1;
    
    /* Add to atomspace */
    if (space->atom_count >= space->atom_capacity) {
        space->atom_capacity *= 2;
        space->atoms = realloc(space->atoms, 
                              space->atom_capacity * sizeof(atom_handle_t*));
    }
    space->atoms[space->atom_count++] = handle;
    space->total_atoms_created++;
    
    /* Add to lookup table */
    hash_table_insert((hash_table_t*)space->lookup_table, atom->id, handle);
    
    return handle;
}

atom_handle_t* atom_create_link(atomspace_t* space, atom_type_t type,
                                atom_handle_t** outgoing, size_t count) {
    if (!space) return NULL;
    
    atom_handle_t* handle = atom_create(space, type, NULL);
    if (!handle) return NULL;
    
    atom_t* atom = handle->atom;
    
    /* Set outgoing set */
    if (count > 0) {
        atom->outgoing = malloc(sizeof(atom_handle_t*) * count);
        atom->outgoing_count = count;
        
        for (size_t i = 0; i < count; i++) {
            atom->outgoing[i] = outgoing[i];
            atom_retain(outgoing[i]);
            
            /* Add to incoming set of target atom */
            atom_t* target = outgoing[i]->atom;
            target->incoming = realloc(target->incoming,
                                      sizeof(atom_handle_t*) * (target->incoming_count + 1));
            target->incoming[target->incoming_count++] = handle;
        }
    }
    
    return handle;
}

void atom_retain(atom_handle_t* handle) {
    if (!handle) return;
    __sync_fetch_and_add(&handle->ref_count, 1);
}

void atom_release(atom_handle_t* handle) {
    if (!handle) return;
    
    if (__sync_sub_and_fetch(&handle->ref_count, 1) == 0) {
        atom_t* atom = handle->atom;
        
        /* Free outgoing atoms */
        for (size_t i = 0; i < atom->outgoing_count; i++) {
            atom_release(atom->outgoing[i]);
        }
        free(atom->outgoing);
        free(atom->incoming);
        free(atom->name);
        free(atom);
        free(handle);
    }
}

/* Truth value operations */
void atom_set_tv(atom_handle_t* handle, double strength, double confidence) {
    if (!handle) return;
    atom_t* atom = handle->atom;
    atom->tv.strength = strength;
    atom->tv.confidence = confidence;
    atom->last_access_time = time(NULL);
}

truth_value_t atom_get_tv(atom_handle_t* handle) {
    truth_value_t tv = {0.0, 0.0};
    if (handle) {
        tv = handle->atom->tv;
        handle->atom->last_access_time = time(NULL);
    }
    return tv;
}

/* Attention value operations */
void atom_set_av(atom_handle_t* handle, int16_t sti, int16_t lti, int16_t vlti) {
    if (!handle) return;
    atom_t* atom = handle->atom;
    atom->av.sti = sti;
    atom->av.lti = lti;
    atom->av.vlti = vlti;
    atom->last_access_time = time(NULL);
}

attention_value_t atom_get_av(atom_handle_t* handle) {
    attention_value_t av = {0, 0, 0};
    if (handle) {
        av = handle->atom->av;
        handle->atom->last_access_time = time(NULL);
    }
    return av;
}

/* Query operations */
atom_handle_t* atomspace_get_atom(atomspace_t* space, uint64_t id) {
    if (!space) return NULL;
    return hash_table_lookup((hash_table_t*)space->lookup_table, id);
}

atom_handle_t** atomspace_get_atoms_by_type(atomspace_t* space, atom_type_t type, size_t* count) {
    if (!space || !count) return NULL;
    
    /* Count matching atoms */
    size_t matches = 0;
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i] && space->atoms[i]->atom->type == type) {
            matches++;
        }
    }
    
    /* Allocate result array */
    atom_handle_t** result = malloc(sizeof(atom_handle_t*) * matches);
    size_t idx = 0;
    
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i] && space->atoms[i]->atom->type == type) {
            result[idx++] = space->atoms[i];
            atom_retain(space->atoms[i]);
        }
    }
    
    *count = matches;
    return result;
}

atom_handle_t** atomspace_get_atoms_by_name(atomspace_t* space, const char* name, size_t* count) {
    if (!space || !name || !count) return NULL;
    
    size_t matches = 0;
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i] && space->atoms[i]->atom->name &&
            strcmp(space->atoms[i]->atom->name, name) == 0) {
            matches++;
        }
    }
    
    atom_handle_t** result = malloc(sizeof(atom_handle_t*) * matches);
    size_t idx = 0;
    
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i] && space->atoms[i]->atom->name &&
            strcmp(space->atoms[i]->atom->name, name) == 0) {
            result[idx++] = space->atoms[i];
            atom_retain(space->atoms[i]);
        }
    }
    
    *count = matches;
    return result;
}

/* Pattern matching */
atom_handle_t** atomspace_match_pattern(atomspace_t* space, pattern_matcher_fn matcher,
                                       void* user_data, size_t* count) {
    if (!space || !matcher || !count) return NULL;
    
    size_t matches = 0;
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i] && matcher(space->atoms[i], user_data)) {
            matches++;
        }
    }
    
    atom_handle_t** result = malloc(sizeof(atom_handle_t*) * matches);
    size_t idx = 0;
    
    for (size_t i = 0; i < space->atom_count; i++) {
        if (space->atoms[i] && matcher(space->atoms[i], user_data)) {
            result[idx++] = space->atoms[i];
            atom_retain(space->atoms[i]);
        }
    }
    
    *count = matches;
    return result;
}

/* Distributed operations - stubs for now */
int atomspace_sync(atomspace_t* space) {
    /* TODO: Implement distributed synchronization */
    return 0;
}

int atomspace_replicate_atom(atomspace_t* space, atom_handle_t* handle, uint32_t target_node) {
    /* TODO: Implement atom replication to target node */
    return 0;
}
