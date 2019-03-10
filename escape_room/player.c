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
#include <unistd.h>
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
#include <assert.h>

#include "common.c"

static char g_player_type;
static FILE* g_input_file;
static int g_player_id;
static mem_data* loaded_memory;

void initialize_input()
{
    char filename_buffer[128];
    filename_buffer[0] = 0;
    sprintf(filename_buffer, "player-%d.in", g_player_id);
    g_input_file = fopen(filename_buffer, "r");
    if(g_input_file == 0)
        syserr("Could not open a file %s!", filename_buffer);
    else
        LOG("Player %d opened an input file\n", g_player_id);

    char input_unit[40];
    char* loaded_line = 0;
    size_t loaded_line_len = 0;

    if (getline(&loaded_line, &loaded_line_len, g_input_file) != EOF)
    {
        if (sscanf(loaded_line, "%s", input_unit) == EOF)
            syserr("sscanf");

        g_player_type = input_unit[0];
        LOG("Player %d loaded his type: %c\n", g_player_id, input_unit[0]);
    }
    else
        syserr("Unexpected end of file.");

    // Loaded line is alloacted with malloc by getline.
    if(loaded_line)
        free(loaded_line);
}

// This is used to check if suggestion is possible, and finds the best room with
// size larger or eq. that number of players. The id of the room is
// returned. The 0 is returned if no such room was found (there was a room with
// ok type, but not with sufficient size).
int get_possibly_best_room_for_game(suggestion* s)
{
    int impl_asked = 0;
    for(int i = 'A'; i<= 'Z'; ++i)
        impl_asked += s->inv_types[i];

    int best_idx = 0;
    for(int i = 1; i <= loaded_memory->num_rooms; ++i)
        if(((size_t)(loaded_memory->rooms[i].size)) >= s->inv_player_idx + 1 + impl_asked
           && s->room_type == loaded_memory->rooms[i].type
           && (best_idx == 0 || loaded_memory->rooms[best_idx].size > loaded_memory->rooms[i].size))
        {
            best_idx = i;
        }

    // We must add exacly one guy from each indirectly specified favours?
    return best_idx;
}

// This is used to search for best room available right now.
int get_best_room_curr_time(suggestion* s)
{
    int best_idx = 0;
    int impl_asked = 0;
    for(int i = 'A'; i<= 'Z'; ++i)
        impl_asked += s->inv_types[i];

    for(int i = 1; i <= loaded_memory->num_rooms; ++i)
        if(((size_t)(loaded_memory->rooms[i].size)) >= s->inv_player_idx + 1 + impl_asked
           && loaded_memory->room_open[i]
           && s->room_type == loaded_memory->rooms[i].type
           && (best_idx == 0 || loaded_memory->rooms[best_idx].size > loaded_memory->rooms[i].size))
        {
            best_idx = i;
        }

    return best_idx;
}

// Check if room is ok and if we can add up players so that the game will be
// ever possible.
int check_if_suggestion_possible(suggestion* s)
{
    char used_players[MAX_PLAYERS + 2];
    int impl_invited['Z' - 'A' + 10];

    memset((void*)used_players, 0, MAX_PLAYERS + 2);
    for(int i = 'A'; i <= 'Z'; ++i)
        impl_invited[i - 'A'] = s->inv_types[i - 'A'];

    if (loaded_memory->rooms_with_type[s->room_type - 'A'] == 0)
    {
        LOG("Game is impossible, because there is no such room %c\n", s->room_type);
        return 0;
    }

    char needed['Z' - 'A' + 2];
    memset((void*)needed, 0, 'Z' - 'A' + 2);
    for(size_t i = 0; i < s->inv_player_idx; ++i)
        if (s->inv_players[i] >= loaded_memory->num_players + 1)
        {
            LOG("Game is impossible, because there is no such player %d\n", s->inv_players[i]);
            return 0;
        }

    int best_room_id = get_possibly_best_room_for_game(s);
    if(best_room_id == 0)
    {
        LOG("Game is impossible, becuase no room with given type is large enough.\n");
        return 0;
    }

    for(int i = 1; i <= loaded_memory->num_players; ++i)
        if(!used_players[i]
           && impl_invited[loaded_memory->player_prefs[i] - 'A'] > 0)
        {
            impl_invited[loaded_memory->player_prefs[i] - 'A']--;
            used_players[i] = 1;
            LOG("Implicitly invited player %d (pref %c)\n", i, loaded_memory->player_prefs[i]);
        }

    for(int i = 'A'; i <= 'Z'; ++i)
        if (impl_invited[i - 'A'] > 0)
        {
            LOG("Cannot start now, bacause there is not enough players with %c prefs.\n", i);
            return 0;
        }

    LOG("best room for the game: [%c](%d)\n",
        loaded_memory->rooms[best_room_id].type,
        loaded_memory->rooms[best_room_id].size);

    return 1;
}

// These contains the list of players that are inplicitly ask to play the game.
int pref_players_vector[MAX_PLAYERS + 2];
int pref_players_vector_idx = 0;
// Must acquire mutex to do this.
int check_for_suggestion_right_now(suggestion* s, int* room_idx, suggestion** game_found_sug)
{
    char used_players[MAX_PLAYERS + 2];
    int impl_invited['Z' - 'A' + 10];
    pref_players_vector_idx = 0;

    memset((void*)used_players, 0, MAX_PLAYERS + 2);
    for(int i = 'A'; i <= 'Z'; ++i)
        impl_invited[i - 'A'] = s->inv_types[i - 'A'];

    // TODO: Check if there is a room which is opened.
    // TODO: This can be checked later? It takes a lot of time.
    int best_room_id = get_best_room_curr_time(s);
    if(best_room_id == 0)
    {
        LOG("Cannot start now, because there is no room than can contain all players\n");
        return 0;
    }

    // Check if explicit players wait.
    if (!loaded_memory->player_awaits[s->author])
    {
        LOG("Cannot start now, because %d(author) does not wait\n", s->author);
        return 0;
    }
    else
        used_players[s->author] = 1;

    for(size_t i = 0; i < s->inv_player_idx; ++i)
        if (!loaded_memory->player_awaits[s->inv_players[i]])
        {
            LOG("Cannot start now, because %d does not wait\n", s->inv_players[i]);
            return 0;
        }
        else
            used_players[s->inv_players[i]] = 1;

    for(int i = 1; i <= loaded_memory->num_players; ++i)
        if(!used_players[i]
           && (loaded_memory->player_awaits[i])
           && impl_invited[loaded_memory->player_prefs[i] - 'A'] > 0)
        {
            impl_invited[loaded_memory->player_prefs[i] - 'A']--;
            used_players[i] = 1;
            LOG("Implicitly invited player %d (pref %c)\n", i, loaded_memory->player_prefs[i]);
            pref_players_vector[pref_players_vector_idx++] = i;
        }

    for(int i = 'A'; i <= 'Z'; ++i)
        if (impl_invited[i - 'A'] > 0)
        {
            LOG("Cannot start now, bacause there is not enough players with %c prefs.\n", i);
            return 0;
        }

    // TODO: Check if we can add up players from awaiting players.
    LOG("... looks OK!\n");
    *room_idx = best_room_id;
    *game_found_sug = s;
    return 1;
}

int consume_line()
{
    char input_unit[40];
    char* loaded_line = 0;
    size_t loaded_line_len = 0;

    // This let us check if we didn't insert the same player twice.
    int already_inserted_players[MAX_PLAYERS + 2];
    memset((void*)(already_inserted_players), 0, sizeof(int) * (MAX_PLAYERS + 2));

    if (getline(&loaded_line, &loaded_line_len, g_input_file) != EOF)
    {
        // We have to take a lock, becuse we push into the shared queue.
        P(loaded_memory->suggestions_queue_mutex);
        LOG("Player %d makes a suggestion:\n", g_player_id);

        int first_val = 1;
        char* line_ptr = loaded_line;

# if 1
        suggestion s = { };
        s.inv_player_idx = 0;
        for (char i = 'A'; i <= 'Z'; ++i)
            s.inv_types['A' - i] = 0;
# endif

        while(sscanf(line_ptr, "%s", input_unit) != EOF)
        {
            if(first_val)
            {
                first_val = 0;
                s.room_type = input_unit[0];
            }
            else
            {
                if ('A' <= input_unit[0] && input_unit[0] <= 'Z')
                    s.inv_types[input_unit[0] - 'A']++;
                else
                {
                    int invited_user_id = atoi(input_unit);
                    if(!already_inserted_players[invited_user_id])
                        s.inv_players[s.inv_player_idx++] = invited_user_id;
                    already_inserted_players[invited_user_id] = 1;
                }
            }
            line_ptr += strlen(input_unit) + 1;
        }
        s.author = g_player_id;

        LOG("Given suggestion(author = %d, type = %c): Players: { ", g_player_id, s.room_type);
        for(size_t i = 0; i < s.inv_player_idx; ++i)
            LOG("%d ", s.inv_players[i]);
        LOG("} Types: { ");
        for(size_t i = 0; i  < 'Z' - 'A' + 1; ++i)
            if(s.inv_types[i])
                LOG("%c -> %d", (char)(i + 'A'), s.inv_types[i]);
        LOG("} \n");

        if(!check_if_suggestion_possible(&s))
        {
            LOG("&&& Suggestion of player %d is impossible!\n", g_player_id);
            V(loaded_memory->suggestions_queue_mutex);
            return -1;
        }

        suggestion_queue_push(&loaded_memory->suggestions, s);

        LOG("Added suggestion: Players: { ");
        for(size_t i = 0; i < s.inv_player_idx; ++i)
            LOG("%d ", s.inv_players[i]);
        LOG("} Types: { ");
        for(size_t i = 0; i  < 'Z' - 'A' + 1; ++i)
            if(s.inv_types[i])
                LOG("%c ", (char)(i + 'A'));
        LOG("} \n");

        V(loaded_memory->suggestions_queue_mutex);
        return 1;
    }
    else
    {
        LOG("Player %d has nothing more to read\n", g_player_id);
        return 0;
    }
}

void init_game(suggestion* s, int room_id)
{
    int first = 0;
    LOG("+++ Game defined by %d is going to start: room %d, players (", g_player_id, room_id);
    LOG("%d", s->author);
    for(size_t i = 0; i < s->inv_player_idx; ++i)
    {
        LOG(", %d", s->inv_players[i]);
        first = 0;
    }
    for(int i = 0; i < pref_players_vector_idx; ++i)
    {
        LOG(", %d", pref_players_vector[i]);
        first = 0;
    }
    LOG(")\n");

    loaded_memory->room_open[room_id] = 0;
    loaded_memory->available_games[room_id].size = 0;
    loaded_memory->available_games[room_id].desired_size = 0;
    loaded_memory->available_games[room_id].players_vector_idx = 0;
    LOG("Game in room %d initalized!\n", room_id);
}

// TODO: rename, as we do not only check.
void check_for_the_game()
{
    P(loaded_memory->main_mutex);
    {
        // TODO: Add enums for these.
        int consume_line_retval;
        do
        {
            consume_line_retval = consume_line();
            if (consume_line_retval == 1)
                LOG("Player %d added his suggestion.\n", g_player_id);
            else if (consume_line_retval == 0)
                LOG("Player %d has no more suggestions.\n", g_player_id);
        } while(consume_line_retval == -1);

        int game_found = 0;
        int create_game_room_id = 0;
        suggestion* game_found_sug = 0;

        loaded_memory->player_awaits[g_player_id] = 1;
        LOG("%d awaits = %d\n", g_player_id, loaded_memory->player_awaits[g_player_id]);

        for(size_t k = loaded_memory->suggestions.tail_idx;
            k != loaded_memory->suggestions.head_idx;)
        {
            suggestion* ptr = loaded_memory->suggestions.buffer + k;
            LOG("Player %d checks suggestion", g_player_id);
            LOG("(author: %d, room type: %c): Players: { ", ptr->author, ptr->room_type);
            for(size_t i = 0; i < ptr->inv_player_idx; ++i)
                LOG("%d ", ptr->inv_players[i]);
            LOG("} Types: { ");
            for(size_t i = 0; i  < 'Z' - 'A' + 1; ++i)
                if(ptr->inv_types[i])
                    LOG("%c ", (char)(i + 'A'));
            LOG("}\n");

            if (check_for_suggestion_right_now(ptr, &create_game_room_id, &game_found_sug))
            {
                game_found = 1;
                break;
            }
            else
                LOG("--> This suggestion is not possible right now!\n");

            ++k;
            if(k >= MAX_PLAYERS)
                k = 0;
        }

        if (game_found)
        {
            assert(game_found_sug);
            LOG("==> Game found. Suggestion: ");
            LOG("(author: %d, room type: %c): Players: { ", game_found_sug->author, game_found_sug->room_type);
            for(size_t i = 0; i < game_found_sug->inv_player_idx; ++i)
                LOG("%d ", game_found_sug->inv_players[i]);
            LOG("} Types: { ");
            for(size_t i = 0; i  < 'Z' - 'A' + 1; ++i)
                if(game_found_sug->inv_types[i])
                    LOG("%c -> %d", (char)(i + 'A'), game_found_sug->inv_types[i]);
            LOG("} ");
            LOG("Implicitly asked: { ");
            for(int i = 0; i < pref_players_vector_idx; ++i)
                LOG("%d ", pref_players_vector[i]);
            LOG("}\n");

            // Player is not waiting any more.
            loaded_memory->player_awaits[g_player_id]--;

            init_game(game_found_sug, create_game_room_id);

            // TODO: Somehow remove the suggestion from the queue.
            // TODO: Tell other players about the game, and wake them if they sleep.
            // TODO: Release the mutex.
        }
        else
        {
            LOG("Could not start any game. Sleeping...\n");
            V(loaded_memory->main_mutex);
            P(loaded_memory->awaiting_players[g_player_id]);
        }
    }
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        // If we don't know out id we are pretty match finished.
        errno = EINVAL;
        syserr("To little arguments, pass Id of the player.");
    }
    g_player_id = atoi(argv[1]);

    loaded_memory = (mem_data*)load_shared_mem(0);
    // print_table((char*)(loaded_memory + 0), SHM_BUFF_SIZE);

    // TODO: DONT GRAB MUTEX HERE, ITS FOR TESTING!
    P(loaded_memory->DEBUG_MUTEX);
    {
        LOG("I am a player, my pid = %d, my id = %d\t"
            "players: %d, rooms: %d\n",
            getpid(), g_player_id,
            loaded_memory->num_players, loaded_memory->num_rooms);

        initialize_input();

        P(loaded_memory->type_members_mutex);
        loaded_memory->type_members[(int)g_player_type - 'A']++;
        loaded_memory->player_prefs[g_player_id] = g_player_type;
        V(loaded_memory->type_members_mutex);
    }
    V(loaded_memory->DEBUG_MUTEX);
    // TODO: More initialization here?

    barrier_await(&loaded_memory->init_barrier);
    LOG("%d have gone though the barrier\n", g_player_id);

    check_for_the_game();

    P(loaded_memory->player_exit_semaphore);

    // Now we can saftely fill the info about our game, then notify the overlord
    // that we've left so that he can print info about how many games we've
    // played and then post player_exit_semaphore semaphore so that next player
    // can leave the game.
    memcpy(loaded_memory->leaving_player_name,
           argv[1],
           strlen(argv[1]) + 1);

    loaded_memory->leaving_player_score = 20 - (argv[1][0] - '0');
    V(loaded_memory->manager_exit_semaphore);

    close_shared_mem((char*)loaded_memory, 0);
    LOG("%s Done!\n", argv[1]);
    return 0;
}
