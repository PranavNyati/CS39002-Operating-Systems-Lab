
#include "sb.h"

using namespace std;

int getGroup(int pid)
{
    char procname[32];
    FILE *fp;

    snprintf(procname, sizeof(procname), "/proc/%u/stat", pid);
    fp = fopen(procname, "r");

    if (fp == NULL)
    {
        cout << "could not open the stat file " << procname << endl;
        return 0;
    }

    char c;
    int group_f = 0;
    int gpid = 0;

    while ((c = fgetc(fp)) != EOF)
    {
        if (c == ' ')
            group_f++;

        if (group_f == 4)
        {
            while ((c = fgetc(fp)) != EOF)
            {
                if (c == ' ')
                    break;
                gpid = gpid * 10 + (int)(c - '0');
            }
            break;
        }
    }
    fclose(fp);
    return gpid;
}

int getParent(int pid, char *process)
{
    char procname[32];
    FILE *fp;

    snprintf(procname, sizeof(procname), "/proc/%u/stat", pid);
    cout << procname << endl;
    fp = fopen(procname, "r");

    if (fp == NULL)
    {
        cout << "could not open the stat file " << procname << endl;
        return 0;
    }

    char c;
    int parent_pid_f = 0;
    int process_f = 0;
    int ppid = 0;

    while ((c = fgetc(fp)) != EOF)
    {
        if (c == ' ')
        {
            parent_pid_f++;
            process_f++;
        }
        if (process_f == 1)
        {
            if (c == '(' || c == ' ')
                continue;
            if (c == ')')
            {
                process_f++;
                continue;
            }
            *process++ = c;
        }
        if (parent_pid_f == 3)
        {
            while ((c = fgetc(fp)) != EOF)
            {
                if (c == ' ')
                    break;
                ppid = ppid * 10 + (int)(c - '0');
            }
            break;
        }
    }
    fclose(fp);
    return ppid;
}

pair<int, int> getRoot(int pid, int suggest)
{
    char *process = (char *)malloc(32 * sizeof(char));
    char *parentProcess = (char *)malloc(32 * sizeof(char));

    int parent = getParent(pid, process);
    int grandParent = getParent(parent, parentProcess);

    // Base case
    if (parent <= 1 || (strcmp(process, parentProcess) != 0 && suggest))
    {
        free(process);
        free(parentProcess);
        // orphaned child: adopted by init
        int gpid = getGroup(pid);
        return {pid, gpid};
    }

    free(process);
    free(parentProcess);
    // recursively call
    return getRoot(parent, suggest);
}