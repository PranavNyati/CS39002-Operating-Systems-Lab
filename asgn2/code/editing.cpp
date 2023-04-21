#include "editing.h"


// takes the user command as input and also maintains the history of commands
string getCmd(bool echo)
{

    printf("\n");

    struct termios old, current;
    string cmd, incomplete_cmd;
    signed char c;
    string left = "", right = "";

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    bool no_history = false;

    if ((fp = fopen("history.txt", "r")) == NULL)
    {
        // perror("History file not found. Creating a new one.");
        fp = fopen("history.txt", "w");
        fclose(fp);
        fp = fopen("history.txt", "r");
        no_history = true;
    }

    vector<string> history;

    char buff[1024];

    while (!feof(fp))
    {
        memset(buff, 0x00, 1024);
        fscanf(fp, "%[^\n]\n", buff);
        history.push_back(string(buff));
    }

    if (history.size() == 1 && history[0] == "")
        no_history = true;

    int offset = 0, typing = 1, edit_mode = 0, saved_history = history.size();
    int history_size = (no_history) ? 0 : history.size();

    tcgetattr(STDIN_FILENO, &old); // Save old terminal settings

    current = old; // Initialize new settings as old settings

    // ICANON enables canonical mode, which means that input is available line by line
    // Need to disable canonical mode to get input character by character as even getchar() waits for a newline in canonical mode
    // Our requirement is to get input character by character without waiting for a newline
    // current.c_lflag &= ~ICANON; // Disable canonical mode [i.e buffer i/o] (local mode)

    // if (echo)
    // current.c_lflag |= ECHO; // Enable echo mode (local mode)
    // else
    // current.c_lflag &= ~ECHO; // Disable echo mode (local mode)


    current.c_lflag &= (~ICANON & ~ECHO);
    current.c_cc[VMIN] = 1;
    current.c_cc[VTIME] = 0;

    tcsetattr(0, TCSANOW, &current); // Set new terminal settings immediately (now)

    while (1)
    {
        c = getchar();
        if (c == 10 || c == 13) // 10 is the ASCII code for newline
        {
            cmd = left + cmd + right;
            left = right = "";
            printf("\n");
            break;
        }

        else if (c == ESC)
        {
            // Handling arrow keys
            c = getchar(); // Skip the [
            c = getchar(); // Get the direction of the arrow key

            if (c == 65)
            {
                // Up arrow
                offset++;
                offset = offset >= history.size() ? history.size() - 1 : offset;

                if (typing && offset == 1)
                {
                    typing = 0;
                    edit_mode = 0;
                 
                    if (history.size() == saved_history)
                        history.push_back(left + cmd + right);
                    history[saved_history] = left + cmd + right;
                    left = right = "";
                }

                int len = left.size() + cmd.length() + right.size();

                for (int i = 0; i < len; i++)
                    printf("\b \b");
                if (offset < history.size())
                {
                    cmd = history[history.size() - 1 - offset];
                    for (int i = 0; i < len; i++)
                        printf("\b \b");
                    printf("%s", cmd.c_str());
                }
            }
            else if (c == 66)
            {
                // Down arrow
                offset--;
                offset = offset < 0 ? 0 : offset;

                int len = left.size() + cmd.length() + right.size();

                if (offset >= 0)
                {
                    cmd = history[history.size() - 1 - offset];

                    for (int i = 0; i < len; i++)
                        printf("\b \b");
                    printf("%s", cmd.c_str());
                }
            }
            else if (c == 67)
            {
                // Right arrow
                printf("\033[1C");
            }
            else if (c == 68)
            {
                // Left arrow
                printf("\033[1D");
            }
        }

        else if (c == BACKSPACE)
        {
            if (edit_mode == CTRL_A)
            {
                if (left.length())
                {
                    left.pop_back();
                    printf("\b \b");
                }
            }
            else if (edit_mode == CTRL_E)
            {
                if (right.length())
                {
                    right.pop_back();
                    printf("\b \b");
                }
                else if (cmd.length())
                {
                    cmd.pop_back();
                    printf("\b \b");
                }
            }
            else
            {

                if (cmd.length() > 0)
                {
                    cmd.pop_back();
                    printf("\b \b");
                }
            }
        }

        else if (c == CTRL_CZ)
        {
            printf("Terminate\n");

            return cmd;
        }

        else if (c == CTRL_A)
        {
            edit_mode = CTRL_A;

            cmd = left + cmd + right;
            left = "";
            right = "";

            for (int i = 0; i < cmd.length(); i++)
                printf("\033[1D");
        }

        else if (c == CTRL_E)
        {
            edit_mode = CTRL_E;

            cmd = left + cmd + right;
            left = "";
            right = "";

            for (int i = 0; i < cmd.length(); i++)
                printf("\033[1C");
        }

        else if (((int)c > 31) && ((int)c < 127))
        {
            typing = 1;

            int len = left.size() + cmd.size() + right.size();
            for (int i = 0; i < len; i++)
                printf("\b \b");

            if (edit_mode == CTRL_A)
            {
                left += c;
            }
            else if (edit_mode == CTRL_E)
            {
                right += c;
            }
            else
            {
                cmd += c;
            }

            cout << left << cmd << right;
            if (edit_mode == CTRL_A)
                printf("\033[%ldD", cmd.size());
        }
    }

    cmd = left + cmd + right;

    fclose(fp); // Close history file after reading

    tcsetattr(0, TCSANOW, &old); // Restore old terminal settings

    // Open history file in append mode
    if ((fp = fopen("history.txt", "a+")) == NULL)
    {
        perror("Couldn't open history file");
    }
    if (cmd.length() > 0)
        fprintf(fp, "%s\n", cmd.c_str());
    fclose(fp);

    cmd += "\n";
    return cmd;
}

char *get_cmd()
{
    string cmd = getCmd();

    // char *c_cmd = new char[cmd.length() + 1];
    char *c_cmd = (char *)malloc((cmd.length() + 1)*sizeof(char));
    strcpy(c_cmd, cmd.c_str());

    return c_cmd;
}
