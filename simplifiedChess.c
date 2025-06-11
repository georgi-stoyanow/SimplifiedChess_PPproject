#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

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

//Създава полетата
static Cell *newCell(Piece p, Player pl)
{
    Cell *c = (Cell*)malloc(sizeof *c);
    if(!c){ perror("malloc"); exit(EXIT_FAILURE); }
    c->piece  = p;
    c->player = pl;
    return c;
}

//Управлява дъската
Board *genBoard(size_t size)
{
    Board *b = (Board*)malloc(sizeof *b);
    if(!b){ perror("malloc"); exit(EXIT_FAILURE); }

    b->rows = b->cols = size;
    b->field = (Cell***)malloc(size * sizeof *b->field);
    if(!b->field){ perror("malloc"); exit(EXIT_FAILURE); }

    for(size_t r = 0; r < size; ++r){
        b->field[r] = (Cell**)calloc(size, sizeof *b->field[r]);
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
    //Принтира буквите на колоните
    printf("  ");
    for(size_t c = 0; c < brd->cols; ++c){
        printf("%c ", 'a' + c);
    }
    printf("\n");

    //Принтира цифрите отстрани
    for(int r = (int)brd->rows - 1; r >= 0; --r){
        // Print flipped row number
        printf("%d ", (int)brd->rows - r);
        for(size_t c = 0; c < brd->cols; ++c){
            printf("%s ", pieceUTF8(brd->field[r][c]));
        }
        printf("%d\n", (int)brd->rows - r);
    }

    //Принтира буквите от другата страна
    printf("  ");
    for(size_t c = 0; c < brd->cols; ++c){
        printf("%c ", 'a' + c);
    }
    printf("\n");
}

//Валидира ходовете
int isValidMove(Board *brd, int r1, int c1, int r2, int c2){
    if(r1 == r2 && c1 == c2) return 0; //Няма движение

    Cell *src = brd->field[r1][c1];
    if(!src) return 0;
    int dr = r2 - r1;
    int dc = c2 - c1;

    switch(src->piece){
        case KING:
            //Царят може да мърда само с 1 квадрат във всички посоки
            if(abs(dr) <= 1 && abs(dc) <= 1) return 1;
            return 0;
        case ROOK:
            // Топът може да мърда само по ред или колона
            if(dr != 0 && dc != 0) return 0; // няма диагонално

            //Проверява дали пътят е свободен
            if(dr == 0){ //Движение по ред
                int step = (dc > 0) ? 1 : -1;
                for(int c = c1 + step; c != c2; c += step){
                    if(brd->field[r1][c] != NULL) return 0;
                }
            } else { //Движение по колона
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

//Проверява дали полето е под атака
int isUnderAttack(Board *brd, int r, int c){
    //Проверява дали топовете атакуват царя
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
//Запазва реплея
void replayGame(const char* filename){
    FILE *f = fopen(filename, "r");
    if(!f){ perror("fopen"); return; }
    char piece[16], from[8], to[8];
    int count = 1;
    while(fscanf(f, "%15s %10s %10s", piece, from, to) == 3){
        printf("%2d. %s %s -> %s\n", count++, piece, from, to);
    }
    fclose(f);
}

//Принтира статистиката след края на играта
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

//minmax
void minimaxMoveForBlackKing(Board *brd, MoveRecord moves[], int moveCount, int kingMoves, int rookMoves, int checks, FILE *replayFile, int *blackKingMoves)
{
    //Намира черния цар
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

    //Посоки на движение на черния цар
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

        //Временно премества царя на нова позиция за да провери дали е безопасна
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

        //Oтменя временното преместване
        brd->field[nr][nc]=NULL;
        brd->field[rBK][cBK]=king;
    }

    if(!canMove){
        printf("Black King cannot make a safe move!\n");
        printf("\nCheckmate! White wins!\n");
        printStats(moves,moveCount,kingMoves,rookMoves,checks);
        replayGame("game_replay.txt");
        freeBoard(brd);
        exit(1);       
    }

    printf("Black King moves to %c%d\n", 'a' + bestC, (int)brd->rows - bestR);
    brd->field[bestR][bestC] = brd->field[rBK][cBK];
    brd->field[rBK][cBK] = NULL;

    //Записва хода на черния цар във файл и увеличава брояча
    if(replayFile) {
        char fromSquare[4], toSquare[4];
        sprintf(fromSquare, "%c%d", 'a' + cBK, (int)brd->rows - rBK);
        sprintf(toSquare, "%c%d", 'a' + bestC, (int)brd->rows - bestR);
        fprintf(replayFile, "BlackKing %s %s\n", fromSquare, toSquare);
    }
    if(blackKingMoves) (*blackKingMoves)++;
}

//Засича дали черния е в мат
int isCheckmate(Board *brd){
    //Намери черния цар
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

    //Ако царя не е под атака не е в мат
    if(!isUnderAttack(brd, rBK, cBK)) return 0;

    //Провери дали царя има възможни ходове
    int dr[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
    int dc[8] = {-1, 0, 1, 1, 1, 0, -1, -1};

    for(int i = 0; i < 8; ++i){
        int nr = rBK + dr[i];
        int nc = cBK + dc[i];

        if(nr < 0 || nr >= (int)brd->rows || nc < 0 || nc >= (int)brd->cols) continue;
        if(brd->field[nr][nc] != NULL) continue;
        if(!isUnderAttack(brd, nr, nc)) return 0;//Намира безопасен ход
    }

    return 1;//Няма намерени безопасни хододве
}

//Помощник за имената на фигурите
const char* pieceName(Piece p){
    switch(p){
        case KING: return "King";
        case ROOK: return "Rook";
        default: return "?";
    }
}

int parseInput(const char* input, int* row, int* col, int boardSize) {
    if(strlen(input) < 2 || strlen(input) > 3)
        return 0;

    *col = input[0] - 'a';
    if(*col < 0 || *col >= boardSize)
        return 0;

    int parsedRow = 0;
    if(strlen(input) == 2){
        if(input[1] < '1' || input[1] > '9') return 0;
        parsedRow = input[1] - '0';
    } else if(strlen(input) == 3){
        if(input[1] < '0' || input[1] > '9') return 0;
        if(input[2] < '0' || input[2] > '9') return 0;
        parsedRow = (input[1] - '0') * 10 + (input[2] - '0');
    }

    if(parsedRow < 1 || parsedRow > (int)boardSize)
        return 0;

    *row = boardSize - parsedRow;
    return 1;
}

//Главна логика
int main() {
    Board *brd = NULL;
    MoveRecord moves[100];
    int moveCount = 0, kingMoves = 0, rookMoves = 0, checks = 0;

    size_t boardSize = 8;

    while(1) {
        printf("\nМеню:\n");
        printf("1. Старт на игра\n");
        printf("2. Промяна на размера на полето (4-26) и старт на нова игра\n");
        printf("3. Реплей на игра\n");
        printf("4. Изход\n");
        printf("Изберете опция: ");

        int choice;
        if(scanf("%d", &choice) != 1) {
            printf("Невалиден избор\n");
            while(getchar() != '\n'); //Изчистване на входния буфер
            continue;
        }
        while(getchar() != '\n'); //Изчистване на буфера след scanf

        if(choice == 1 || choice == 2) {
            if(choice == 2) {
                printf("Въведете нов размер на дъската (4-26): ");
                int newSize;
                if(scanf("%d", &newSize) != 1 || newSize < 4 || newSize > 26) {
                    printf("Невалиден размер! Продължаваме със стария размер %zu.\n", boardSize);
                    while(getchar() != '\n');
                } else {
                    boardSize = (size_t)newSize;
                    printf("Размерът на дъската е променен на %zu x %zu\n", boardSize, boardSize);
                    while(getchar() != '\n');
                }
            }

            //Стартиране на игра с текущия boardSize
            if(brd) freeBoard(brd);
            brd = genBoard(boardSize);
            moveCount = 0;
            kingMoves = 0;
            rookMoves = 0;
            checks = 0;
            placeRandom(brd, KING,  WHITE);
            placeRandom(brd, ROOK,  WHITE);
            placeRandom(brd, ROOK,  WHITE);
            placeRandom(brd, KING,  BLACK);

            //Отваря файл за записване на раплей
            FILE *replayFile = fopen("game_replay.txt", "w");
            if(!replayFile) {
                perror("Не може да се отвори файл за запис на реплей!");
                if(brd) freeBoard(brd);
                exit(1);
            }

            printBoard(brd);

            char input[10];
            while(1){
                printf("\nВаш ход (напр. a7 a6): ");
                char input[16];
                if(!fgets(input, sizeof(input), stdin)) break;
                input[strcspn(input, "\n")] = 0; // премахва \n

                char from[4], to[4];
                if(sscanf(input, "%2s %2s", from, to) != 2) {
                    printf("Невалиден формат на хода.\n");
                    continue;
                }

                int r1, c1, r2, c2;
                if (!parseInput(from, &r1, &c1, boardSize) || !parseInput(to, &r2, &c2, boardSize)) {
                    printf("Невалиден ход (неразпознат формат или извън дъската)\n");
                    continue;
                }

                Cell *src = brd->field[r1][c1];
                if(!src || src->player != WHITE){
                    printf("Няма бяла фигура на %s\n", from);
                    continue;
                }

                if(!isValidMove(brd, r1, c1, r2, c2)){
                    printf("Невалиден ход за фигурата.\n");
                    continue;
                }

                if(brd->field[r2][c2]) free(brd->field[r2][c2]);
                brd->field[r2][c2] = brd->field[r1][c1];
                brd->field[r1][c1] = NULL;

                //Записва хода
                MoveRecord *mr = &moves[moveCount++];
                strcpy(mr->from, from);
                strcpy(mr->to, to);
                strcpy(mr->pieceName, pieceName(src->piece));

                //Записва ходовете във файла
                fprintf(replayFile, "%s %s %s\n", mr->pieceName, mr->from, mr->to);

                if(src->piece == KING) kingMoves++;
                else if(src->piece == ROOK) rookMoves++;

                //Проверява дали черния цар е под атака след ход на белия
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
                if(rBK != -1 && isUnderAttack(brd, rBK, cBK)){
                    checks++;
                    printf("Шах на черния цар!\n");
                }

                printBoard(brd);

                if(isCheckmate(brd)){
                    printf("Мат! Бялата страна печели!\n");
                    printStats(moves, moveCount, kingMoves, rookMoves, checks);
                    fclose(replayFile);
                    freeBoard(brd);
                    brd = NULL;
                    break;
                }

                    static int blackKingMoves = 0;
                    minimaxMoveForBlackKing(brd, moves, moveCount, kingMoves, rookMoves, checks, replayFile, &blackKingMoves);
                    printBoard(brd);

            }
            fclose(replayFile);//Затваря и ако има break или нещо се случи
        } else if(choice == 3) {
            replayGame("game_replay.txt");
        } else if(choice == 4) {
            printf("Изход от програмата.\n");
            if(brd) freeBoard(brd);
            break;
        } else {
            printf("Невалиден избор.\n");
        }
    }

    return 0;
}

void fprintBoard(FILE *f, Board *brd)
{
    //Принтира буквите на колоните
    fprintf(f, "  ");
    for(size_t c = 0; c < brd->cols; ++c){
        fprintf(f, "%c ", 'a' + c);
    }
    fprintf(f, "\n");

    //Принтира цифрите отстрани
    for(int r = (int)brd->rows - 1; r >= 0; --r){
        fprintf(f, "%d ", (int)brd->rows - r);
        for(size_t c = 0; c < brd->cols; ++c){
            fprintf(f, "%s ", pieceUTF8(brd->field[r][c]));
        }
        fprintf(f, "%d\n", (int)brd->rows - r);
    }

    // Принтира буквите от другата страна
    fprintf(f, "  ");
    for(size_t c = 0; c < brd->cols; ++c){
        fprintf(f, "%c ", 'a' + c);
    }
    fprintf(f, "\n\n");
}