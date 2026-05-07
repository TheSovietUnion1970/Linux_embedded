#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <urcu.h>          // Userspace RCU

struct Data {
    int id;
    char message[64];
    long long timestamp;
};

// Global pointer protected by RCU
struct Data __rcu *global_data = NULL;

void *reader_thread(void *arg) {
    int id = (intptr_t)arg;
    
    for (int i = 0; i < 10; i++) {
        rcu_read_lock();                    // Start RCU read-side critical section
        
        struct Data *data = rcu_dereference(global_data);
        if (data) {
            printf("Reader %d: id=%d, msg=%s, i=%d\n", 
                   id, data->id, data->message, i);
        }
        
        rcu_read_unlock();                  // End critical section
        
        usleep(100000); // 100ms
    }
    return NULL;
}

void *writer_thread(void *arg) {
    int version = 0;
    
    for (int i = 0; i < 5; i++) {
        // Create new copy
        struct Data *new_data = malloc(sizeof(struct Data));
        new_data->id = ++version;
        snprintf(new_data->message, sizeof(new_data->message), "Updated version %d", version);
        new_data->timestamp = time(NULL);
        
        // Publish new version
        struct Data *old_data = rcu_xchg_pointer(&global_data, new_data);
        
        printf(">>> Writer updated to version %d\n", version);
        
        // Wait for all readers to finish with old version
        synchronize_rcu();
        
        // Safe to free old data now
        if (old_data) free(old_data);
        
        sleep(1);
    }
    return NULL;
}

#define NUM_THREAD 10
int main() {
    pthread_t readers[NUM_THREAD], writer;
    
    // Initialize RCU
    rcu_register_thread();
    
    // Initial data
    struct Data *init = malloc(sizeof(struct Data));
    init->id = 0;
    strcpy(init->message, "Initial data");
    init->timestamp = time(NULL);
    rcu_assign_pointer(global_data, init);
    
    // Create 3 readers
    for (int i = 0; i < NUM_THREAD; i++) {
        pthread_create(&readers[i], NULL, reader_thread, (void*)(intptr_t)i); // multi phreads try to read data
        if (i == NUM_THREAD/2) pthread_create(&writer, NULL, writer_thread, NULL); // suddenly, there is a write update
    }
    
    
    // Wait for threads
    for (int i = 0; i < NUM_THREAD; i++) pthread_join(readers[i], NULL);
    pthread_join(writer, NULL);
    
    // Cleanup
    synchronize_rcu();
    free(rcu_dereference(global_data));
    rcu_unregister_thread();
    
    return 0;
}
// gcc -o r rcu.c -lurcu -lpthread

