#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* For initial seed */
#include <string.h>

#define ARRAY_SIZE(_ARR) (sizeof(_ARR) / sizeof((_ARR)[0]))

#define MS_ROWS 26
#define MS_COLS 26
#define MS_NUM_INDICES (MS_ROWS * MS_COLS)
#define MS_NUM_MINES (MS_NUM_INDICES / 8)

typedef struct {
    int is_mine; /* We could make minesweeper more fun with
                    an enum and different types of items */
    int visible;
    int proximity; /* How many mines are adjacent? */
} ms_space_t;

typedef enum {
    MS_TOPLEFT,
    MS_TOP,
    MS_TOPRIGHT,
    MS_RIGHT,
    MS_BOTRIGHT,
    MS_BOT,
    MS_BOTLEFT,
    MS_LEFT,
} ms_relpos_t;

typedef struct {
    ms_relpos_t relpos;
    int relrow;
    int relcol;
} ms_relpos_def_t;

static ms_space_t ms_board[MS_ROWS][MS_COLS];

static ms_relpos_def_t ms_relpos_defs[] = {
    { MS_TOPLEFT,  -1, -1, },
    { MS_TOP,      -1,  0, },
    { MS_TOPRIGHT, -1,  1, },
    { MS_RIGHT,     0,  1, },
    { MS_BOTRIGHT,  1,  1, },
    { MS_BOT,       1,  0, },
    { MS_BOTLEFT,   1, -1, },
    { MS_LEFT,      0, -1, },
};

static int ms_totalnonminehidden;

static int ms_quit = 0;


static int ms_idx2row(int idx)
{
    return idx / MS_COLS;
}

static int ms_idx2col(int idx)
{
    return idx % MS_COLS;
}

static int ms_rowcol2idx(int row, int col)
{
    if (row < 0 || row >= MS_ROWS || col < 0 || col >= MS_COLS)
        return -1;
    return row * MS_COLS + col;
}

static int ms_relrowcol2idx(int row, int col, ms_relpos_t relpos)
{
    /* TODO: sanity check relpos */
    ms_relpos_def_t *rpd = &ms_relpos_defs[relpos];
    return ms_rowcol2idx(row + rpd->relrow, col + rpd->relcol);
}

static void ms_populate_board()
{
    int ridx, row, col;
    int randomizer[MS_NUM_INDICES];
    int minecount;
    ms_relpos_t relpos;

    memset(ms_board, 0, sizeof(ms_board));

    /* Make a list of all of the indices for mine randomization purposes */
    for (ridx = 0; ridx < MS_NUM_INDICES; ridx++) {
        randomizer[ridx] = ridx;
    }

    /* Random place mines with help of randomizer list */
    for (minecount = 0; minecount < MS_NUM_MINES; minecount++) {

        /* Pick random index of randomizer list */
        /* FIXME: correct modulo bias */
        ridx = rand() % (MS_NUM_INDICES - minecount);
        row = ms_idx2row(randomizer[ridx]);
        col = ms_idx2col(randomizer[ridx]);

        /* Make it a mine! */
        ms_board[row][col].is_mine = 1;

        /* Increment the adjacency counts */
        for (relpos = 0; relpos < ARRAY_SIZE(ms_relpos_defs); relpos++) {
            int relidx = ms_relrowcol2idx(row, col, relpos);

            if (relidx < 0) /* Don't go offboard */
                continue;

            ((ms_space_t*)ms_board)[relidx].proximity++;
        }

        /* Remove index from list so it won't be randomized again */
        randomizer[ridx] = randomizer[MS_NUM_INDICES - minecount - 1];
    }
    ms_totalnonminehidden = MS_NUM_INDICES - MS_NUM_MINES;
}

static void ms_draw_board(int xray)
{
    int row, col;
    ms_space_t *space;

    printf("  ");
    for (col = 0; col < MS_COLS; col++) {
        printf("%c ", col + 'A');
    }
    printf("\n");

    for (row = 0; row < MS_ROWS; row++) {
        printf("%c ", row + 'a');
        for (col = 0; col < MS_COLS; col++) {
            space = &ms_board[row][col];
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
    for (col = 0; col < MS_COLS; col++) {
        printf("%c ", col + 'A');
    }
    printf("\n");
}

/* Recursively auto selects all spaces surrounding 0s.  */
static void ms_auto_select(int row, int col)
{
    int idx;
    int relpos;
    ms_space_t *space;

    idx = ms_rowcol2idx(row, col);

    /* If invalid space, stop recursing! */
    if (idx < 0)
        return;

    space = &((ms_space_t*)ms_board)[idx];

    /* If already picked, stop recursing! */
    if (space->visible)
        return;

    space->visible = 1;
    ms_totalnonminehidden--;

    if (space->proximity != 0) /* Stop recursing if proximity 0 */
        return;

    /* Recurse into surrounding areas if proximity is 0 */
    for (relpos = 0; relpos < ARRAY_SIZE(ms_relpos_defs); relpos++) {
        ms_auto_select(row + ms_relpos_defs[relpos].relrow,
                       col + ms_relpos_defs[relpos].relcol);
    }
}

/* Returns -1 for invalid selection, -2 for mine, -3 for already selected; else proximity */
static int ms_select(int row, int col)
{
    int idx;
    ms_space_t *space;

    idx = ms_rowcol2idx(row, col);
    if (idx < 0)
        return idx;

    space = &((ms_space_t*)ms_board)[idx];

    if (space->is_mine)
        return -2;
    if (space->visible)
        return -3;

    ms_auto_select(row, col);

    return space->proximity;
}

static void ms_get_input(char *input, int sz)
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
    ms_populate_board();

    printf("Welcome to Text Based Sweeper (TBS)\n\n");

    /* TODO: add signal handler that sets ms_quit to true */
    while (!ms_quit) {
        struct {
            const char *label;
            int *fill;
        } inputs[] = {
            { "row", &row },
            { "col", &col },
        };

        ms_draw_board(0);
        printf("\n");

        for (ii = 0; ii < ARRAY_SIZE(inputs); ii++) {
            char usr_str[10];
            printf("%s? ", inputs[ii].label);
            fflush(stdout);
            /* TODO: perhaps combine inputs into one and make it to where
                     you don't have to press enter. */
            ms_get_input(usr_str, sizeof(usr_str));
            /* TODO: make more robust. Check for out of bounds and invalid
                     chars, auto lowercase, etc */
            *(inputs[ii].fill) = usr_str[0] - 'a';
        }

        proximity = ms_select(row, col);
        if (proximity == -1) {
            printf("Invalid selection. Choose again.\n\n");
            continue;
        }

        /* Check lose condition */
        if (proximity == -2) {
            printf("BOOM! Game Over!\n\n");
            ms_draw_board(1);
            break;
        }

        /* Check lose condition */
        if (proximity == -3) {
            printf("Already cleared. Choose again.\n\n");
            continue;
        }

        /* Check win condition */
        if (ms_totalnonminehidden <= 0) {
            printf("You win!!! Congrats!\n\n");
            ms_draw_board(1);
            break;
        }

        printf("\n");
    }

    return 0;
}
