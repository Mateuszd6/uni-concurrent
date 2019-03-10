void barrier_init(barrier* self, size_t strength)
{
    sem_init(&self->mutex, 1, 1);
    sem_init(&self->barrier, 1, 0);

    self->count = 0;
    self->strength = strength;
}

void barrier_destroy(barrier* self)
{
    // TODO: Check errno?
    sem_destroy(&self->mutex);
    sem_destroy(&self->barrier);
    self->count = 0;
    self->strength = 0;
}

void barrier_await(barrier* self)
{
    P(self->mutex);
    self->count++;
    V(self->mutex);

    if (self->count == self->strength)
        V(self->barrier);

    P(self->barrier);
    V(self->barrier);
}
