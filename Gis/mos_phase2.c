#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char M[300][4];
char IR[4], R[4];
char C = 'F';
int IC = 0;
int SI, PI, TI;
int PTR;
int TTC, LLC;
int TTL, TLL;
char jobID[5];
int frameUsed[30];
int pageTable[10];
int faultAddr;
int faultOnOperand;
int running;

FILE *fin, *fout;

void clearFrame(int f)
{
    int base = f * 10;
    int i, j;
    for (i = 0; i < 10; i++)
        for (j = 0; j < 4; j++)
            M[base + i][j] = ' ';
}

int getFrame()
{
    int free[30], cnt = 0, i;
    for (i = 0; i < 30; i++)
        if (!frameUsed[i])
            free[cnt++] = i;
    if (cnt == 0)
        return -1;
    int f = free[rand() % cnt];
    frameUsed[f] = 1;
    clearFrame(f);
    return f;
}

int addrMap(int va)
{
    if (va < 0 || va > 99)
    {
        PI = 2;
        return -1;
    }
    int pg = va / 10;
    int off = va % 10;
    if (pageTable[pg] == -1)
    {
        PI = 3;
        return -1;
    }
    return pageTable[pg] * 10 + off;
}

int allocPage(int va)
{
    int pg = va / 10;
    if (pageTable[pg] != -1)
        return 1;
    int f = getFrame();
    if (f < 0)
        return 0;
    pageTable[pg] = f;
    int e = PTR * 10 + pg;
    M[e][0] = '1';
    M[e][1] = ' ';
    M[e][2] = '0' + f / 10;
    M[e][3] = '0' + f % 10;
    return 1;
}

void freeJobMemory()
{
    int i;
    for (i = 0; i < 10; i++)
    {
        if (pageTable[i] != -1)
        {
            frameUsed[pageTable[i]] = 0;
            clearFrame(pageTable[i]);
            pageTable[i] = -1;
        }
    }
    if (PTR != -1)
    {
        frameUsed[PTR] = 0;
        clearFrame(PTR);
        PTR = -1;
    }
}

char *errMsg[] = {
    "NORMAL TERMINATION",
    "ERROR: OUT OF DATA",
    "ERROR: LINE LIMIT EXCEEDED",
    "ERROR: TIME LIMIT EXCEEDED",
    "ERROR: OPERATION CODE ERROR",
    "ERROR: OPERAND ERROR",
    "ERROR: INVALID PAGE FAULT"};

void terminate(int e1, int e2)
{
    fprintf(fout, "\n\n");
    fprintf(fout, "Job ID : %s\n", jobID);
    if (e2 == -1)
        fprintf(fout, "EM : %d - %s\n", e1, errMsg[e1]);
    else
        fprintf(fout, "EM : %d,%d - %s | %s\n", e1, e2, errMsg[e1], errMsg[e2]);
    freeJobMemory();
    running = 0;
}

void MOS()
{
    int va, ra, i, j;
    char buf[128];

    if (PI == 1)
    {
        (TI == 2) ? terminate(3, 4) : terminate(4, -1);
        return;
    }
    if (PI == 2)
    {
        (TI == 2) ? terminate(3, 5) : terminate(5, -1);
        return;
    }
    if (PI == 3)
    {
        if (TI == 2)
        {
            terminate(3, -1);
            return;
        }
        if (faultOnOperand && ((IR[0] == 'G' && IR[1] == 'D') || (IR[0] == 'S' && IR[1] == 'R')))
        {
            if (!allocPage(faultAddr))
            {
                terminate(6, -1);
                return;
            }
            PI = 0;
            faultOnOperand = 0;
            return;
        }
        terminate(6, -1);
        return;
    }

    if (SI == 1)
    {
        if (TI == 2)
        {
            terminate(3, -1);
            return;
        }
        if (fgets(buf, 128, fin) == NULL || strncmp(buf, "$END", 4) == 0)
        {
            terminate(1, -1);
            return;
        }
        buf[strcspn(buf, "\n")] = '\0';
        int len = strlen(buf);
        while (len < 40)
            buf[len++] = ' ';
        buf[40] = '\0';

        if (IR[2] < '0' || IR[2] > '9' || IR[3] < '0' || IR[3] > '9')
        {
            terminate(5, -1);
            return;
        }
        va = (IR[2] - '0') * 10 + (IR[3] - '0');

        int k = 0;
        for (i = 0; i < 10; i++)
        {
            PI = 0;
            ra = addrMap(va + i);
            if (PI == 3)
            {
                if (!allocPage(va + i))
                {
                    terminate(6, -1);
                    return;
                }
                PI = 0;
                ra = addrMap(va + i);
            }
            if (ra < 0)
            {
                terminate(5, -1);
                return;
            }
            for (j = 0; j < 4; j++)
                M[ra][j] = buf[k++];
        }
        PI = 0;
        SI = 0;
        return;
    }

    if (SI == 2)
    {
        LLC++;
        if (IR[2] < '0' || IR[2] > '9' || IR[3] < '0' || IR[3] > '9')
        {
            terminate(5, -1);
            return;
        }
        va = (IR[2] - '0') * 10 + (IR[3] - '0');

        if (TI == 2)
        {
            for (i = 0; i < 10; i++)
            {
                PI = 0;
                ra = addrMap(va + i);
                if (ra < 0)
                {
                    for (j = 0; j < 4; j++)
                        fputc(' ', fout);
                }
                else
                {
                    for (j = 0; j < 4; j++)
                        fputc(M[ra][j], fout);
                }
            }
            fputc('\n', fout);
            PI = 0;
            terminate(3, -1);
            return;
        }

        if (LLC > TLL)
        {
            terminate(2, -1);
            return;
        }

        for (i = 0; i < 10; i++)
        {
            PI = 0;
            ra = addrMap(va + i);
            if (ra < 0)
            {
                for (j = 0; j < 4; j++)
                    fputc(' ', fout);
            }
            else
            {
                for (j = 0; j < 4; j++)
                    fputc(M[ra][j], fout);
            }
        }
        fputc('\n', fout);
        PI = 0;
        SI = 0;
        return;
    }

    if (SI == 3)
    {
        terminate(0, -1);
        return;
    }

    if (TI == 2)
        terminate(3, -1);
}

void executeUserProgram()
{
    int va, ra;
    int i;

    while (running)
    {
        SI = 0;
        PI = 0;
        faultOnOperand = 0;
        faultAddr = -1;

        ra = addrMap(IC);
        if (PI != 0)
        {
            // fetch page fault - always invalid in this design
        }
        else
        {
            for (i = 0; i < 4; i++)
                IR[i] = M[ra][i];
            IC++;

            if (IR[0] == 'H')
            {
                SI = 3;
            }
            else if (
                (IR[0] == 'L' && IR[1] == 'R') ||
                (IR[0] == 'S' && IR[1] == 'R') ||
                (IR[0] == 'C' && IR[1] == 'R') ||
                (IR[0] == 'B' && IR[1] == 'T') ||
                (IR[0] == 'G' && IR[1] == 'D') ||
                (IR[0] == 'P' && IR[1] == 'D'))
            {
                if (IR[2] < '0' || IR[2] > '9' || IR[3] < '0' || IR[3] > '9')
                {
                    PI = 2;
                    IC--;
                }
                else
                {
                    va = (IR[2] - '0') * 10 + (IR[3] - '0');
                    ra = addrMap(va);
                    if (PI != 0)
                    {
                        IC--;
                        if (PI == 3)
                        {
                            faultOnOperand = 1;
                            faultAddr = va;
                        }
                    }
                    else
                    {
                        if (IR[0] == 'L' && IR[1] == 'R')
                            for (i = 0; i < 4; i++)
                                R[i] = M[ra][i];
                        else if (IR[0] == 'S' && IR[1] == 'R')
                            for (i = 0; i < 4; i++)
                                M[ra][i] = R[i];
                        else if (IR[0] == 'C' && IR[1] == 'R')
                        {
                            C = 'T';
                            for (i = 0; i < 4; i++)
                                if (R[i] != M[ra][i])
                                {
                                    C = 'F';
                                    break;
                                }
                        }
                        else if (IR[0] == 'B' && IR[1] == 'T')
                        {
                            if (C == 'T')
                                IC = va;
                        }
                        else if (IR[0] == 'G' && IR[1] == 'D')
                            SI = 1;
                        else if (IR[0] == 'P' && IR[1] == 'D')
                            SI = 2;
                    }
                }
            }
            else
            {
                PI = 1; // invalid opcode
            }
        }

        TTC++;
        if (TTC == TTL)
            TI = 2;

        if (SI || PI || TI)
            MOS();
    }
}

int parseNum(char *s, int n)
{
    int v = 0, i;
    for (i = 0; i < n; i++)
        if (s[i] >= '0' && s[i] <= '9')
            v = v * 10 + (s[i] - '0');
    return v;
}

void load()
{
    char line[128];
    int pageNum;

    fin = fopen("input.txt", "r");
    fout = fopen("output.txt", "w");
    if (!fin || !fout)
    {
        printf("file open error\n");
        return;
    }

    memset(frameUsed, 0, sizeof(frameUsed));
    for (int i = 0; i < 300; i++)
        for (int j = 0; j < 4; j++)
            M[i][j] = ' ';
    srand(time(NULL));

    while (fgets(line, sizeof(line), fin))
    {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "$AMJ", 4) == 0)
        {
            for (int i = 0; i < 10; i++)
                pageTable[i] = -1;
            for (int i = 0; i < 4; i++)
            {
                IR[i] = ' ';
                R[i] = ' ';
            }
            IC = 0;
            C = 'F';
            SI = 0;
            TI = 0;
            PI = 0;
            TTC = 0;
            LLC = 0;
            running = 0;
            faultAddr = -1;
            faultOnOperand = 0;
            pageNum = 0;

            strncpy(jobID, line + 4, 4);
            jobID[4] = '\0';
            TTL = parseNum(line + 8, 4);
            TLL = parseNum(line + 12, 4);

            PTR = getFrame();
            int base = PTR * 10;
            for (int i = 0; i < 10; i++)
            {
                M[base + i][0] = '0';
                M[base + i][1] = ' ';
                M[base + i][2] = '*';
                M[base + i][3] = '*';
            }
        }
        else if (strncmp(line, "$DTA", 4) == 0)
        {
            IC = 0;
            running = 1;
            executeUserProgram();
        }
        else if (strncmp(line, "$END", 4) == 0)
        {
            // nothing to do
        }
        else
        {
            if (pageNum >= 10)
                continue;
            int f = getFrame();
            if (f < 0)
            {
                printf("no free frame\n");
                continue;
            }

            pageTable[pageNum] = f;
            int e = PTR * 10 + pageNum;
            M[e][0] = '1';
            M[e][1] = ' ';
            M[e][2] = '0' + f / 10;
            M[e][3] = '0' + f % 10;

            int len = strlen(line);
            while (len < 40)
                line[len++] = ' ';
            line[40] = '\0';

            int base = f * 10, k = 0;
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 4; j++)
                    M[base + i][j] = line[k++];

            pageNum++;
        }
    }

    fclose(fin);
    fclose(fout);
}

int main()
{
    load();
    printf("done. check output.txt\n");
    return 0;
}
