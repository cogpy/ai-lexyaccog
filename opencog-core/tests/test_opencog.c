/*
 * OpenCog Core Test Suite
 * Basic functionality tests for AtomSpace and distributed operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/atom.h"
#include "../include/distributed.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("Running test: %s...", #name); \
    if (test_##name()) { \
        printf(" PASSED\n"); \
        tests_passed++; \
    } else { \
        printf(" FAILED\n"); \
        tests_failed++; \
    }

/* AtomSpace Tests */

int test_atomspace_create_destroy() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atomspace_destroy(space);
    return 1;
}

int test_atom_creation() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atom_handle_t* atom = atom_create(space, ATOM_TYPE_CONCEPT, "TestConcept");
    if (!atom) {
        atomspace_destroy(space);
        return 0;
    }
    
    if (atom->atom->type != ATOM_TYPE_CONCEPT) {
        atomspace_destroy(space);
        return 0;
    }
    
    if (strcmp(atom->atom->name, "TestConcept") != 0) {
        atomspace_destroy(space);
        return 0;
    }
    
    atomspace_destroy(space);
    return 1;
}

int test_truth_value_operations() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atom_handle_t* atom = atom_create(space, ATOM_TYPE_CONCEPT, "TestConcept");
    if (!atom) {
        atomspace_destroy(space);
        return 0;
    }
    
    atom_set_tv(atom, 0.8, 0.9);
    truth_value_t tv = atom_get_tv(atom);
    
    if (tv.strength != 0.8 || tv.confidence != 0.9) {
        atomspace_destroy(space);
        return 0;
    }
    
    atomspace_destroy(space);
    return 1;
}

int test_attention_value_operations() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atom_handle_t* atom = atom_create(space, ATOM_TYPE_CONCEPT, "TestConcept");
    if (!atom) {
        atomspace_destroy(space);
        return 0;
    }
    
    atom_set_av(atom, 100, 50, 25);
    attention_value_t av = atom_get_av(atom);
    
    if (av.sti != 100 || av.lti != 50 || av.vlti != 25) {
        atomspace_destroy(space);
        return 0;
    }
    
    atomspace_destroy(space);
    return 1;
}

int test_link_creation() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atom_handle_t* atom1 = atom_create(space, ATOM_TYPE_CONCEPT, "Concept1");
    atom_handle_t* atom2 = atom_create(space, ATOM_TYPE_CONCEPT, "Concept2");
    
    if (!atom1 || !atom2) {
        atomspace_destroy(space);
        return 0;
    }
    
    atom_handle_t* outgoing[2] = { atom1, atom2 };
    atom_handle_t* link = atom_create_link(space, ATOM_TYPE_LINK, outgoing, 2);
    
    if (!link) {
        atomspace_destroy(space);
        return 0;
    }
    
    if (link->atom->outgoing_count != 2) {
        atomspace_destroy(space);
        return 0;
    }
    
    if (link->atom->outgoing[0] != atom1 || link->atom->outgoing[1] != atom2) {
        atomspace_destroy(space);
        return 0;
    }
    
    atomspace_destroy(space);
    return 1;
}

int test_atom_query_by_type() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atom_create(space, ATOM_TYPE_CONCEPT, "Concept1");
    atom_create(space, ATOM_TYPE_CONCEPT, "Concept2");
    atom_create(space, ATOM_TYPE_PREDICATE, "Predicate1");
    
    size_t count = 0;
    atom_handle_t** results = atomspace_get_atoms_by_type(space, ATOM_TYPE_CONCEPT, &count);
    
    if (count != 2) {
        atomspace_destroy(space);
        return 0;
    }
    
    for (size_t i = 0; i < count; i++) {
        atom_release(results[i]);
    }
    free(results);
    
    atomspace_destroy(space);
    return 1;
}

int test_atom_query_by_name() {
    atomspace_t* space = atomspace_create(1);
    if (!space) return 0;
    
    atom_create(space, ATOM_TYPE_CONCEPT, "TestConcept");
    atom_create(space, ATOM_TYPE_CONCEPT, "TestConcept");
    atom_create(space, ATOM_TYPE_CONCEPT, "OtherConcept");
    
    size_t count = 0;
    atom_handle_t** results = atomspace_get_atoms_by_name(space, "TestConcept", &count);
    
    if (count != 2) {
        atomspace_destroy(space);
        return 0;
    }
    
    for (size_t i = 0; i < count; i++) {
        atom_release(results[i]);
    }
    free(results);
    
    atomspace_destroy(space);
    return 1;
}

/* Distributed System Tests */

int test_distributed_context_create() {
    distributed_ctx_t* ctx = distributed_create(1, "localhost", 5000);
    if (!ctx) return 0;
    
    distributed_destroy(ctx);
    return 1;
}

int test_node_management() {
    distributed_ctx_t* ctx = distributed_create(1, "localhost", 5000);
    if (!ctx) return 0;
    
    int result = distributed_add_node(ctx, 2, "node2.local", 5001);
    if (result != 0) {
        distributed_destroy(ctx);
        return 0;
    }
    
    if (ctx->node_count != 1) {
        distributed_destroy(ctx);
        return 0;
    }
    
    result = distributed_remove_node(ctx, 2);
    if (result != 0) {
        distributed_destroy(ctx);
        return 0;
    }
    
    if (ctx->node_count != 0) {
        distributed_destroy(ctx);
        return 0;
    }
    
    distributed_destroy(ctx);
    return 1;
}

int test_shared_memory() {
    shared_memory_t* shm = shared_memory_create(4096);
    if (!shm) return 0;
    
    void* addr = shared_memory_lock(shm);
    if (!addr) {
        shared_memory_destroy(shm);
        return 0;
    }
    
    /* Write test data */
    strcpy((char*)addr, "Test data");
    shared_memory_unlock(shm);
    
    /* Read back */
    addr = shared_memory_lock(shm);
    if (strcmp((char*)addr, "Test data") != 0) {
        shared_memory_unlock(shm);
        shared_memory_destroy(shm);
        return 0;
    }
    shared_memory_unlock(shm);
    
    shared_memory_destroy(shm);
    return 1;
}

/* Main test runner */
int main(int argc, char** argv) {
    printf("OpenCog Core Test Suite\n");
    printf("=======================\n\n");
    
    /* AtomSpace tests */
    printf("AtomSpace Tests:\n");
    TEST(atomspace_create_destroy);
    TEST(atom_creation);
    TEST(truth_value_operations);
    TEST(attention_value_operations);
    TEST(link_creation);
    TEST(atom_query_by_type);
    TEST(atom_query_by_name);
    
    printf("\n");
    
    /* Distributed system tests */
    printf("Distributed System Tests:\n");
    TEST(distributed_context_create);
    TEST(node_management);
    TEST(shared_memory);
    
    printf("\n");
    printf("=======================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Total tests:  %d\n", tests_passed + tests_failed);
    
    return (tests_failed == 0) ? 0 : 1;
}
