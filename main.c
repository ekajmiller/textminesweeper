#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* For initial seed */
#include <string.h>

#define ARRAY_SIZE(_ARR) (sizeof(_ARR) / sizeof((_ARR)[0]))

#define TBS_ROWS 26
#define TBS_COLS 26
#define TBS_NUM_INDICES (TBS_ROWS * TBS_COLS)
#define TBS_NUM_MINES (TBS_NUM_INDICES / 16)

typedef struct {
    int is_mine; /* We could make minesweeper more fun with
                    an enum and different types of items */
    int visible;
    int proximity; /* How many mines are adjacent? */
} tbs_space_t;

typedef enum {
    TBS_TOPLEFT,
    TBS_TOP,
    TBS_TOPRIGHT,
    TBS_RIGHT,
    TBS_BOTRIGHT,
    TBS_BOT,
    TBS_BOTLEFT,
    TBS_LEFT,
} tbs_relpos_t;

typedef struct {
    tbs_relpos_t relpos;
    int relrow;
    int relcol;
} tbs_relpos_def_t;

static tbs_space_t tbs_board[TBS_ROWS][TBS_COLS];

static tbs_relpos_def_t tbs_relpos_defs[] = {
    { TBS_TOPLEFT,  -1, -1, },
    { TBS_TOP,      -1,  0, },
    { TBS_TOPRIGHT, -1,  1, },
    { TBS_RIGHT,     0,  1, },
    { TBS_BOTRIGHT,  1,  1, },
    { TBS_BOT,       1,  0, },
    { TBS_BOTLEFT,   1, -1, },
    { TBS_LEFT,      0, -1, },
};

static int tbs_totalnonminehidden;

static int tbs_quit = 0;


static int tbs_idx2row(int idx)
{
    return idx / TBS_COLS;
}

static int tbs_idx2col(int idx)
{
    return idx % TBS_COLS;
}

static int tbs_rowcol2idx(int row, int col)
{
    if (row < 0 || row >= TBS_ROWS || col < 0 || col >= TBS_COLS)
        return -1;
    return row * TBS_COLS + col;
}

static int tbs_relrowcol2idx(int row, int col, tbs_relpos_t relpos)
{
    /* TODO: sanity check relpos */
    tbs_relpos_def_t *rpd = &tbs_relpos_defs[relpos];
    return tbs_rowcol2idx(row + rpd->relrow, col + rpd->relcol);
}

static void tbs_populate_board()
{
    int ridx, row, col;
    int randomizer[TBS_NUM_INDICES];
    int minecount;
    tbs_relpos_t relpos;

    memset(tbs_board, 0, sizeof(tbs_board));

    /* Make a list of all of the indices for mine randomization purposes */
    for (ridx = 0; ridx < TBS_NUM_INDICES; ridx++) {
        randomizer[ridx] = ridx;
    }

    /* Random place mines with help of randomizer list */
    for (minecount = 0; minecount < TBS_NUM_MINES; minecount++) {

        /* Pick random index of randomizer list */
        /* FIXME: correct modulo bias */
        ridx = rand() % (TBS_NUM_INDICES - minecount);
        row = tbs_idx2row(randomizer[ridx]);
        col = tbs_idx2col(randomizer[ridx]);

        /* Make it a mine! */
        tbs_board[row][col].is_mine = 1;

        /* Increment the adjacency counts */
        for (relpos = 0; relpos < ARRAY_SIZE(tbs_relpos_defs); relpos++) {
            int relidx = tbs_relrowcol2idx(row, col, relpos);

            if (relidx < 0) /* Don't go offboard */
                continue;

            ((tbs_space_t*)tbs_board)[relidx].proximity++;
        }

        /* Remove index from list so it won't be randomized again */
        randomizer[ridx] = randomizer[TBS_NUM_INDICES - minecount - 1];
    }
    tbs_totalnonminehidden = TBS_NUM_INDICES - TBS_NUM_MINES;
}

static void tbs_draw_board(int xray)
{
    int row, col;
    tbs_space_t *space;

    printf("  ");
    for (col = 0; col < TBS_COLS; col++) {
        printf("%c ", col + 'A');
    }
    printf("\n");

    for (row = 0; row < TBS_ROWS; row++) {
        printf("%c ", row + 'a');
        for (col = 0; col < TBS_COLS; col++) {
            space = &tbs_board[row][col];
            if (space->visible || xray) {
                if (!space->is_mine) {
                    if (space->proximity) {
                        printf("%d", space->proximity);
                    } else {
                        printf(" ");
                    }
                } else {
                    printf("*");
                }
            } else {
                printf("-");
            }
            printf(" ");
        }
        printf("%c ", row + 'a');
        printf("\n");
    }

    printf("  ");
    for (col = 0; col < TBS_COLS; col++) {
        printf("%c ", col + 'A');
    }
    printf("\n");
}

/* Recursively auto selects all spaces surrounding 0s.  */
static void tbs_auto_select(int row, int col)
{
    int idx;
    int relpos;
    tbs_space_t *space;

    idx = tbs_rowcol2idx(row, col);

    /* If invalid space, stop recursing! */
    if (idx < 0)
        return;

    space = &((tbs_space_t*)tbs_board)[idx];

    /* If already picked, stop recursing! */
    if (space->visible)
        return;

    space->visible = 1;
    tbs_totalnonminehidden--;

    if (space->proximity != 0) /* Stop recursing if proximity 0 */
        return;

    /* Recurse into surrounding areas if proximity is 0 */
    for (relpos = 0; relpos < ARRAY_SIZE(tbs_relpos_defs); relpos++) {
        tbs_auto_select(row + tbs_relpos_defs[relpos].relrow,
                       col + tbs_relpos_defs[relpos].relcol);
    }
}

/* Returns -1 for invalid selection, -2 for mine, -3 for already selected; else proximity */
static int tbs_select(int row, int col)
{
    int idx;
    tbs_space_t *space;

    idx = tbs_rowcol2idx(row, col);
    if (idx < 0)
        return idx;

    space = &((tbs_space_t*)tbs_board)[idx];

    if (space->is_mine)
        return -2;
    if (space->visible)
        return -3;

    tbs_auto_select(row, col);

    return space->proximity;
}

static void tbs_get_input(char *input, int sz)
{
    char ch;

    fgets(input, sz, stdin);
    if (input[strlen(input)-1] != '\n') {
        while (((ch = getchar()) != '\n') && (ch != EOF));
    } else {
        input[strlen(input)-1] = '\0';
    }
}

int main(int argc, char const *argv[])
{
    int row, col;
    int ii;
    int proximity;

    srand(time(NULL));
    tbs_populate_board();

    printf("Welcome to Text Based Sweeper (TBS)\n\n");

    /* TODO: add signal handler that sets tbs_quit to true */
    while (!tbs_quit) {
        struct {
            const char *label;
            int *fill;
        } inputs[] = {
            { "row", &row },
            { "col", &col },
        };

        tbs_draw_board(0);
        printf("\n");

        for (ii = 0; ii < ARRAY_SIZE(inputs); ii++) {
            char usr_str[10];
            printf("%s? ", inputs[ii].label);
            fflush(stdout);
            /* TODO: perhaps combine inputs into one and make it to where
                     you don't have to press enter. */
            tbs_get_input(usr_str, sizeof(usr_str));
            /* TODO: make more robust. Check for out of bounds and invalid
                     chars, auto lowercase, etc */
            *(inputs[ii].fill) = usr_str[0] - 'a';
        }

        proximity = tbs_select(row, col);
        if (proximity == -1) {
            printf("Invalid selection. Choose again.\n\n");
            continue;
        }

        /* Check lose condition */
        if (proximity == -2) {
            printf("BOOM! Game Over!\n\n");
            tbs_draw_board(1);
            break;
        }

        /* Check lose condition */
        if (proximity == -3) {
            printf("Already cleared. Choose again.\n\n");
            continue;
        }

        /* Check win condition */
        if (tbs_totalnonminehidden <= 0) {
            printf("You win!!! Congrats!\n\n");
            tbs_draw_board(1);
            break;
        }

        printf("\n");
    }

    return 0;
}
