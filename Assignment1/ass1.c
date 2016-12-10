#include <stdio.h>
#include <stdlib.h>

/* Global Variables */
/* the height of the cells */
int height;
/* the width of the cells */
int width;
/* the number of players */
int playerCount;
/* current player */
int player = 1;
/* the 2D array that stores cell status */
int **cell;
/* the 2D array that stores horizontal grid status */
int **gridH;
/* the 2D array that stores vertical grid status */
int **gridV;
/* the array that stores score of the game
* score[0] is the total number of closed cells
* score[i] is the score of player i
*/
int *score;


/* the struct that saves current command */
typedef struct {
    int x;
    int y;
    char direction;
} Command;

/* takes in error code, then print stderr message and exit program */
void error(int errorCode) {
    switch (errorCode) {
        case 1:
            fprintf(stderr,
                    "Usage: boxes height width playercount [filename]\n");
            exit(1);
            break;
        case 2:
            fprintf(stderr, "Invalid grid dimensions\n");
            exit(2);
            break;
        case 3:
            fprintf(stderr, "Invalid player count\n");
            exit(3);
            break;
        case 4:
            fprintf(stderr, "Invalid grid file\n");
            exit(4);
            break;
        case 5:
            fprintf(stderr, "Error reading grid contents\n");
            exit(5);
            break;
        case 6:
            fprintf(stderr, "End of user input\n");
            exit(6);
            break;
        case 9:
            fprintf(stderr, "System call failure\n");
            exit(9);
            break;
    }
}

/* check if memory is successful allocated (for (int *) type) */
void malloc_check_1(int *p) {
    if (p == NULL) {
        error(9);
    }
}
/* check if memory is successful allocated (for (int **) type) */
void malloc_check_2(int **p) {
    if (p == NULL) {
        error(9);
    }
}
/* check if memory is successful allocated (for (char *) type) */
void malloc_check_3(char *p) {
    if (p == NULL) {
        error(9);
    }
}
/* check if memory is successful allocated (for (char **) type) */
void malloc_check_4(char **p) {
    if (p == NULL) {
        error(9);
    }
}

/* read and check a given file 
 * NOTE: Reading and verifying file needs some consecutive steps that
 * modify several variables, and they relate to each other.
 * I tried to break them into small functions, but this only reduce
 * readability and efficiency. Thus I kept them in a single function.
 * Each step is separated with blank line and I described what they are
 * doing with comments.
 */
void game_load(char *argv[]) {
    FILE *in = fopen(argv[4], "r");
    int i, j, k, lineNum, len, justComma;
    char ch, lastCh;
    int *lineLen;
    char **data;
    lineNum = 0;
    if (in == NULL) {
        error(4);
    }
    /* check file line number */
    while ((ch = fgetc(in)) != EOF) {
        lastCh = ch;
        if (ch == '\n') {
            lineNum++;
        }
    }
    /* (in case the last line of a valid file doesn't have '\0') */
    if (lastCh != '\n') {
        lineNum++;
    }
    if (lineNum < 2 + 3 * height) {
        error(5);
    }
    /* (in case the file has more lines 
     * (this is not invalid according to spec)) */
    lineNum = 2 + 3 * height;

    /* get length of each line */
    lineLen = (int *)malloc(sizeof(int) * (lineNum + 1));
    malloc_check_1(lineLen);
    for (i = 0; i < lineNum; i++) {
        lineLen[i] = 0;
    }
    i = 0;
    len = 0;
    rewind(in);
    while ((ch = fgetc(in)) != EOF) {
        if (ch == '\n') {
            if (i < lineNum) {
                lineLen[i] = len;
            }
            len = 0;
            i++;
        } else {
            len++;
        }
    }
    if (i == (lineNum - 1) && len != 0) {
        lineLen[i] = len;
    }

    /* check length of the lines that contains grid data */
    for (i = 0; i < lineNum; i++) {
        if (i >= 1 && i <= (lineNum - height - 1)) {
            if (i % 2 == 1 && lineLen[i] != width) {
                error(5);
            }
            if (i % 2 == 0 && lineLen[i] != (width + 1)) {
                error(5);
            }
        }
    }

    /* initiate 2d array to store file content */
    data = (char**)malloc(sizeof(char *) * lineNum);
    malloc_check_4(data);
    for (i = 0; i < lineNum; i++) {
        data[i] = (char *)malloc(sizeof(char) * (lineLen[i] + 1));
        malloc_check_3(data[i]);
        for (j = 0; j < lineLen[i] + 1; j++) {
            data[i][j] = '\0';
        }
    }

    /* put openned file in to the 2d array */
    rewind(in);
    i = 0;
    j = 0;
    while ((ch = fgetc(in)) != EOF) {
        if (ch == '\n') {
            if (i < lineNum) {
                data[i][j] = '\0';
            }
            j = 0;
            i++;
        } else {
            if (i == 0) {
                if (ch >= '0' && ch <= '9') {
                    data[i][j] = ch;
                    j++;
                } else {
                    error(5);
                }
            } else if (i < lineNum - height) {
                if (ch == '0' || ch == '1') {
                    data[i][j] = ch;
                    j++;
                } else {
                    error(5);
                }
            } else if (i >= lineNum - height && i < lineNum) {
                if ((ch >= '0' && ch <= '9') || ch == ',') {
                    data[i][j] = ch;
                    j++;
                } else {
                    error(5);
                }
            }
        }
        if (i == (lineNum - 1) && j != 0) {
            data[i][j] = '\0';
        }
    }

    /* get next player number, 
     * and check if it is outside the total number of players 
     */
    if (atoi(data[0]) <= playerCount) {
        player = atoi(data[0]);
    } else {
        error(5);
    }

    /* get grid status */
    for (i = 0; i < height + 1; i++) {
        for (j = 0; j < width; j++) {
            gridH[i][j] = data[2 * i + 1][j] - '0';
        }
    }
    for (i = 0; i < height; i++) {
        for (j = 0; j < width + 1; j++) {
            gridV[i][j] = data[2 * i + 2][j] - '0';
        }
    }

    /* check if data of cells are valid
     * (no consecutive commas, no leading and ending commas)
     */
    justComma = 0;
    for (i = lineNum - height; i < lineNum; i++) {
        k = 0;
        if (data[i][0] == ',' || data[i][lineLen[i] - 1] == ',') {
            error(5);
        }
        for (j = 0; j < lineLen[i]; j++) {
            if (data[i][j] == ',') {
                if (justComma == 1) {
                    error(5);
                }
                justComma = 1;
                k++;
            } else {
                justComma = 0;
            }
        }
        if (k != width - 1) {
            error(5);
        }
    }

    /* convert cell data to 2d int array */
    for (i = 0; i < height; i++) {
        k = 0;
        for (j = 0; j < lineLen[lineNum - height + i]; j++) {
            if (data[lineNum - height + i][j] == ',') {
                if (cell[i][k] > playerCount) {
                    error(5);
                }
                k++;
            } else {
                cell[i][k] = 10 * cell[i][k] +
                        (data[lineNum - height + i][j] - '0');
            }
        }
        if (cell[i][k] > playerCount) {
            error(5);
        }
    }

    /* check if the grid data is consistent with the cell data */
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (cell[i][j] == 0) {
                if (gridH[i][j] == 1 && gridH[i + 1][j] == 1 &&
                        gridV[i][j] == 1 && gridV[i][j + 1] == 1) {
                    error(5);
                }
            } else {
                if (gridH[i][j] == 0 || gridH[i + 1][j] == 0 ||
                        gridV[i][j] == 0 || gridV[i][j + 1] == 0) {
                    error(5);
                }
            }
        }
    }

    /* calculate the number of closed cells and each player's score */
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (cell[i][j] != 0) {
                score[cell[i][j]]++;
                score[0]++;
            }
        }
    }
    fclose(in);
    free(lineLen);
    for (i = 0; i < lineNum; i++) {
        free(data[i]);
    }
    free(data);
}

/* check if arguments are valid */
void arg_check(char *argv[]) {
    int i;
    for (i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            error(2);
        }
    }
    for (i = 0; argv[2][i] != '\0'; i++) {
        if (argv[2][i] < '0' || argv[2][i] > '9') {
            error(2);
        }
    }
    for (i = 0; argv[3][i] != '\0'; i++) {
        if (argv[3][i] < '0' || argv[3][i] > '9') {
            error(3);
        }
    }
}

/* get user input and store it in *input. input length is saved in strLen */
void input_str(int *strLen, char *input) {
    char ch;
    int i = 0;
    while ((ch = getchar())) {
        if (ch == EOF) {
            error(6);
        } else if (ch == '\n') {
            if (i <= 30) {
                input[i] = '\0';
            } else {
                input[30] = '\0';
            }
            break;
        } else if (i < 30) {
            input[i] = ch;
        }
        i++;
    }
    *strLen = i;
}

/* check user command input.
 * return 0 if the input is invalid,
 * return 1 if it is a saving command, return 2 if it is a valid move
 * NOTE: This function is a little bit long, as it has some tiny check.
 */
int input_check(Command *com, char *saveName) {
    int i = 0, spaceIndex = -1, xValue = 0, yValue = 0, strLen;
    char input[31] = "";
    input_str(&strLen, input);
    if (strLen > 30 || strLen < 3) {
        return 0;
    } else {
        if (input[0] == ' ' || input[strLen - 1] == ' ') {
            return 0;
        } else if (input[0] == 'w' && input[1] == ' ') {
            for (i = 2; i < strLen; i++) {
                *saveName = input[i];
                saveName++;
            }
            *saveName = '\0';
            return 1;
        } else if (input[0] >= '0' && input[0] <= '9' &&
                input[strLen - 2] == ' ' &&
                (input[strLen - 1] == 'v' || input[strLen - 1] == 'h')) {
            for (i = 0; i < strLen - 2; i++) {
                if ((input[i] >= '0' && input[i] <= '9') || input[i] == ' ') {
                    if (input[i] == ' ') {
                        if (spaceIndex == -1) {
                            spaceIndex = i;
                        } else {
                            return 0;
                        }
                    }
                } else {
                    return 0;
                }
            }
            /* check if input has leading 0 */
            if (input[0] == '0' && spaceIndex != 1) {
                return 0;
            }
            if (input[spaceIndex + 1] == '0' &&
                    (strLen - 2 - spaceIndex != 2)) {
                return 0;
            }
            for (i = 0; i < spaceIndex; i++) {
                xValue = 10 * xValue + (input[i] - '0');
            }
            for (i = spaceIndex + 1; i < strLen - 2; i++) {
                yValue = 10 * yValue + (input[i] - '0');
            }
            com->x = xValue;
            com->y = yValue;
            com->direction = input[strLen - 1];
            return 2;
        } else {
            return 0;
        }
    }
    return 0;
}


/* check if a command is valid
* (x&y are within valid range, and the modifing grid is empty).
* if yes: return 1; else: return 0
*/
int command_check(Command com) {
    if (com.direction == 'h' && (com.x) <= height && (com.y) < width &&
            (com.x) >= 0 && (com.y) >= 0) {
        if (gridH[com.x][com.y] == 0) {
            return 1;
        } else {
            return 0;
        }
    } else if (com.direction == 'v' && (com.x) < height && (com.y) <= width &&
            (com.x) >= 0 && (com.y) >= 0) {
        if (gridV[com.x][com.y] == 0) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}


/* update the grid and cell */
void grid_update(Command com) {
    if (com.direction == 'h') {
        gridH[com.x][com.y] = 1;
    } else {
        gridV[com.x][com.y] = 1;
    }
}


/* check if a cell is closed. if yes: return 1; if not: return 0 */
int cell_check(int x, int y) {
    if (cell[x][y] == 0) {
        if (gridH[x][y] == 1 && gridH[x + 1][y] == 1 && gridV[x][y] == 1 &&
                gridV[x][y + 1] == 1) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

/* change cell status to player's number (if the cell is closed)
 * and update score 
 */
void score_update(int x, int y) {
    cell[x][y] = player;
    score[player]++;
    score[0]++;
}

/* check and update cells that is related to current command 
 * NOTE: Although I already put check method into other functions, 
 * This function is a little bit long, as it has to check different
 * cells according to user's command. Sorry about that.
 */
int cell_update(Command com) {
    int changed;
    if (com.direction == 'h') {
        if (com.x == 0) {
            if (cell_check(com.x, com.y)) {
                score_update(com.x, com.y);
                return 1;
            } else {
                return 0;
            }
        } else if (com.x == height) {
            if (cell_check(com.x - 1, com.y)) {
                score_update(com.x - 1, com.y);
                return 1;
            } else {
                return 0;
            }
        } else {
            changed = 0;
            if (cell_check(com.x, com.y)) {
                score_update(com.x, com.y);
                changed = 1;
            }
            if (cell_check(com.x - 1, com.y)) {
                score_update(com.x - 1, com.y);
                changed = 1;
            }
            return changed;
        }
    } else if (com.direction == 'v') {
        if (com.y == 0) {
            if (cell_check(com.x, com.y)) {
                score_update(com.x, com.y);
                return 1;
            } else {
                return 0;
            }
        } else if (com.y == width) {
            if (cell_check(com.x, com.y - 1)) {
                score_update(com.x, com.y - 1);
                return 1;
            } else {
                return 0;
            }
        } else {
            changed = 0;
            if (cell_check(com.x, com.y - 1)) {
                score_update(com.x, com.y - 1);
                changed = 1;
            }
            if (cell_check(com.x, com.y)) {
                score_update(com.x, com.y);
                changed = 1;
            }
            return changed;
        }
    }
    return 0;
}


/* display the grids and cells */
void grid_display() {
    int i, j;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            printf("+");
            if (gridH[i][j] == 1) {
                printf("-");
            } else {
                printf(" ");
            }
        }
        printf("+\n");
        for (j = 0; j < width; j++) {
            if (gridV[i][j] == 1) {
                printf("|");
            } else {
                printf(" ");
            }
            if (cell[i][j] != 0) {
                printf("%c", 'A' - 1 + cell[i][j]);
            } else {
                printf(" ");
            }
        }
        if (gridV[i][width] == 1) {
            printf("|\n");
        } else {
            printf(" \n");
        }
    }
    for (j = 0; j < width; j++) {
        printf("+");
        if (gridH[height][j] == 1) {
            printf("-");
        } else {
            printf(" ");
        }
    }
    printf("+\n");
}


/* change current player to the next */
void player_change() {
    if (player == playerCount) {
        player = 1;
    } else {
        (player)++;
    }
}


/* check if the game is finished */
void gameover_check() {
    if (score[0] == height * width) {
        int max, i;
        max = 0;
        for (i = 1; i <= playerCount; i++) {
            if (score[i] > max) {
                max = score[i];
            }
        }
        printf("Winner(s): ");
        i = 1;
        while (score[i] != max) {
            i++;
        }
        /* print the first winner */
        printf("%c", 'A' - 1 + i);
        /* print the rest, with comma and space at the front */
        for (i++; i <= playerCount; i++) {
            if (score[i] == max) {
                printf(", %c", 'A' - 1 + i);
            }
        }
        printf("\n");
        exit(0);
    }
}


/* save the game */
void game_save(char saveName[]) {
    FILE *out = fopen(saveName, "w");
    int i, j;
    if (out == NULL) {
        fprintf(stderr, "Can not open file for write\n");
    } else {
        fprintf(out, "%d\n", player);

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                fprintf(out, "%d", gridH[i][j]);
            }
            fprintf(out, "\n");
            for (j = 0; j <= width; j++) {
                fprintf(out, "%d", gridV[i][j]);
            }
            fprintf(out, "\n");
        }
        for (j = 0; j < width; j++) {
            fprintf(out, "%d", gridH[i][j]);
        }
        fprintf(out, "\n");

        for (i = 0; i < height; i++) {
            for (j = 0; j < width - 1; j++) {
                fprintf(out, "%d,", cell[i][j]);
            }
            fprintf(out, "%d\n", cell[i][j]);
        }
        fclose(out);
        fprintf(stderr, "Save complete\n");
    }
}



/* initiate dynamic global arrays */
void array_init() {
    int i, j;
    /* initiate 2d array for cell status */
    cell = (int **)malloc(sizeof(int *) * height);
    malloc_check_2(cell);
    for (i = 0; i < height; i++) {
        cell[i] = (int *)malloc(sizeof(int) * width);
        malloc_check_1(cell[i]);
        for (j = 0; j < width; j++) {
            cell[i][j] = 0;
        }
    }
    /* initiate 2d array for horizontal grid status */
    gridH = (int **)malloc(sizeof(int *) * (height + 1));
    malloc_check_2(gridH);
    for (i = 0; i < height + 1; i++) {
        gridH[i] = (int *)malloc(sizeof(int) * width);
        malloc_check_1(gridH[i]);
        for (j = 0; j < width; j++) {
            gridH[i][j] = 0;
        }
    }
    /* initiate 2d array for vertical grid status */
    gridV = (int **)malloc(sizeof(int *) * height);
    malloc_check_2(gridV);
    for (i = 0; i < height; i++) {
        gridV[i] = (int *)malloc(sizeof(int) * (width + 1));
        malloc_check_1(gridV[i]);
        for (j = 0; j < width + 1; j++) {
            gridV[i][j] = 0;
        }
    }
    /* initiate the array for scores.*/
    score = (int *)malloc(sizeof(int) * (playerCount + 1));
    malloc_check_1(score);
    for (i = 0; i <= playerCount; i++) {
        score[i] = 0;
    }
}

/* free the memory allocated by malloc */
void malloc_free() {
    int i;
    for (i = 0; i < height; i++) {
        free(cell[i]);
    }
    free(cell);

    for (i = 0; i < height + 1; i++) {
        free(gridH[i]);
    }
    free(gridH);

    for (i = 0; i < height; i++) {
        free(gridV[i]);
    }
    free(gridV);

    free(score);
}


int main(int argc, char *argv[]) {
    char saveName[31] = "";
    Command com = {0, 0, ' '};
    int inputStatus = 0;

    if (argc < 4 || argc > 5) {
        error(1);
    }
    arg_check(argv);
    height = atoi(argv[1]);
    width = atoi(argv[2]);
    playerCount = atoi(argv[3]);
    if (height <= 1 || height >= 1000 || width <= 1 || width >= 1000) {
        error(2);
    }
    if (playerCount <= 1 || playerCount >= 101) {
        error(3);
    }

    array_init();

    if (argc == 5) {
        game_load(argv);
    }

    grid_display();
    while (1) {
        printf("%c> ", 'A' - 1 + player);
        inputStatus = input_check(&com, saveName);
        if (inputStatus == 1) {
            game_save(saveName);
        } else if (inputStatus == 2) {
            if (command_check(com) == 1) {
                grid_update(com);
                if (cell_update(com) == 0) {
                    player_change();
                }
                grid_display();
                gameover_check();
            }
        }
    }
    malloc_free();
    return 0;
}