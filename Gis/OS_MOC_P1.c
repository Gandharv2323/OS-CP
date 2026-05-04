#include <stdio.h>
#include <string.h>

char M[100][4];
char IR[4];
char R[4];
int IC;
char C;
int SI;
int abortFlag;

FILE *fin;
FILE *fout;

void init() {
    int i, j;
    for (i = 0; i < 100; i++)
        for (j = 0; j < 4; j++)
            M[i][j] = ' ';

    for (i = 0; i < 4; i++) {
        IR[i] = ' ';
        R[i] = ' ';
    }

    IC = 0;
    C = 'F';
    SI = 0;
    abortFlag = 0;
}

int getAddress() {
    return (IR[2] - '0') * 10 + (IR[3] - '0');
}

void MOS() {

    if (SI == 1) {
        char line[100];
        int addr, k = 0, i, j;

        if (fgets(line, sizeof(line), fin) == NULL) {
            abortFlag = 1;
            return;
        }

        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "$END", 4) == 0) {
            abortFlag = 1;
            return;
        }

        int len = strlen(line);
        while (len < 40)
            line[len++] = ' ';
        line[40] = '\0';

        addr = getAddress();

        for (i = 0; i < 10 && addr + i < 100; i++) {
            for (j = 0; j < 4; j++) {
                M[addr + i][j] = line[k++];
            }
        }
    }

    else if (SI == 2) {
        int addr, i, j;

        addr = getAddress();

        for (i = 0; i < 10 && addr + i < 100; i++) {
            for (j = 0; j < 4; j++) {
                fputc(M[addr + i][j], fout);
            }
        }
        fputc('\n', fout);
    }

    else if (SI == 3) {
        fputc('\n', fout);
        fputc('\n', fout);
    }
}

void EXECUTEUSERPROGRAM() {
    int addr, i;
    char opcode[3];

    while (1) {

        if (IC < 0 || IC >= 100)
            break;

        for (i = 0; i < 4; i++)
            IR[i] = M[IC][i];
        IC++;

        opcode[0] = IR[0];
        opcode[1] = IR[1];
        opcode[2] = '\0';

        if (IR[0] == 'H') {
            SI = 3;
            MOS();
            break;
        }

        addr = getAddress();

        if (strcmp(opcode, "LR") == 0) {
            for (i = 0; i < 4; i++)
                R[i] = M[addr][i];
        }

        else if (strcmp(opcode, "SR") == 0) {
            for (i = 0; i < 4; i++)
                M[addr][i] = R[i];
        }

        else if (strcmp(opcode, "CR") == 0) {
            int match = 1;
            for (i = 0; i < 4; i++) {
                if (R[i] != M[addr][i]) {
                    match = 0;
                    break;
                }
            }
            C = match ? 'T' : 'F';
        }

        else if (strcmp(opcode, "BT") == 0) {
            if (C == 'T')
                IC = addr;
        }

        else if (strcmp(opcode, "GD") == 0) {
            SI = 1;
            MOS();
            if (abortFlag) break;
        }

        else if (strcmp(opcode, "PD") == 0) {
            SI = 2;
            MOS();
        }
    }
}

void STARTEXECUTION() {
    IC = 0;
    EXECUTEUSERPROGRAM();
}

void LOAD() {

    char line[100];
    int m = 0;

    fin = fopen("input.txt", "r");
    fout = fopen("output.txt", "w");

    if (!fin || !fout) {
        printf("Error opening file!\n");
        return;
    }

    while (fgets(line, sizeof(line), fin)) {

        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "$AMJ", 4) == 0) {
            init();
            m = 0;
        }

        else if (strncmp(line, "$DTA", 4) == 0) {
            STARTEXECUTION();
        }

        else if (strncmp(line, "$END", 4) == 0) {
            continue;
        }

        else {
            int len = strlen(line);

            while (len < 40)
                line[len++] = ' ';
            line[40] = '\0';

            int k = 0;

            for (int i = 0; i < 10 && m < 100; i++) {
                for (int j = 0; j < 4; j++) {
                    M[m][j] = line[k++];
                }
                m++;
            }
        }
    }

    fclose(fin);
    fclose(fout);
}

int main() {
    LOAD();
    printf("Execution Completed.\n");
    return 0;
}
