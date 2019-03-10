#include "barrier.h"

#define MAX_PLAYERS (1024)
#define MAX_ROOMS (1024)

typedef struct
{
    // Array of the invited player ids.
    short inv_players[MAX_PLAYERS];

    size_t inv_player_idx;

    // Invited types of players. 0 means not invited, otherwise invited.
    int inv_types['Z' - 'A' + 10];

    int author;
    char room_type;
} suggestion;

typedef struct
{
    // Max number of suggestions is not greater than the max number of players,
    // becuase each does one at the time.
    suggestion buffer[MAX_PLAYERS + 2];

    size_t head_idx;
    size_t tail_idx;
} suggestion_queue;

typedef struct
{
    sem_t entry_sem;
    sem_t exit_sem;

    size_t size;
    size_t desired_size;

    short players_vector[MAX_PLAYERS + 2];
    int players_vector_idx;
} game;

void suggestion_queue_init(suggestion_queue* queue);
int suggestion_queue_take(suggestion_queue* queue, suggestion* res);
void suggestion_queue_push(suggestion_queue* queue, suggestion value);

typedef struct
{
    char type;
    int size;
} room;

typedef struct
{
    int num_players;
    int num_rooms;

    sem_t type_members_mutex;

    // How many players prefer each room. Must acquire type_members_mutex to
    // change.
    int type_members['Z' - 'A' + 2];

    // This holds info prefered room for each player. Does not really require
    // mutex, when we assume that player with index i is allowed only to write
    // into player_prefs[i], and reading is done after init which syncs all
    // players with a barrier.
    char player_prefs[MAX_PLAYERS + 2];

    room rooms[MAX_ROOMS + 2];
    // This speeds up checking if room is available.
    int rooms_with_type['Z' - 'A' + 2];

    // The suggestions queue and its mutex.
    // TODO: This should probobly be protected with the main_mutex
    sem_t suggestions_queue_mutex;
    suggestion_queue suggestions;

    // Used when initialization to make sure all players have entered before
    // doing anything else.
    barrier init_barrier;

    // These contains the name and the score of the player that is leaving the
    // party (can be one at the time).
    char leaving_player_name[16];
    size_t leaving_player_score;

    sem_t player_exit_semaphore;
    sem_t manager_exit_semaphore;

    // ------ TODO Main mutex, really?
    sem_t main_mutex;

    char player_awaits[MAX_PLAYERS + 2];
    sem_t awaiting_players[MAX_PLAYERS + 2];

    // Game corresponds to room.
    char room_open[MAX_ROOMS + 2];
    game available_games[MAX_ROOMS + 2];

    // 0 if no game is asigned. If player gets worken up and this is 0, it means
    // that he can safetly quit.
    int player_to_room[MAX_PLAYERS + 2];

    // Use to sync everyyyything.
    sem_t DEBUG_MUTEX; // TODO: Remove later.
} mem_data;

void syserr(const char *fmt, ...);

#define LOG(...) fprintf(stderr, __VA_ARGS__)

#define P(semaphore_)                           \
    do {                                        \
        if (sem_wait(&semaphore_))              \
            syserr("sem_wait");                 \
    } while(0)

#define V(semaphore_)                           \
    do {                                        \
        if (sem_post(&semaphore_))              \
            syserr("sem_post");                 \
    } while(0)
