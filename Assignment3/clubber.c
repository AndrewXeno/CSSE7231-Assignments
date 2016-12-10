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
            fprintf(stderr, "Usage: player number_of_players myid\n");
            exit(1);
            break;
        case 2:
            fprintf(stderr, "Invalid player count\n");
            exit(2);
            break;
        case 3:
            fprintf(stderr, "Invalid player ID\n");
            exit(3);
            break;
        case 4:
            fprintf(stderr, "Unexpected loss of hub\n");
            exit(4);
            break;
        case 5:
            fprintf(stderr, "Bad message from hub\n");
            exit(5);
            break;
    }
}

/* Check if the arguments are valid. If not, exit with corresponding code*/
void check_argu(int argc, char *argv[]) {
    if (argc != 3) {
        error(1);
    }
    if (argv[1][1] != '\0' || atoi(argv[1]) < 2 || atoi(argv[1]) > 4) {
        error(2);
    }
    if (argv[2][1] != '\0') {
        error(2);
    } else if (argv[2][0] < 'A' || argv[2][0] > 'A' + atoi(argv[1]) - 1) {
        error(3);
    }
}

/*
 * read message from stdin, can put it into a string
 * also print the message to stderr (the first 20 chars)*
 */
void get_message(char message[100]) {
    for (int i = 0; i < 100; i++) {
        message[i] = '\0';
    }
    char ch;
    int i = 0;
    while ((ch = getchar()) != '\n') {
        if (ch == EOF) {
            error(4);
        }
        if (i < 100) {
            message[i] = ch;
        }
        i++;
    }
    if (i < 100) {
        message[i] = '\0';
    } else {
        message[99] = '\0';
    }

    char temp[21];
    if (strlen(message) < 20) {
        int i;
        for (i = 0; i < strlen(message); i++) {
            temp[i] = message[i];
        }
        temp[i] = '\0';
    } else {
        int i;
        for (i = 0; i < 20; i++) {
            temp[i] = message[i];
        }
        temp[20] = '\0';
    }
    fprintf(stderr, "From hub:%s\n", temp);
}

/* taks in a string of message and analyse its category
 * returns an int (1:newround, 2:newtrick, 3:trickover, 4:yourturn
 * 5:played, 6:scores, 7:end
 */
int message_category_check(char message[100]) {
    int len = strlen(message);
    if (len == 8 && strcmp(message, "newtrick") == 0) {
        return 2;
    } else if (len == 9 && strcmp(message, "trickover") == 0) {
        return 3;
    } else if (len == 8 && strcmp(message, "yourturn") == 0) {
        return 4;
    } else if (len == 9 && strncmp(message, "played ", 7) == 0) {
        return 5;
    } else if (len >= 10 && strncmp(message, "scores ", 7) == 0) {
        return 6;
    } else if (len == 3 && strcmp(message, "end") == 0) {
        return 7;
    } else if ((len == 86 || len == 59 || len == 47) &&
            strncmp(message, "newround ", 9) == 0) {
        return 1;
    } else {

        error(5);
    }
    return 0;
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

/* put newround card message to hand card array */
void get_hand(char message[100], int playerNumber, int hand[4][13]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 13; j++) {
            hand[i][j] = 0;
        }
    }
    for (int i = 0; i < (52 / playerNumber); i++) {
        int suit = char_to_suit_index(message[9 + 3 * i + 1]);
        int rank = char_to_rank_index(message[9 + 3 * i]);
        hand[suit][rank] = 1;
    }
}

/* check if all clubs which are smaller than i have already played
 * if yes, return 1, if no, return 0;
 */
int check_played_club(int i, int hand[4][13]) {
    if (i == 0) {
        return 1;
    } else {
        for (int j = 0; j < i; j++) {
            if (hand[1][j] == 0 || hand[1][j] == 1) {
                return 0;
            }
        }
        return 1;
    }
}

/* send played card to stdout */
void send_card(int suit, int rank) {
    fprintf(stdout, "%c%c\n", rank_index_to_char(rank),
            suit_index_to_char(suit));
    fflush(stdout);
}

/* play the lowest available card in a given suit */
int play_lowest_card(int suit, int hand[4][13]) {
    for (int i = 0; i < 13; i++) {
        if (hand[suit][i] == 1) {
            send_card(suit, i);
            hand[suit][i] = 2;
            return 1;
        }
    }
    return 0;
}

/* play the highest available card in a given suit */
int play_highest_card(int suit, int hand[4][13]) {
    for (int i = 12; i >= 0; i--) {
        if (hand[suit][i] == 1) {
            send_card(suit, i);
            hand[suit][i] = 2;
            return 1;
        }
    }
    return 0;
}

/* lead the trick */
void lead_card(int hand[4][13]) {
    for (int i = 0; i < 13; ++i) {
        if (hand[1][i] == 1) {
            if (check_played_club(i, hand)) {
                send_card(1, i);
                hand[1][i] = 2;
                return;
            } else {
                break;
            }
        }
    }

    if (play_lowest_card(2, hand)) {
        return;
    } else if (play_lowest_card(3, hand)) {
        return;
    } else if (play_lowest_card(0, hand)) {
        return;
    } else if (play_lowest_card(1, hand)) {
        return;
    }
}

/* follow the trick according to a given leading suit
 * and whether is the last to play
 */
void follow_card(int hand[4][13], int leadingSuit, int turnNo,
        int playerNumber) {
    if (play_lowest_card(leadingSuit, hand)) {
        return;
    } else if (turnNo == playerNumber - 1) {
        if (play_highest_card(3, hand)) {
            return;
        } else if (play_highest_card(2, hand)) {
            return;
        } else if (play_highest_card(1, hand)) {
            return;
        } else if (play_highest_card(0, hand)) {
            return;
        }
    } else {
        if (play_highest_card(1, hand)) {
            return;
        } else if (play_highest_card(2, hand)) {
            return;
        } else if (play_highest_card(3, hand)) {
            return;
        } else if (play_highest_card(0, hand)) {
            return;
        }
    }
}

/* save the score sent by hub */
void get_scores(char scores[100], char message[100]) {
    for (int i = 0; i < 100; i++) {
        scores[i] = '\0';
    }
    for (int i = 7; message[i] != '\0'; ++i) {
        scores[i - 7] = message[i];
    }
}

/* update the card condition according to played message */
void update_hand(int hand[4][13], char message[100], int turnNo,
        int *leadingSuit) {
    int rank = (char_to_rank_index(message[7]));
    int suit = (char_to_suit_index(message[8]));
    if (hand[suit][rank] == 0 || hand[suit][rank] == 2) {
        hand[suit][rank] = 2;
        if (turnNo == 0) {
            *leadingSuit = suit;
        }
    } else {
        error(5);
    }
}

/* set the score string to 0s */
void scores_init(char scores[100], int playerNumber) {
    if (playerNumber == 2) {
        strcpy(scores, "0,0");
    } else if (playerNumber == 3) {
        strcpy(scores, "0,0,0");
    } else {
        strcpy(scores, "0,0,0,0");
    }
}

/* print hand cards to stderr */
void display_hand(int hand[4][13]) {
    char temp[100] = "";
    int length = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 13; j++) {
            if (hand[i][j] == 1) {
                temp[length] = rank_index_to_char(j);
                length++;
                temp[length] = suit_index_to_char(i);
                length++;
                temp[length] = ',';
                length++;
            }
        }
    }
    if (length == 0) {
        temp[0] = '\0';
    } else {
        temp[length - 1] = '\0';
    }
    fprintf(stderr, "Hand: %s\n", temp);
}

/* print played cards to stderr */
void display_played(int hand[4][13]) {
    for (int i = 0; i < 4; i++) {
        char temp[100] = "";
        int length = 0;
        fprintf(stderr, "Played (%c): ", suit_index_to_char(i));
        for (int j = 0; j < 13; j++) {
            if (hand[i][j] == 2) {
                temp[length] = rank_index_to_char(j);
                length++;
                temp[length] = ',';
                length++;
            }
        }
        if (length == 0) {
            temp[0] = '\0';
        } else {
            temp[length - 1] = '\0';
        }
        fprintf(stderr, "%s\n", temp);
    }
}

/* print information to stderr */
void output_stderr(char message[100], int hand[4][13], char scores[100]) {
    display_hand(hand);
    display_played(hand);
    fprintf(stderr, "Scores: %s\n", scores);
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    check_argu(argc, argv);
    char scores[100];
    int hand[4][13];
    int playerNumber = atoi(argv[1]);
    int turnNo = 0;
    int leadingSuit = 0;

    scores_init(scores, playerNumber);
    fprintf(stdout, "-");
    fflush(stdout);


    while (1) {
        char message[100] = "";
        get_message(message);
        switch (message_category_check(message)) {
            case 1:
                get_hand(message, playerNumber, hand);
                break;
            case 2:
                lead_card(hand);
                break;
            case 3:
                turnNo = 0;
                break;
            case 4:
                follow_card(hand, leadingSuit, turnNo, playerNumber);
                break;
            case 5:
                update_hand(hand, message, turnNo, &leadingSuit);
                turnNo++;
                break;
            case 6:
                get_scores(scores, message);
                break;
            case 7:
                exit(0);
                break;
        }
        output_stderr(message, hand, scores);
    }
    return 0;
}


