#include <stdio.h>
#include <string.h>

void ratemonotonic();
void deadline();
int processSwap();
int processQueue();
int processExecution();
void resetProcess();
int processQueueD();
int processExecutionD();
int processSwapD();
int lostAdd();

int main(int argc, char *argv[])
{
    // char name1[][100] = {"T1", "T2"};
    // int period1[] = {50, 80};
    // int cpuburst1[] = {25, 35};
    FILE *arquivo;
    int lines = 0;
    int time;

    FILE *file_out;
    int A_REAL = 0;

#ifdef RATE
    A_REAL = 1;
    file_out = freopen("rate.out", "w", stdout);
#elif EDF
    A_REAL = 2;
    file_out = freopen("edf.out", "w", stdout);
#endif

    if (file_out == NULL)
    {
        printf("Erro ao abrir arquivo de saída.");
        return 1;
    }

    if (A_REAL == 0)
    {
        printf("Algoritimo não selecionado.");
        return 1;
    }

    if (argc != 2)
    {
        printf("Usage: %s [algorithm] [filename]\n", argv[0]);
        printf("Algorithm options: rate or edf\n");
        return 1;
    }

    // OPEN FILE
    arquivo = fopen(argv[1], "r");

    // CHECK IF OPEN
    if (arquivo == NULL)
    {
        printf("Erro ao abrir o arquivo.\n");
        return 1;
    }

    // READ TIME
    if (fscanf(arquivo, "%d", &time) != 1)
    {
        printf("Erro ao ler o valor inicial.\n");
        fclose(arquivo);
        return 1; // Sinaliza um erro
    }
    // COUNT LINES
    char ch;
    while (!feof(arquivo))
    {
        ch = fgetc(arquivo);
        if (ch == '\n')
        {
            lines++;
        }
    }
    rewind(arquivo);
    char name[lines][100];
    int period[lines];
    int cpu_burst[lines];

    // IGNORE FIRST LINE
    int c;
    while ((c = fgetc(arquivo)) != EOF && c != '\n')
        ;

    for (int i = 0; i < lines; i++)
    {
        fscanf(arquivo, "%s %d %d", name[i], &period[i], &cpu_burst[i]);
    }

    // CLOSE FILE
    fclose(arquivo);
    if (A_REAL == 1)
    {
        ratemonotonic(time, name, period, cpu_burst, lines);
    }
    else if (A_REAL == 2)
    {

        deadline(time, name, period, cpu_burst, lines);
    }

    fclose(file_out); // O arquivo já foi fechado pelo fclose(stdout)

    return 0;
}

void ratemonotonic(int total_time, char name[][100], int period[], int cpu_burst[], int size)
{
    // COUNT
    int lost_deadlines[size];
    int complete_execution[size];
    int killed_[size];
    // VAR
    int time = 0;
    int current_p = 0;
    int last_p = 0;
    int highest_p = 0;
    int avail_list[size];
    int remaining_burst[size];
    resetProcess(period, avail_list, cpu_burst, remaining_burst, time, size);
    // INITIALIZE
    for (int h = 0; h < size; h++)
    {
        lost_deadlines[h] = 0;
        complete_execution[h] = 0;
        killed_[h] = 0;
        if (period[h] > highest_p)
        {
            highest_p = period[h];
        }
    }
    // RUN
    printf("EXECUTION BY RATE\n");
    while (time < total_time)
    {

        current_p = processQueue(period, avail_list, remaining_burst, highest_p, last_p, size);

        time = processExecution(period, remaining_burst, cpu_burst, lost_deadlines, complete_execution, killed_, avail_list, name, time, current_p, total_time, size);
        //printf("\ntime: %d\n", time);
        resetProcess(period, avail_list, cpu_burst, remaining_burst, time, size);
    }

    printf("\nLOST DEADLINES\n");
    for (int i = 0; i < size; i++)
    {
        printf("[%s] %d\n", name[i], lost_deadlines[i]);
    }
    printf("\n");
    printf("COMPLETE EXECUTION\n");
    for (int i = 0; i < size; i++)
    {
        printf("[%s] %d\n", name[i], complete_execution[i]);
    }
    printf("\n");
    printf("KILLED\n");
    for (int i = 0; i < size; i++)
    {
        printf("[%s] %d\n", name[i], killed_[i]);
    }
}
int processExecution(int period[], int remaining_burst[], int cpu_burst[], int lost_deadlines[], int complete_execution[], int killed_[], int avail_list[],
                     char name[][100], int time, int current_p, int total_time, int size)
{
    int time1 = time;
    int check_remaining = 0;
    if (current_p == 0)
    {
        int t = 0;
        while (check_remaining == 0)
        {
            t++;

            resetProcess(period, avail_list, cpu_burst, remaining_burst, time1 + t, size);
            for (int i = 0; i < size; i++)
            {
                if (remaining_burst[i] > 0)
                {
                    check_remaining += 1;
                }
            }
            if (time1 + t >= total_time)
            {
                return time1 + t;
            }
        }
        printf("idle for %d units\n", t);
        return time1 + t;
    }

    for (int k = 0; k < size; k++)
    {
        if (period[k] == current_p && remaining_burst[k] > 0)
        {
            int period_time = time1 % period[k];
            int limit_time = period[k] - period_time;
            if (remaining_burst[k] <= limit_time)
            {
                int t = 0;
                int swap = 0;
                int lost_msg = 0;
                int msg_sent = 0;
                int remain = remaining_burst[k];
                for (; t < remain; t++)
                {
                    swap = processSwap(period, time1, size, current_p);
                    if (swap == 1)
                    {
                        break;
                    }
                    time1 += 1;
                    remaining_burst[k] -= 1;
                    lost_msg = lostAdd(period, lost_deadlines, remaining_burst, time1, size);
                    if (remaining_burst[k] == 0)
                    {
                        printf("[%s] for %d units - F\n", name[k], t + 1);
                        complete_execution[k] += 1;
                        avail_list[k] = 1;
                        msg_sent = 1;
                        if (!(time1 >= total_time))
                        {
                            return time1;
                        }
                    }
                    resetProcess(period, avail_list, cpu_burst, remaining_burst, time1, size);

                    if (time1 >= total_time)
                    {
                        for (int j = 0; j < size; j++)
                        {
                            if (remaining_burst[j] > 0)
                            {
                                killed_[j] += 1;
                            }
                        }
                        if (msg_sent == 1)
                        {
                            return time1;
                        }

                        else if (lost_msg == period[k])
                        {
                            printf("[%s] for %d units - L\n", name[k], t);
                        }
                        else
                        {
                            printf("[%s] for %d units - K\n", name[k], t + 1);
                        }
                        return time1;
                    }
                }

                if (lost_msg == period[k])
                {
                    printf("[%s] for %d units - L\n", name[k], t);
                }
                else
                {
                    printf("[%s] for %d units - H\n", name[k], t);
                }
                return time1;
            }
            else
            {
                int t = 0;
                int swap = 0;
                int msg_sent = 0;
                int lost_msg = 0;
                for (; t < limit_time; t++)
                {
                    swap = processSwap(period, time1, size, current_p);
                    if (swap == 1)
                    {
                        break;
                    }
                    time1 += 1;
                    remaining_burst[k] -= 1;
                    lost_msg = lostAdd(period, lost_deadlines, remaining_burst, time1, size);
                    if (remaining_burst[k] == 0)
                    {
                        printf("[%s] for %d units - F\n", name[k], t + 1);
                        complete_execution[k] += 1;
                        avail_list[k] = 1;
                        msg_sent = 1;
                        if (!(time1 >= total_time))
                        {
                            return time1;
                        }
                    }
                    resetProcess(period, avail_list, cpu_burst, remaining_burst, time1, size);

                    if (time1 >= total_time)
                    {
                        for (int j = 0; j < size; j++)
                        {
                            if (remaining_burst[j] > 0)
                            {
                                killed_[j] += 1;
                            }
                        }
                        if (msg_sent == 1)
                        {
                            return time1;
                        }

                        else if (lost_msg == period[k])
                        {
                            printf("[%s] for %d units - L\n", name[k], t);
                        }
                        else
                        {
                            printf("[%s] for %d units - K\n", name[k], t + 1);
                        }
                        return time1;
                    }
                }

                if (lost_msg == period[k])
                {
                    printf("[%s] for %d units - L\n", name[k], t);
                }
                else
                {
                    printf("[%s] for %d units - H\n", name[k], t);
                }
                return time1;

                // printf("REMAINING: %d, %d\n", burst[0], burst[1]);
            }
        }
    }
    return time1;
}

int processQueue(int period[], int avail_list[], int remaining_burst[], int highest_p, int last_p, int size)
{
    int any_available = 0;
    for (int k = 0; k < size; k++)
    {
        if (remaining_burst[k] > 0)
        {
            any_available += 1;
            // printf("REMAINING: %d, %d\n", remaining_burst[0], remaining_burst[1]);
        }
    }
    if (any_available == 0)
    {
        //  printf("REMAINING: %d, %d", remaining_burst[0], remaining_burst[1]);
        return 0;
    }
    int current_p = 0;
    for (int i = 0; i < size; i++)
    {
        // printf("Last P: %d\n", last_p);
        if (remaining_burst[i] > 0)
        {
            if (current_p == 0)
            {
                current_p = period[i];
            }
            else if ((period[i] < current_p))
            {
                {
                    current_p = period[i];
                }
            }
        }
    }
    return current_p;
}

void resetProcess(int period[], int avail_list[], int cpu_burst[], int remaining_burst[], int time, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (time % period[i] == 0)
        {
            avail_list[i] = 0;
            remaining_burst[i] = cpu_burst[i];
        }
    }
}

int processSwap(int period[], int time, int size, int current)
{
    int new_current = 0;
    for (int i = 0; i < size; i++)
    {
        if (time > 0 && time % period[i] == 0)
        {
            if (period[i] < current)
            {
                new_current = 1;
            }
        }
    }
    return new_current;
}

void deadline(int total_time, char name[][100], int period[], int cpu_burst[], int size)
{
    // COUNT
    int lost_deadlines[size];
    int complete_execution[size];
    int killed_[size];
    // VAR
    int time = 0;
    int current_p = 0;
    int avail_list[size];
    int remaining_burst[size];
    resetProcess(period, avail_list, cpu_burst, remaining_burst, time, size);
    // INITIALIZE
    for (int h = 0; h < size; h++)
    {
        lost_deadlines[h] = 0;
        complete_execution[h] = 0;
        killed_[h] = 0;
    }
    // RUN
    printf("EXECUTION BY EDF\n");
    while (time < total_time)
    {

        current_p = processQueueD(period, avail_list, remaining_burst, time, size);

        time = processExecutionD(period, remaining_burst, cpu_burst, lost_deadlines, complete_execution, killed_, avail_list, name, time, current_p, total_time, size);
        resetProcess(period, avail_list, cpu_burst, remaining_burst, time, size);
    }

    printf("\nLOST DEADLINES\n");
    for (int i = 0; i < size; i++)
    {
        printf("[%s] %d\n", name[i], lost_deadlines[i]);
    }
    printf("\n");
    printf("COMPLETE EXECUTION\n");
    for (int i = 0; i < size; i++)
    {
        printf("[%s] %d\n", name[i], complete_execution[i]);
    }
    printf("\n");
    printf("KILLED\n");
    for (int i = 0; i < size; i++)
    {
        printf("[%s] %d\n", name[i], killed_[i]);
    }
}

int processQueueD(int period[], int available_list[], int remaining_burst[], int time, int size)
{
    int any_available = 0;
    for (int k = 0; k < size; k++)
    {
        if (remaining_burst[k] > 0)
        {
            any_available += 1;
            // printf("REMAINING: %d, %d\n", remaining_burst[0], remaining_burst[1]);
        }
    }
    if (any_available == 0)
    {
        //  printf("REMAINING: %d, %d", remaining_burst[0], remaining_burst[1]);
        return 0;
    }
    int current_p = 0;
    for (int i = 0; i < size; i++)
    {
        // printf("Last P: %d\n", last_p);
        if (available_list[i] == 0)
        {
            if (current_p == 0)
            {
                current_p = period[i];
            }
            else if ((period[i] - (time % period[i])) < (current_p - (time % current_p)))
            {
                {
                    current_p = period[i];
                }
            }
        }
    }
    return current_p;
}

int lostAdd(int period[], int lost_deadlines[], int remaining_burst[], int time, int size)
{
    int lost = 0;
    for (int j = 0; j < size; j++)
    {
        if (remaining_burst[j] > 0)
        {
            if (time % period[j] == 0)
            {
                lost_deadlines[j] += 1;
                lost = period[j];
            }
        }
    }
    return lost;
}

int processExecutionD(int period[], int remaining_burst[], int cpu_burst[], int lost_deadlines[], int complete_execution[], int killed_[], int avail_list[],
                      char name[][100], int time, int current_p, int total_time, int size)
{
    int time1 = time;
    int check_remaining = 0;
    if (current_p == 0)
    {
        int t = 0;
        while (check_remaining == 0)
        {
            t++;

            resetProcess(period, avail_list, cpu_burst, remaining_burst, time1 + t, size);
            for (int i = 0; i < size; i++)
            {
                if (remaining_burst[i] > 0)
                {
                    check_remaining += 1;
                }
            }
            if (time1 + t >= total_time)
            {
                return time1 + t;
            }
        }
        printf("idle for %d units\n", t);
        return time1 + t;
    }

    for (int k = 0; k < size; k++)
    {
        if (period[k] == current_p && remaining_burst[k] > 0)
        {
            if (remaining_burst[k] <= (period[k] - (time1 % period[k])))
            {
                int t = 0;
                int swap = 0;
                int msg_sent = 0;
                int lost_msg = 0;
                int remain = remaining_burst[k];
                for (; t < remain; t++)
                {
                    swap = processSwapD(period, remaining_burst, time1, size, current_p);
                    if (swap == 1)
                    {
                        break;
                    }
                    time1 += 1;
                    remaining_burst[k] -= 1;
                    lost_msg = lostAdd(period, lost_deadlines, remaining_burst, time1, size);
                    if (remaining_burst[k] == 0)
                    {
                        printf("[%s] for %d units - F\n", name[k], t + 1);
                        complete_execution[k] += 1;
                        avail_list[k] = 1;
                        msg_sent = 1;
                        if (!(time1 >= total_time))
                        {
                            return time1;
                        }
                    }
                    resetProcess(period, avail_list, cpu_burst, remaining_burst, time1, size);
                    if (time1 >= total_time)
                    {
                        for (int j = 0; j < size; j++)
                        {
                            if (remaining_burst[j] > 0)
                            {

                                killed_[j] += 1;
                            }
                        }
                        if (msg_sent == 1)
                        {
                            return time1;
                        }

                        else if (lost_msg == period[k])
                        {
                            printf("[%s] for %d units - L\n", name[k], t);
                        }
                        else if (remaining_burst[k] > 0)
                        {
                            printf("[%s] for %d units - K\n", name[k], t + 1);
                        }
                        return time1;
                    }
                }
                if (lost_msg == period[k])
                {
                    printf("[%s] for %d units - L\n", name[k], t);
                    return time1;
                }
                else if (remaining_burst[k] == 0)
                {
                    printf("[%s] for %d units - F\n", name[k], t);
                    complete_execution[k] += 1;
                    avail_list[k] = 1;
                }
                else
                {
                    printf("[%s] for %d units - H\n", name[k], t);
                }
            }
            else
            {
                int t = 0;
                int swap = 0;
                int lost_msg = 0;
                int msg_sent = 0;
                int lmt_time = period[k] - (time1 % period[k]);
                for (; t < lmt_time; t++)
                {
                    swap = processSwapD(period, remaining_burst, time1, size, current_p);
                    if (swap == 1)
                    {
                        break;
                    }
                    time1 += 1;
                    remaining_burst[k] -= 1;
                    lost_msg = lostAdd(period, lost_deadlines, remaining_burst, time1, size);
                    if (remaining_burst[k] == 0)
                    {
                        printf("[%s] for %d units - F\n", name[k], t + 1);
                        complete_execution[k] += 1;
                        avail_list[k] = 1;
                        msg_sent = 1;
                        if (!(time1 >= total_time))
                        {
                            return time1;
                        }
                    }
                    resetProcess(period, avail_list, cpu_burst, remaining_burst, time1, size);
                    if (time1 >= total_time)
                    {
                        for (int j = 0; j < size; j++)
                        {
                            if (remaining_burst[j] > 0)
                            {

                                killed_[j] += 1;
                            }
                        }
                        if (msg_sent == 1)
                        {
                            return time1;
                        }

                        else if (lost_msg == period[k])
                        {
                            printf("[%s] for %d units - L\n", name[k], t);
                        }
                        if (remaining_burst[k] > 0)
                        {

                            printf("[%s] for %d units - K\n", name[k], t + 1);
                        }
                        return time1;
                    }
                }

                if (lost_msg == period[k])
                {
                    printf("[%s] for %d units - L\n", name[k], t);
                    return time1;
                }
                else
                {
                    printf("[%s] for %d units - H\n", name[k], t);
                }

                // printf("REMAINING: %d, %d\n", burst[0], burst[1]);
            }
        }
    }
    return time1;
}

int processSwapD(int period[], int remaining_burst[], int time, int size, int current)
{
    for (int i = 0; i < size; i++)
    {
        if (remaining_burst[i] > 0)
        {
            if ((period[i] - (time % period[i])) < (current - (time % current)))
            {
                return 1;
            }
        }
    }
    return 0;
}
