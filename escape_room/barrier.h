typedef struct
{
    size_t count;
    size_t strength;
    sem_t mutex;
    sem_t barrier;
} barrier;

void barrier_init(barrier* self, size_t strength);
void barrier_destroy(barrier* self);
void barrier_await(barrier* self);
