// NOTE: This feature test macro is (with some reason) required to supress the
//       warrning about implicite declaration of getline and ftruncate
#define _GNU_SOURCE

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>

#include "common.c" // Make it separate trans unit.

void start_player(int playerid)
{
    char playerid_str[16];
    sprintf(playerid_str, "%d", playerid);

    pid_t pid;
    switch (pid = fork())
    {
    case -1:
        syserr("Error in fork");
        break;

    case 0:
        execl("player", "player", playerid_str, 0);
        syserr("Error in execlp");
        break;

    default:
        // TODO: Get rid ot this!
        // if (wait(0) == -1)
        //    syserr("Error in wait");
        break;
    }
}

void load_input(int* numplayers, int* numrooms, room** rooms)
{
    scanf("%d%d", numplayers, numrooms);
    (*rooms) = malloc(sizeof(room) * (*numrooms));

    for(int i = 0; i < *numrooms; ++i)
    {
        char ch[2];
        int siz;
        scanf("%s%d", ch, &siz);

        (*rooms)[i].type = ch[0];
        (*rooms)[i].size = siz;
    }
}

mem_data* loaded_memory;
int main()
{
    int num_players;
    int num_rooms;
    room* rooms;
    loaded_memory = (mem_data*)load_shared_mem(1);

    load_input(&num_players, &num_rooms, &rooms);

    // Initialize shared memory:
    loaded_memory->num_players = num_players;
    loaded_memory->num_rooms = num_rooms;

    for(int i = 0; i < 'Z' - 'A' + 1; ++i)
        loaded_memory->rooms_with_type[i] = 0;

    for(int i = 0; i < MAX_ROOMS + 2; ++i)
        loaded_memory->room_open[i] = 1;

    for(int i = 1; i <= num_rooms; ++i)
    {
        loaded_memory->rooms[i] = rooms[i - 1];
        loaded_memory->rooms_with_type[rooms[i - 1].type - 'A']++;

    }
    free(rooms);

    // Prints some general info
    {
        LOG("Num players: %d\n", num_players);
        for(int i = 1; i <= num_rooms; ++i)
            LOG("Room %d: %c ; %d\n", i + 1, loaded_memory->rooms[i].type, loaded_memory->rooms[i].size);
    }

    // Print rooms info:
    {
        LOG("Rooms: (%d): { ", loaded_memory->num_rooms);
        for(int i = 1; i <= loaded_memory->num_rooms; ++i)
        {
            LOG("[%c](size = %d) ",
                loaded_memory->rooms[i].type,
                loaded_memory->rooms[i].size);
        }
        LOG("}\n");

        for(int i = 'A'; i <= 'Z'; ++i)
            LOG("%c ", i);
        LOG("\n");
        for(int i = 0; i < 'Z' - 'A' + 1; ++i)
            LOG("%d ", loaded_memory->rooms_with_type[i]);
        LOG("\n");
    }

    suggestion_queue_init(&loaded_memory->suggestions);
    // TODO: Check errno?
    sem_init(&loaded_memory->type_members_mutex, 1, 1);
    sem_init(&loaded_memory->player_exit_semaphore, 1, 1);
    sem_init(&loaded_memory->manager_exit_semaphore, 1, 0);
    sem_init(&loaded_memory->suggestions_queue_mutex, 1, 1);
    sem_init(&loaded_memory->main_mutex, 1, 1);
    for(int i = 0; i <= num_players; ++i)
        sem_init(loaded_memory->awaiting_players + i, 1, 0);
    sem_init(&loaded_memory->DEBUG_MUTEX, 1, 1);
    for(int i = 0; i <= num_rooms; ++i)
    {
        sem_init(&(loaded_memory->available_games[i].entry_sem), 1, 0);
        sem_init(&(loaded_memory->available_games[i].exit_sem), 1, 0);
        loaded_memory->available_games[i].size = 0;
        loaded_memory->available_games[i].desired_size = 0;
    }

    barrier_init(&loaded_memory->init_barrier, num_players);

    print_table((char*)(loaded_memory + 0), SHM_BUFF_SIZE);

    for(int i = 1; i <= num_players; ++i)
        start_player(i);

    for(int i = 1; i <= num_players; ++i)
    {
        P(loaded_memory->manager_exit_semaphore);

        // One process has player_exit_semaphore and gives us info about his
        // play. We must display it and post player_exit_semaphore so that next
        // player can go through it.
        LOG("Player %s finished! Score: %lu\n",
            loaded_memory->leaving_player_name,
            loaded_memory->leaving_player_score);
        V(loaded_memory->player_exit_semaphore);
    }

    // TODO: This is just test if queue works.
    sleep(1);
    LOG("Everyone has left. Cleaning up...\n");

    for(size_t k = loaded_memory->suggestions.tail_idx;
        k != loaded_memory->suggestions.head_idx;
        ++k)
    {
        suggestion* ptr = loaded_memory->suggestions.buffer + k;
        LOG("Suggestion (author: %d, room type: %c) is:\n\tPlayers: ", ptr->author, ptr->room_type);
        for(size_t i = 0; i < ptr->inv_player_idx; ++i)
            LOG("%d ", ptr->inv_players[i]);
        LOG("\n\tTypes: ");
        for(size_t i = 0; i  < 'Z' - 'A' + 1; ++i)
            if(ptr->inv_types[i])
                LOG("%c ", (char)(i + 'A'));
        LOG("\n");
    }

    LOG("Player preferences (for each player):\n");
    for(int i = 1; i <= loaded_memory->num_players; ++i)
        LOG("\tPlayer %d -> %c\n", i, loaded_memory->player_prefs[i]);
    LOG("Player preferences (for each room):\n");
    for(int i = 0; i < 'Z' - 'A' + 1; ++i)
        if(loaded_memory->type_members[i] > 0)
            LOG("\tType %c -> %d\n", 'A' + i, loaded_memory->type_members[i]);
    LOG("\n");

    // Cleanup:
    close_shared_mem((char*)loaded_memory, 1);

    LOG("Overlord done\n");
    return 0;
}
