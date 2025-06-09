#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

/* ---------- data types ---------- */
typedef enum { KING, ROOK } Piece;
typedef enum { WHITE, BLACK } Player;

typedef struct {
    Piece  piece;
    Player player;
} Cell;

typedef struct {
    Cell ***field;
    size_t  rows;
    size_t  cols;
} Board;

typedef struct {
    char from[3];
    char to[3];
    char pieceName[8];
} MoveRecord;

/* ---------- cell management ---------- */
static Cell *newCell(Piece p, Player pl)
{
    Cell *c = (Cell*)malloc(sizeof *c);
    if(!c){ perror("malloc"); exit(EXIT_FAILURE); }
    c->piece  = p;
    c->player = pl;
    return c;
}

/* ---------- board management ---------- */
Board *genBoard(void)
{
    const size_t N = 8;
    Board *b = (Board*)malloc(sizeof *b);
    if(!b){ perror("malloc"); exit(EXIT_FAILURE); }

    b->rows = b->cols = N;
    b->field = (Cell***)malloc(N * sizeof *b->field);
    if(!b->field){ perror("malloc"); exit(EXIT_FAILURE); }

    for(size_t r = 0; r < N; ++r){
        b->field[r] = (Cell**)calloc(N, sizeof *b->field[r]);
        if(!b->field[r]){ perror("calloc"); exit(EXIT_FAILURE); }
    }
    return b;
}

void freeBoard(Board *b)
{
    for(size_t r = 0; r < b->rows; ++r){
        for(size_t c = 0; c < b->cols; ++c)
            free(b->field[r][c]);
        free(b->field[r]);
    }
    free(b->field);
    free(b);
}

static const char *pieceUTF8(const Cell *cell)
{
    if(!cell) return "·";
    switch(cell->piece){
        case KING: return cell->player == WHITE ? "♚" : "♔";
        case ROOK: return cell->player == WHITE ? "♜" : "♖";
        default:   return "?";
    }
}

int isAdjacent(Board *brd, size_t r, size_t c){
    int dr[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dc[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    
    for(int i = 0; i < 8; i++){
        int nr = r + dr[i];
        int nc = c + dc[i];
        
        if(nr >= 0 && nr < (int)brd->rows && nc >= 0 && nc < (int)brd->cols){
            Cell *cell = brd->field[nr][nc];
            if(cell && cell->piece == KING) return 1;
        }
    }
    return 0;
}

void placeRandom(Board *brd, Piece piece, Player player){
    size_t r, c;
    do{
        r = rand() % brd->rows;
        c = rand() % brd->cols;
    } while(brd->field[r][c] != NULL || (piece == KING && isAdjacent(brd, r, c)));
    
    brd->field[r][c] = newCell(piece, player);
}

void printBoard(Board *brd)
{
    printf("  a b c d e f g h\n");
    for(size_t r = 0; r < brd->rows; ++r){
        printf("%zu ", 8 - r);
        for(size_t c = 0; c < brd->cols; ++c)
            printf("%s ", pieceUTF8(brd->field[r][c]));
        printf("%zu\n", 8 - r);
    }
    printf("  a b c d e f g h\n");
}

/* ---------- validate moves ---------- */
int isValidMove(Board *brd, int r1, int c1, int r2, int c2){
    if(r1 == r2 && c1 == c2) return 0; // няма движение

    Cell *src = brd->field[r1][c1];
    if(!src) return 0;

    int dr = r2 - r1;
    int dc = c2 - c1;

    switch(src->piece){
        case KING:
            // Царят може да мърда само с 1 квадрат във всички посоки
            if(abs(dr) <= 1 && abs(dc) <= 1) return 1;
            return 0;

        case ROOK:
            // Топът може да мърда само по ред или колона
            if(dr != 0 && dc != 0) return 0; // няма диагонално

            // Проверяваме дали пътят е свободен
            if(dr == 0){ // движение по ред
                int step = (dc > 0) ? 1 : -1;
                for(int c = c1 + step; c != c2; c += step){
                    if(brd->field[r1][c] != NULL) return 0;
                }
            } else { // движение по колона
                int step = (dr > 0) ? 1 : -1;
                for(int r = r1 + step; r != r2; r += step){
                    if(brd->field[r][c1] != NULL) return 0;
                }
            }
            return 1;

        default:
            return 0;
    }
}

/* ---------- helper: check if cell under attack ---------- */
int isUnderAttack(Board *brd, int r, int c){
    // Check rooks
    for(size_t i = 0; i < brd->rows; ++i){
        for(size_t j = 0; j < brd->cols; ++j){
            Cell *cell = brd->field[i][j];
            if(!cell || cell->player != WHITE) continue;
            if(cell->piece == ROOK){
                if(i == r){
                    int dir = (j < c) ? 1 : -1;
                    int clear = 1;
                    for(int col = j + dir; col != c; col += dir){
                        if(brd->field[i][col] != NULL){ clear = 0; break; }
                    }
                    if(clear) return 1;
                }
                if(j == c){
                    int dir = (i < r) ? 1 : -1;
                    int clear = 1;
                    for(int row = i + dir; row != r; row += dir){
                        if(brd->field[row][j] != NULL){ clear = 0; break; }
                    }
                    if(clear) return 1;
                }
            }
            if(cell->piece == KING){
                if(abs((int)i - r) <= 1 && abs((int)j - c) <= 1)
                    return 1;
            }
        }
    }
    return 0;
}

/* ---------- minimax for black king ---------- */
void minimaxMoveForBlackKing(Board *brd, MoveRecord moves[], int moveCount, int kingMoves, int rookMoves, int checks){
    // Find Black King
    int rBK = -1, cBK = -1;
    for(size_t i = 0; i < brd->rows; ++i){
        for(size_t j = 0; j < brd->cols; ++j){
            Cell *cell = brd->field[i][j];
            if(cell && cell->player == BLACK && cell->piece == KING){
                rBK = i;
                cBK = j;
                break;
            }
        }
        if(rBK != -1) break;
    }

    

    // Directions for King movement
    int dr[9] = {0, -1, -1, -1, 0, 1, 1, 1, 0};
    int dc[9] = {0, -1, 0, 1, 1, 1, 0, -1, -1};

    int bestScore = -10000;
    int bestR = rBK, bestC = cBK;
    int canMove = 0;

    for(int i = 0; i < 9; ++i){
        int nr = rBK + dr[i];
        int nc = cBK + dc[i];

        if(nr < 0 || nr >= (int)brd->rows || nc < 0 || nc >= (int)brd->cols) continue;
        if(brd->field[nr][nc] != NULL) continue;

        // Temporarily move the king to check if the new position is safe
        Cell* king = brd->field[rBK][cBK];
        brd->field[rBK][cBK] = NULL;
        brd->field[nr][nc] = king;

        if(!isUnderAttack(brd, nr, nc)){
            int score = 0;
            for(size_t r = 0; r < brd->rows; ++r){
                for(size_t c = 0; c < brd->cols; ++c){
                    Cell *cell = brd->field[r][c];
                    if(cell && cell->player == WHITE){
                        int dist = abs((int)r - nr) + abs((int)c - nc);
                        score += dist;
                    }
                }
            }
            if(score > bestScore){
                bestScore = score;
                bestR = nr;
                bestC = nc;
                canMove = 1;
            }
        }

        // Undo the temporary move
        brd->field[nr][nc] = NULL;
        brd->field[rBK][cBK] = king;
    }

    if(!canMove){
        printf("Black King cannot make a safe move!\n");
        printf("\nCheckmate! White wins!\n");
        printStats(moves, moveCount, kingMoves, rookMoves, checks);
        saveReplay("game_replay.txt", moves, moveCount);
        freeBoard(brd);
        exit(1);       
    }

    printf("Black King moves to %c%d\n", 'a' + bestC, 8 - bestR);
    brd->field[bestR][bestC] = brd->field[rBK][cBK];
    brd->field[rBK][cBK] = NULL;
}

/* ---------- checkmate detection ---------- */
int isCheckmate(Board *brd){
    // Find Black King
    int rBK = -1, cBK = -1;
    for(size_t i = 0; i < brd->rows; ++i){
        for(size_t j = 0; j < brd->cols; ++j){
            Cell *cell = brd->field[i][j];
            if(cell && cell->player == BLACK && cell->piece == KING){
                rBK = i;
                cBK = j;
                break;
            }
        }
        if(rBK != -1) break;
    }

    // If king is not under attack, it's not checkmate
    if(!isUnderAttack(brd, rBK, cBK)) return 0;

    // Check all possible moves for the king
    int dr[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
    int dc[8] = {-1, 0, 1, 1, 1, 0, -1, -1};

    for(int i = 0; i < 8; ++i){
        int nr = rBK + dr[i];
        int nc = cBK + dc[i];

        if(nr < 0 || nr >= (int)brd->rows || nc < 0 || nc >= (int)brd->cols) continue;
        if(brd->field[nr][nc] != NULL) continue;
        if(!isUnderAttack(brd, nr, nc)) return 0; // Found a safe square
    }

    return 1; // No safe moves found
}

/* ---------- helper: piece name ---------- */
const char* pieceName(Piece p){
    switch(p){
        case KING: return "King";
        case ROOK: return "Rook";
        default: return "?";
    }
}

/* ---------- helper: save replay ---------- */
void saveReplay(const char* filename, MoveRecord moves[], int moveCount){
    FILE *f = fopen(filename, "w");
    if(!f){ perror("fopen"); return; }
    for(int i = 0; i < moveCount; ++i){
        fprintf(f, "%s %s %s\n", moves[i].pieceName, moves[i].from, moves[i].to);
    }
    fclose(f);
}

/* ---------- helper: print stats ---------- */
void printStats(MoveRecord moves[], int moveCount, int kingMoves, int rookMoves, int checks){
    printf("\n--- Game Over ---\n");
    printf("Total moves: %d\n", moveCount);
    printf("King moves: %d\n", kingMoves);
    printf("Rook moves: %d\n", rookMoves);
    printf("Checks given: %d\n", checks);
    printf("Moves played:\n");
    for(int i = 0; i < moveCount; ++i){
        printf("%2d. %s %s -> %s\n", i+1, moves[i].pieceName, moves[i].from, moves[i].to);
    }
}

/* ---------- main logic ---------- */
int main(void)
{
    srand((unsigned)time(NULL));
    Board *brd = genBoard();

    /* Place white pieces */
    placeRandom(brd, KING,  WHITE);
    placeRandom(brd, ROOK,  WHITE);
    placeRandom(brd, ROOK,  WHITE);

    /* Place black king */
    placeRandom(brd, KING,  BLACK);

    
    char command[100];
    printBoard(brd);

    MoveRecord moves[100];
    int moveCount = 0;
    int kingMoves = 0, rookMoves = 0, checks = 0;

    while(1){
        printf("\nEnter move (e.g., e2 e4) or 'q' to quit:\n> ");
        fgets(command, sizeof(command), stdin);

        if(command[0] == 'q') break;

        char from[3], to[3];
        int r1, c1, r2, c2;

        if(sscanf(command, "%2s %2s", from, to) != 2 ||
            from[0] < 'a' || from[0] > 'h' || from[1] < '1' || from[1] > '8' ||
            to[0] < 'a' || to[0] > 'h' || to[1] < '1' || to[1] > '8'){
            printf("Invalid input. Use format like e2 e4.\n");
            continue;
        }

        c1 = from[0] - 'a';
        r1 = 8 - (from[1] - '0');

        c2 = to[0] - 'a';
        r2 = 8 - (to[1] - '0');

        Cell *src = brd->field[r1][c1];
        if(!src || src->player != WHITE){
            printf("No white piece at that location.\n");
            continue;
        }

        if(brd->field[r2][c2]){
            printf("Destination not empty.\n");
            continue;
        }

        if(!isValidMove(brd, r1, c1, r2, c2)){
            printf("Invalid move for this piece.\n");
            continue;
        }

        // Move piece
        brd->field[r2][c2] = src;
        brd->field[r1][c1] = NULL;

        // Record the move
        strcpy(moves[moveCount].from, from);
        strcpy(moves[moveCount].to, to);
        strcpy(moves[moveCount].pieceName, pieceName(src->piece));
        moveCount++;

        printBoard(brd);

        // Check for checkmate after White's move
        if(isCheckmate(brd)){
            printf("\nCheckmate! White wins!\n");
            break;
        }

        //BLACK KING move (Minimax)
        minimaxMoveForBlackKing(brd, moves, moveCount, kingMoves, rookMoves, checks);
        if(isCheckmate(brd)){
            printStats(moves, moveCount, kingMoves, rookMoves, checks);
            break;  // End the game if black king is in checkmate
        }
        if(isCheckmate(brd)){
            break;
        }
        printBoard(brd);

    }
    return 0;
}