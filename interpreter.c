#include "interpreter.h" 
#include "shellmemory.h" 
#include "kernel.h"
#include "memorymanager.h"

#include "ram.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


#define NUM_EXEC 3 // number of programs we're allowed to run at the same time

/**
 *  takes a raw input (which is malloc'd), checks how many words there are in it, turns the tokens in place into
 * strings.
 * 
 * malloc's a return array ret_arr, each cell of ret_arr pointing to the 
 * previously malloc'd address of the raw input.
 * 
 * means we need to free the returned array, as well as eventually the raw input
 * 
 * we free the raw input in shell.c 
 * 
 * we free the returned array in 
 * 
*/
char **tokenize(char *str)
{
    //count the number of tokens
    size_t num_tokens = 1;
    int flag = 0;
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (flag == 0 && str[i] == ' ')
        {
            num_tokens = num_tokens + 1;
            flag = 1;
        }
        if (str[i] != ' ')
        {
            flag = 0;
        }
    }
    //malloc an array to hold all the tokens, which we return
    char **ret_arr =
        (char **)malloc(sizeof(char *) * (num_tokens + 1));

    if (ret_arr == NULL)
    {
        perror("malloc");
        return NULL;
    }

    flag = 0;
    int ignore_flag = 0;
    char *modified_str = (char *)str; //point modified_str to contents of str
    size_t counter = 0;
    const size_t length_str = strlen(str);
    for (size_t i = 0; i < length_str; i++)
    {
        if (modified_str[i] == '\n' || modified_str[i] == '\r')
            modified_str[i] = ' ';
        if (modified_str[i] == '"')
        {
            ignore_flag = ignore_flag ^ 0x1;
        }
        if (flag == 0 && modified_str[i] != ' ')
        {
            ret_arr[counter] = &(modified_str[i]); // point to the address of i'th token
            counter = counter + 1;
            flag = 1;
        }
        if (modified_str[i] == '\\' && modified_str[i + 1] == ' ')
        {
            i++;
            continue;
        }
        if (flag == 1 && modified_str[i] == ' ' && ignore_flag == 0)
        {
            modified_str[i] = '\0';
            flag = 0;
            continue;
        }
    }
    ret_arr[counter] = NULL;

    for (size_t i = 0; i < counter; ++i) //
    {
        if (ret_arr[i][0] == '\"' &&
            ret_arr[i][strlen(ret_arr[i] - 1)] == '\"')
        {
            ret_arr[i][strlen(ret_arr[i]) - 1] = '\0';
            ret_arr[i] = ret_arr[i] + 1;
        }
    }

    return ret_arr; 
}

int in_file_flag = 0;
int interpret(char *raw_input);

int help()
{
    printf(""
           "COMMAND         DESCRIPTION\n"
           "help            Displays all the commands\n"
           "quit            Exits / terminates the shell with \"Bye!\"\n"
           "set VAR STRING  Assigns a value to shell memory\n"
           "print VAR       Displays the STRING assigned to VAR\n"
           "run SCRIPT.TXT  Executes the file SCRIPT.TXT\n"
           "exec <FILES>    Executes the (up to 3) <FILES>\n");
    return 0;
}

int quit()
{
    printf("Bye!\n");
    if (in_file_flag == 0)
    {
        quitCleanup();
        exit(0);
    }
    quit_state=1;
    return 0;
}

int run(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        printf("Script not found.\n");
        return 1;
    }
    int enter_flag_status = in_file_flag;
    in_file_flag = 1;
    while (!feof(file))
    {
        char *line = NULL;
        size_t linecap = 0;
        getline(&line, &linecap, file);

        int status = interpret(line);
        free(line);
        if (status != 0)
        {
            break;
            return status;
        }
        if(quit_state){
            break;
        }
    }
    fclose(file);
    quit_state = initial_quit_state;
    in_file_flag = enter_flag_status;
    return 0;
}

int set(const char *key, const char *value)
{
    int status = shell_memory_set(key, value);
    if (status != 0)
        printf("set: Unable to set shell memory.\n");
    return status;
}

int print(const char *key)
{
    const char *value = shell_memory_get(key);
    if (value == NULL)
    {
        printf("print: Undefiend value.\n");
        return 1;
    }
    printf("%s\n", value);
    return 0;
}

int exec(char* filenames []){
    int result = 0;
    int i = 0;

    while (filenames[i] != NULL && i < NUM_EXEC)
    {
        //open the program file 
        FILE *fp = fopen(filenames[i], "r"); // this is fclosed in memorymanager.launcher
        if(fp ==NULL){
            printf("Problem opening %s, command aborted :(\n", filenames [i]);
            return 1;
        }
        int launchResult = launcher (fp, filenames [i]);
        if (launchResult!=0) return 1;       //launch fails
        numProcesses++;
        i++;
    }
    result = scheduler();
    return result;
}

int interpret(char *raw_input)
{
    char **tokens = tokenize(raw_input);

    if (tokens[0] == NULL)
        return 0; // empty command

    if (strcmp(tokens[0], "help") == 0)
    {
        if (tokens[1] != NULL)
        {
            printf("help: Malformed command\n");
            free(tokens);
            return 1;
        }
        free(tokens);
        return help();
    }

    if (strcmp(tokens[0], "quit") == 0)
    {
        if (tokens[1] != NULL)
        {
            printf("quit: Malformed command\n");
            free(tokens);
            return 1;
        }
        //free(raw_input); <-- this is not necessary
        free(tokens);
        return quit();
    };

    if (strcmp(tokens[0], "set") == 0)
    {
        if (!(tokens[1] != NULL && tokens[2] != NULL && tokens[3] == NULL))
        {
            printf("set: Malformed command\n");
            free(tokens);
            return 1;
        }
        int status = set(tokens[1], tokens[2]);
        free(tokens);
        return status;
    }

    if (strcmp(tokens[0], "print") == 0)
    {
        if (!(tokens[1] != NULL && tokens[2] == NULL))
        {
            printf("print: Malformed command\n");
            free(tokens);
            return 1;
        }
        int status = print(tokens[1]);
        free(tokens);
        return status;
    }

    if (strcmp(tokens[0], "run") == 0)
    {
        if (!(tokens[1] != NULL && tokens[2] == NULL))
        {
            printf("run: Malformed command\n");
            free(tokens);
        }
        int result = run(tokens[1]);
        free(tokens);
        return result;
    }

    if (strcmp(tokens[0], "exec") == 0)
    {     
        int result = 0;
        
        if (!(tokens[1]!=NULL))
        {
            printf("exec: Malformed command\n");
            free(tokens);
            result = 1;
            return result;
        }
        //load only the tokens representing file names into another array of strings
        char* fileNames [NUM_EXEC] = {NULL};
        int i = 1;
        while (i <= NUM_EXEC  && tokens[i]!=NULL){
            fileNames[i-1] = strdup(tokens[i]); // this gets freed a few lines later
            i++;
        }
        result = exec(fileNames);
        //free the filenames array
        for(int j = 0; j < NUM_EXEC; j++){
            free(fileNames[j]);        
            fileNames[j] = NULL;
        } 
        free(tokens);
        return result;
    }

    printf("Unrecognized command \"%s\"\n", tokens[0]);
    free(tokens);
    return 1;
}