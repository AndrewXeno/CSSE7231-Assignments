#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

/* takes in error code, then print stderr message and exit program */
void error(int errorCode) {
    switch (errorCode) {
        case 1:
            fprintf(stderr,
                    "Usage: clubhub deckfile winscore prog1 prog2 "\
                    "[prog3 [prog4]]\n");
            exit(1);
            break;
        case 2:
            fprintf(stderr, "Invalid score\n");
            exit(2);
            break;
        case 3:
            fprintf(stderr, "Unable to access deckfile\n");
            exit(3);
            break;
        case 4:
            fprintf(stderr, "Error reading deck\n");
            exit(4);
            break;
        case 5:
            fprintf(stderr, "Unable to start subprocess\n");
            exit(5);
            break;
        case 6:
            fprintf(stderr, "Player quit\n");
            exit(6);
            break;
        case 7:
            fprintf(stderr, "Invalid message received from player\n");
            exit(7);
            break;
        case 8:
            fprintf(stderr, "Invalid play by player\n");
            exit(8);
            break;
        case 9:
            fprintf(stderr, "SIGINT caught\n");
            exit(9);
            break;
    }
}

/* check if arguments are valid, if not, exit */
void check_argu(int argc, char *argv[]) {
    if (argc < 5 || argc > 7) {
        error(1);
    }

    for (int i = 0; argv[2][i] != '\0'; i++) {
        if (argv[2][i] < '0' || argv[2][i] > '9') {
            error(2);
        }
    }
}

/* return the maximum line length of the file */
int get_max_length(FILE *f) {
    int maxLineLength = 0;
    int lineLength = 0;
    char lastCh;
    char ch;
    rewind(f);
    while ((ch = fgetc(f)) != EOF) {
        if (ch == '\n') {
            if (lineLength > maxLineLength) {
                maxLineLength = lineLength;
            }
            lineLength = 0;
        } else {
            lastCh = ch;
            lineLength++;
        }
    }
    if (lastCh != '\n') {
        if (lineLength > maxLineLength) {
            maxLineLength = lineLength;
        }
    }
    return maxLineLength;
}

/* return the number of decks contained in a file */
int get_deck_num(FILE *f, int maxLineLength) {
    int deckNumber = 1;
    char *line = NULL;
    rewind(f);
    line = (char *)malloc(sizeof(char) * (maxLineLength + 1));
    while (fgets(line, maxLineLength + 1, f) != NULL) {
        if (line[0] == '.') {
            if (line[1] == '\n' && line[2] == '\0') {
                deckNumber++;
            } else {
                error(4);
            }
        }
    }
    free(line);
    return deckNumber;
}

/* check if the file contains valid decks */
void check_file_validity(FILE *f, int maxLineLength) {
    char *line = NULL;
    rewind(f);
    line = (char *)malloc(sizeof(char) * (maxLineLength + 1));
    while (fgets(line, maxLineLength + 2, f) != NULL) {
        if (line[0] == '.') {
        } else if (line[0] == '\n') {
        } else if (line[0] == '#') {
        } else {
            if (strlen(line) % 3 == 0 || strlen(line) % 3 == 1) {
                for (int i = 0; (line[i] != '\n') && (line[i] != '\0'); i++) {
                    char c = line[i];
                    if (i % 3 == 0) {
                        if ((c >= '2' && c <= '9') || c == 'T' || c == 'J' ||
                                c == 'Q' || c == 'K' || c == 'A') {
                        } else {
                            error(4);
                        }
                    } else if (i % 3 == 1) {
                        if (c == 'S' || c == 'C' || c == 'D' || c == 'H') {
                        } else {
                            error(4);
                        }
                    } else if (i % 3 == 2) {
                        if (c != ',') {
                            printf("here%s", line);
                            error(4);
                        }
                    }
                }
            } else {

                error(4);
            }
        }
    }
    free(line);
}

/* initialize the empty deck array */
char **decks_init(int deckNumber) {
    int i = 0, j = 0;
    char **decks;
    decks = (char**)malloc(sizeof(char *) * deckNumber);
    for (i = 0; i < deckNumber; i++) {
        decks[i] = (char *)malloc(sizeof(char) * 105);
        for (j = 0; j < 104; j++) {
            decks[i][j] = '\0';
        }
        decks[i][j] = '\0';
    }
    return decks;
}

/* read the deck file and store deck in the deck array */
void read_decks(int deckNumber, char ***decks, FILE *f, int maxLineLength) {
    int currentDeck = 0;
    int currentPosition = 0;
    char *line = NULL;
    rewind(f);
    line = (char *)malloc(sizeof(char) * (maxLineLength + 1));
    while (fgets(line, maxLineLength + 2, f) != NULL) {
        if (line[0] == '.') {
            if (currentPosition != 104) {

                error(4);
            }
            currentDeck++;
            currentPosition = 0;
        } else if (line[0] == '\n') {
        } else if (line[0] == '#') {
        } else {
            for (int i = 0; (line[i] != '\n') && (line[i] != '\0'); i++) {
                if (line[i] != ',') {
                    if (currentPosition >= 104) {

                        error(4);
                    }
                    (*decks)[currentDeck][currentPosition] = line[i];
                    currentPosition++;
                }
            }
        }
    }
    if (currentPosition != 104) {
        error(4);
    }
    free(line);
}

/* open the deck file, check file validity and read the file */
void open_decks_file(char *argv[], int *deckNumberP, char ***decks) {
    FILE *f = fopen(argv[1], "r");
    int maxLineLength = 0;
    if (f == NULL) {
        error(3);
    }
    maxLineLength = get_max_length(f);
    *deckNumberP = get_deck_num(f, maxLineLength);
    int deckNumber = *deckNumberP;
    check_file_validity(f, maxLineLength);
    *decks = decks_init(deckNumber);
    read_decks(deckNumber, decks, f, maxLineLength);
}

/* initializer player's hand card array */
void player_hand_init(char playerHand[4][78]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 78; j++) {
            playerHand[i][j] = '\0';
        }
    }
}

/* convert a char representing a rank to corresponding index*/
int char_to_rank_index(char c) {
    if (c == '2') {
        return 0;
    } else if (c == '3') {
        return 1;
    } else if (c == '4') {
        return 2;
    } else if (c == '5') {
        return 3;
    } else if (c == '6') {
        return 4;
    } else if (c == '7') {
        return 5;
    } else if (c == '8') {
        return 6;
    } else if (c == '9') {
        return 7;
    } else if (c == 'T') {
        return 8;
    } else if (c == 'J') {
        return 9;
    } else if (c == 'Q') {
        return 10;
    } else if (c == 'K') {
        return 11;
    } else if (c == 'A') {
        return 12;
    } else {
        error(5);
    }
    return 0;
}

/* convert a char representing a suit to corresponding index*/
int char_to_suit_index(char c) {
    if (c == 'S') {
        return 0;
    } else if (c == 'C') {
        return 1;
    } else if (c == 'D') {
        return 2;
    } else if (c == 'H') {
        return 3;
    } else {
        error(5);
    }
    return 0;
}

/* convert a rank index to its corresponding char representation */
char rank_index_to_char(int i) {
    if (i == 0) {
        return '2';
    } else if (i == 1) {
        return '3';
    } else if (i == 2) {
        return '4';
    } else if (i == 3) {
        return '5';
    } else if (i == 4) {
        return '6';
    } else if (i == 5) {
        return '7';
    } else if (i == 6) {
        return '8';
    } else if (i == 7) {
        return '9';
    } else if (i == 8) {
        return 'T';
    } else if (i == 9) {
        return 'J';
    } else if (i == 10) {
        return 'Q';
    } else if (i == 11) {
        return 'K';
    } else if (i == 12) {
        return 'A';
    } else {
    }
    return '2';
}

/* convert a suit index to its corresponding char representation */
char suit_index_to_char(int i) {
    if (i == 0) {
        return 'S';
    } else if (i == 1) {
        return 'C';
    } else if (i == 2) {
        return 'D';
    } else if (i == 3) {
        return 'H';
    } else {
    }
    return 'S';
}

/* sort the players hand array to SCDH order */
void sort_hands(char playerHand[4][78], int playerNumber) {
    for (int player = 0; player < playerNumber; player++) {
        int hand[4][13];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 13; j++) {
                hand[i][j] = 0;
            }
        }
        for (int i = 0; i < (52 / playerNumber); i++) {
            int suit = char_to_suit_index(playerHand[player][3 * i + 1]);
            int rank = char_to_rank_index(playerHand[player][3 * i]);
            hand[suit][rank] = 1;
        }
        int length = 0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 13; j++) {
                if (hand[i][j] == 1) {
                    playerHand[player][length] = rank_index_to_char(j);
                    length++;
                    playerHand[player][length] = suit_index_to_char(i);
                    length++;
                    playerHand[player][length] = ',';
                    length++;
                }
            }
        }
        if (length == 0) {
            playerHand[player][0] = '\0';
        } else {
            playerHand[player][length - 1] = '\0';
        }

    }

}

/* distribute cards from a given deck */
void deal_cards(char playerHand[4][78], int playerNumber, char **decks,
        int roundNo) {
    int handIndex = 0;
    int currentPlayer = 0;
    for (int i = 0; i < 52; i++) {
        if (playerNumber == 3 && decks[roundNo][i * 2] == '2' &&
                decks[roundNo][i * 2 + 1] == 'D') {

        } else {
            playerHand[currentPlayer][handIndex * 3] = decks[roundNo][i * 2];
            playerHand[currentPlayer][handIndex * 3 + 1] =
                    decks[roundNo][i * 2 + 1];
            playerHand[currentPlayer][handIndex * 3 + 2] = ',';
            if (currentPlayer == playerNumber - 1) {
                currentPlayer = 0;
                handIndex++;
            } else {
                currentPlayer++;
            }
        }
    }
    for (int i = 0; i < playerNumber; i++) {
        playerHand[i][handIndex * 3 - 1] = '\0';
    }
    sort_hands(playerHand, playerNumber);
}

/* run players, any system call failure will cause error(5) */
void run_clubbers(int player, char *argv[], int playerNumber,
        int pipeIn[4][2], int pipeOut[4][2], int pipeErr[4][2]) {
    if (dup2(pipeOut[player][0], 0) < 0) {
        error(5);
    }
    if (dup2(pipeIn[player][1], 1) < 0) {
        error(5);
    }
    if (freopen("/dev/null", "w", stderr) == NULL) {
        error(5);
    }
    char playerCode[2] = {'A' + player, '\0'};
    // if (freopen(playerCode, "w", stderr) == NULL) {
    //     error(5);
    // }
    char temp[2] = {'0' + playerNumber, '\0'};
    execl(argv[3 + player], argv[3 + player], temp, playerCode, NULL);
    error(5);
}

/* fork and run processes */
void fork_players(int playerNumber, int pid[4], char *argv[], FILE *fileIn[4],
        FILE *fileOut[4]) {
    int pipeIn[4][2], pipeOut[4][2], pipeErr[4][2];
    for (int i = 0; i < 4; i++) {
        if (pipe(pipeIn[i]) < 0 || pipe(pipeOut[i]) < 0 ||
                pipe(pipeErr[i]) < 0) {
            error(5);
        }
    }

    for (int i = 0; i < playerNumber; i++) {
        if((pid[i] = fork()) == 0) {
            for (int j = 0; j < playerNumber; j++) {
                if (i != j) {
                    close(pipeIn[j][1]);
                }
                
            }
            run_clubbers(i, argv, playerNumber, pipeIn, pipeOut, pipeErr);
        } else {
            close(pipeIn[i][1]);
            close(pipeOut[i][0]);

            if ((fileIn[i] = fdopen(pipeIn[i][0], "r")) == NULL) {
                error(5);
            }
            if ((fileOut[i] = fdopen(pipeOut[i][1], "w")) == NULL) {
                error(5);
            }
        }
    }
}

/* print hand card of each player */
void display_hand(char playerHand[4][78], int playerNumber) {
    for (int i = 0; i < playerNumber; i++) {
        fprintf(stdout, "Player (%c): %s\n", 'A' + i, playerHand[i]);
    }
}

/* send newround message to players */
void give_card_to_player(char playerHand[4][78], int playerNumber,
        FILE *fileOut[4]) {
    for (int i = 0; i < playerNumber; i++) {
        fprintf(fileOut[i], "newround %s\n", playerHand[i]);
        fflush(fileOut[i]);
    }
}

/* check if all players started successfully, if not close the hub*/
void check_players_started(int playerNumber, FILE *fileIn[4]) {
    for (int i = 0; i < playerNumber; ++i) {
        int ch;
        if((ch = fgetc(fileIn[i])) == EOF) {
            error(5);
        } else if(ch != '-') {
            error(6);
        }
    }
}

/* switch the current player to the next */
void next_player(int *currentPlayer, int playerNumber) {
    if (*currentPlayer == playerNumber - 1) {
        *currentPlayer = 0;
    } else {
        (*currentPlayer)++;
    }
}

/* read the pipe file for the current player */
void get_response(int currentPlayer, FILE *fileIn[4], char response[100]) {
    for (int i = 0; i < 100; i++) {
        response[i] = '\0';
    }
    char ch;
    int i = 0;
    while ((ch = fgetc(fileIn[currentPlayer])) != '\n') {
        if (ch == EOF) {
            error(6);
        }
        if (i < 100) {
            response[i] = ch;
        }
        i++;
    }
    if (i < 100) {
        response[i] = '\0';
    } else {
        response[99] = '\0';
    }
}

/* print and send card played message */
void declare_played(FILE *fileOut[4], char playedCard[4][2],
        int currentPlayer, int playerNumber, int type) {
    for (int i = 0; i < playerNumber; ++i) {
        fprintf(fileOut[i], "played %c%c\n", playedCard[currentPlayer][0],
                playedCard[currentPlayer][1]);
        fflush(fileOut[i]);
    }
    char p = 'A' + currentPlayer;
    if (type == 0) {
        fprintf(stdout, "Player %c led %c%c\n", p,
                playedCard[currentPlayer][0], playedCard[currentPlayer][1]);
    } else {
        fprintf(stdout, "Player %c played %c%c\n", p,
                playedCard[currentPlayer][0], playedCard[currentPlayer][1]);
    }
}

/* start new trick and send message to current player */
void lead_card(int currentPlayer, FILE *fileOut[4], FILE *fileIn[4],
        char playerHand[4][78], char playedCard[4][2], int playerNumber) {
    fprintf(fileOut[currentPlayer], "newtrick\n");
    fflush(fileOut[currentPlayer]);
    char response[100];
    get_response(currentPlayer, fileIn, response);
    if (strlen(response) != 2) {
        error(7);
    }
    char c = response[0];
    if (!((c >= '2' && c <= '9') || c == 'T' || c == 'J' || c == 'Q' ||
            c == 'K' || c == 'A')) {
        error(7);
    }
    c = response[1];
    if (!(c == 'S' || c == 'C' || c == 'D' || c == 'H')) {
        error(7);
    }
    char *find = strstr(playerHand[currentPlayer], response);
    if (find == NULL) {
        error(8);
    }
    playedCard[currentPlayer][0] = *find;
    *find = '-';
    find++;
    playedCard[currentPlayer][1] = *find;
    *find = '-';
    declare_played(fileOut, playedCard, currentPlayer, playerNumber, 0);
}

/* follow the  trick and send message to current player */
void follow_card(int currentPlayer, FILE *fileOut[4], FILE *fileIn[4],
        char playerHand[4][78], char playedCard[4][2], int playerNumber,
        char leadingSuit) {
    fprintf(fileOut[currentPlayer], "yourturn\n");
    fflush(fileOut[currentPlayer]);
    char response[100];
    get_response(currentPlayer, fileIn, response);
    if (strlen(response) != 2) {
        error(7);
    }
    char c = response[0];
    if (!((c >= '2' && c <= '9') || c == 'T' || c == 'J' ||
            c == 'Q' || c == 'K' || c == 'A')) {
        error(7);
    }
    c = response[1];
    if (!(c == 'S' || c == 'C' || c == 'D' || c == 'H')) {
        error(7);
    }
    char *find = strstr(playerHand[currentPlayer], response);
    if (find == NULL) {
        error(8);
    }
    if (c != leadingSuit) {
        for (int i = 0; i < strlen(playerHand[currentPlayer]); ++i) {
            if (playerHand[currentPlayer][i] == leadingSuit) {
                error(8);
            }
        }
    }
    playedCard[currentPlayer][0] = *find;
    *find = '-';
    find++;
    playedCard[currentPlayer][1] = *find;
    *find = '-';
    declare_played(fileOut, playedCard, currentPlayer, playerNumber, 1);
}

/* send trick over message to all players */
void trick_over(FILE *fileOut[4], int playerNumber) {
    for (int i = 0; i < playerNumber; ++i) {
        fprintf(fileOut[i], "trickover\n");
        fflush(fileOut[i]);
    }
}

/* update scores after a trick */
void update_score(char playedCard[4][2], int playerNumber, char leadingSuit,
        int *currentPlayer, int scores[4]) {
    int trickScore = 0;
    for (int i = 0; i < playerNumber; ++i) {
        if (playedCard[i][1] == 'C') {
            trickScore++;
        }
    }
    char maxRank = '2';
    for (int i = 0; i < playerNumber; ++i) {
        if (playedCard[i][1] == leadingSuit) {
            if (char_to_rank_index(playedCard[i][0]) >
                    char_to_rank_index(maxRank)) {
                maxRank = playedCard[i][0];
            }
        }
    }
    for (int i = 0; i < playerNumber; ++i) {
        if (playedCard[i][1] == leadingSuit && playedCard[i][0] == maxRank) {
            *currentPlayer = i;
            scores[i] += trickScore;
        }
    }
}

/* print the winners */
void display_winner(int scores[4], int playerNumber) {
    int min, i;
    min = scores[0];
    for (i = 1; i < playerNumber; i++) {
        if (scores[i] < min) {
            min = scores[i];
        }
    }
    printf("Winner(s): ");
    i = 0;
    while (scores[i] != min) {
        i++;
    }
    printf("%c", 'A' + i);
    for (i++; i < playerNumber; i++) {
        if (scores[i] == min) {
            printf(" %c", 'A' + i);
        }
    }
    printf("\n");
}

/* send gameover message to players */
void send_game_over(FILE *fileOut[4], int playerNumber) {
    for (int i = 0; i < playerNumber; ++i) {
        fprintf(fileOut[i], "end\n");
        fflush(fileOut[i]);
    }
}

/* send score message to players */
void send_score(FILE *fileOut[4], int playerNumber, int scores[4]) {
    for (int i = 0; i < playerNumber; ++i) {
        if (playerNumber == 2) {
            fprintf(fileOut[i], "scores %d,%d\n", scores[0], scores[1]);
            fflush(fileOut[i]);
        } else if (playerNumber == 3) {
            fprintf(fileOut[i], "scores %d,%d,%d\n", scores[0], scores[1],
                    scores[2]);
            fflush(fileOut[i]);
        } else {
            fprintf(fileOut[i], "scores %d,%d,%d,%d\n", scores[0], scores[1], 
                    scores[2], scores[3]);
            fflush(fileOut[i]);
        }
    }
    if (playerNumber == 2) {
        fprintf(stdout, "scores %d,%d\n", scores[0], scores[1]);
        fflush(stdout);
    } else if (playerNumber == 3) {
        fprintf(stdout, "scores %d,%d,%d\n", scores[0], scores[1], scores[2]);
        fflush(stdout);
    } else {
        fprintf(stdout, "scores %d,%d,%d,%d\n", scores[0],
                scores[1], scores[2], scores[3]);
        fflush(stdout);
    }
}

/* calculate scores, check if game is over and send corresponding message */
void round_over(int playerNumber, int goalPoint, int *roundNo, int deckNumber,
        int scores[4], FILE *fileOut[4]) {
    for (int i = 0; i < playerNumber; ++i) {
        if (scores[i] >= goalPoint) {
            send_score(fileOut, playerNumber, scores);
            display_winner(scores, playerNumber);
            send_game_over(fileOut, playerNumber);
            exit(0);
        }
    }
    send_score(fileOut, playerNumber, scores);
    if (*roundNo == deckNumber - 1) {
        *roundNo = 0;
    } else {
        (*roundNo)++;
    }
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    char **decks = NULL;
    int playerNumber = 2, deckNumber = 1, goalPoint = 0, roundNo = 0;
    check_argu(argc, argv);
    open_decks_file(argv, &deckNumber, &decks);
    char playerHand[4][78];
    player_hand_init(playerHand);
    FILE *fileIn[4];
    FILE *fileOut[4];
    int pid[4];
    int scores[4] = {0, 0, 0, 0};

    playerNumber = argc - 3;
    goalPoint = atoi(argv[2]);

    fork_players(playerNumber, pid, argv, fileIn, fileOut);
    check_players_started(playerNumber, fileIn);

    int currentPlayer = 0;
    while (1) {
        deal_cards(playerHand, playerNumber, decks, roundNo);
        display_hand(playerHand, playerNumber);
        give_card_to_player(playerHand, playerNumber, fileOut);
        int trickNumberber = 52 / playerNumber;
        for (int trick = 0; trick < trickNumberber; trick++) {
            char playedCard[4][2];
            lead_card(currentPlayer, fileOut, fileIn, playerHand, 
                    playedCard, playerNumber);
            char leadingSuit = playedCard[currentPlayer][1];
            next_player(&currentPlayer, playerNumber);
            for (int i = 1; i < playerNumber; i++) {
                follow_card(currentPlayer, fileOut, fileIn,
                        playerHand, playedCard, playerNumber, leadingSuit);
                next_player(&currentPlayer, playerNumber);
            }
            trick_over(fileOut, playerNumber);
            update_score(playedCard, playerNumber, leadingSuit,
                    &currentPlayer, scores);
        }
        round_over(playerNumber, goalPoint, &roundNo, deckNumber,
                scores, fileOut);
    }
    return 0;
}