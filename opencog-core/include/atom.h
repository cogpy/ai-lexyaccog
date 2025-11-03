#ifndef OPENCOG_ATOM_H
#define OPENCOG_ATOM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Atom types for cognitive representation */
typedef enum {
    ATOM_TYPE_CONCEPT,
    ATOM_TYPE_PREDICATE,
    ATOM_TYPE_LINK,
    ATOM_TYPE_NODE,
    ATOM_TYPE_VARIABLE,
    ATOM_TYPE_EVALUATION,
    ATOM_TYPE_EXECUTION,
    ATOM_TYPE_CUSTOM
} atom_type_t;

/* Truth value representation */
typedef struct {
    double strength;      /* Probability [0.0, 1.0] */
    double confidence;    /* Confidence [0.0, 1.0] */
} truth_value_t;

/* Attention value for importance tracking */
typedef struct {
    int16_t sti;         /* Short-term importance */
    int16_t lti;         /* Long-term importance */
    int16_t vlti;        /* Very long-term importance */
} attention_value_t;

/* Forward declarations */
typedef struct atom atom_t;
typedef struct atom_handle atom_handle_t;

/* Atom handle for reference counting */
struct atom_handle {
    uint64_t id;          /* Unique atom identifier */
    atom_t* atom;         /* Pointer to actual atom */
    uint32_t ref_count;   /* Reference counter */
};

/* Core atom structure */
struct atom {
    uint64_t id;                  /* Unique identifier */
    atom_type_t type;             /* Atom type */
    char* name;                   /* Atom name/value */
    truth_value_t tv;             /* Truth value */
    attention_value_t av;         /* Attention value */
    
    /* Outgoing set (for links) */
    atom_handle_t** outgoing;
    size_t outgoing_count;
    
    /* Incoming set (back-references) */
    atom_handle_t** incoming;
    size_t incoming_count;
    
    /* Metadata */
    void* user_data;
    uint64_t creation_time;
    uint64_t last_access_time;
};

/* AtomSpace - distributed knowledge base */
typedef struct {
    atom_handle_t** atoms;        /* Array of atom handles */
    size_t atom_count;
    size_t atom_capacity;
    
    /* Hash table for fast lookup */
    void* lookup_table;
    
    /* Statistics */
    uint64_t total_atoms_created;
    uint64_t total_atoms_deleted;
    
    /* Distributed coordination */
    uint32_t node_id;             /* This node's ID in distributed system */
    void* coordination_ctx;       /* Coordination context */
} atomspace_t;

/* AtomSpace operations */
atomspace_t* atomspace_create(uint32_t node_id);
void atomspace_destroy(atomspace_t* space);

/* Atom creation and manipulation */
atom_handle_t* atom_create(atomspace_t* space, atom_type_t type, const char* name);
atom_handle_t* atom_create_link(atomspace_t* space, atom_type_t type, 
                                atom_handle_t** outgoing, size_t count);
void atom_retain(atom_handle_t* handle);
void atom_release(atom_handle_t* handle);

/* Truth value operations */
void atom_set_tv(atom_handle_t* handle, double strength, double confidence);
truth_value_t atom_get_tv(atom_handle_t* handle);

/* Attention value operations */
void atom_set_av(atom_handle_t* handle, int16_t sti, int16_t lti, int16_t vlti);
attention_value_t atom_get_av(atom_handle_t* handle);

/* Query operations */
atom_handle_t* atomspace_get_atom(atomspace_t* space, uint64_t id);
atom_handle_t** atomspace_get_atoms_by_type(atomspace_t* space, atom_type_t type, size_t* count);
atom_handle_t** atomspace_get_atoms_by_name(atomspace_t* space, const char* name, size_t* count);

/* Pattern matching */
typedef bool (*pattern_matcher_fn)(atom_handle_t* atom, void* user_data);
atom_handle_t** atomspace_match_pattern(atomspace_t* space, pattern_matcher_fn matcher, 
                                       void* user_data, size_t* count);

/* Distributed operations */
int atomspace_sync(atomspace_t* space);
int atomspace_replicate_atom(atomspace_t* space, atom_handle_t* handle, uint32_t target_node);

#ifdef __cplusplus
}
#endif

#endif /* OPENCOG_ATOM_H */
