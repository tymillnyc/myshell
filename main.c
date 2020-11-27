#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>


const int stringChunk = 10;
int count = 0;
int fileDesctriptorState[2] = {0};

void printMemoryError() {
    printf("OS did't gave memory... Exit");
    exit(1);
}

void cleanList(char **wordList) {
    int i = 0;
    if (wordList != NULL) {
        while (i < count) {
            if (wordList[i] != NULL) free(wordList[i]);
            ++i;
        }
        free(wordList);
    }
}

void getError(char ***wordList) {
    printf("Process execution error\n");
    cleanList(*wordList);
    exit(1);
}

int logicalMetaToken(char *wordList) {
    int specialCharacterFlag;
    if (strcmp(wordList, ">") == 0)
        specialCharacterFlag = 1;
    else if (strcmp(wordList, ">>") == 0)
        specialCharacterFlag = 1;
    else if (strcmp(wordList, "<") == 0)
        specialCharacterFlag = 1;
   else specialCharacterFlag = 0;
   return specialCharacterFlag;
}

int checkCharactersIO(char **wordList) {
    int i = 0, charactersIOflag = 0;
    while (wordList[i] != NULL) {
        if (logicalMetaToken(wordList[i])) {
            charactersIOflag = 1;
            break;
        }
        ++i;
    }
    return (charactersIOflag);
}

int checkCharacterPipe(char **wordList) {
    int characterOrFlag = 0, i = 0;
    while ((wordList[i] != NULL) && (characterOrFlag == 0)) {
        if (strcmp(wordList[i], "|") == 0) characterOrFlag = 1;
        i++;
    }
    return characterOrFlag;
}

void getLineCount(int *countBuf, char **wordList) {
    *countBuf = 0;
    int i = 0;
    while (wordList[i] != NULL) {
        ++i;
    }
    *countBuf = i;
}

void countNumberCharactersIO(int *characterInput, int *characterInput1, int *characterInput2, char **wordList) {
    int i = 0;
    while (wordList[i] != NULL) {
        if (strcmp(wordList[i], ">") == 0) ++(*characterInput1);
        else if (strcmp(wordList[i], ">>") == 0) ++(*characterInput2);
        else if (strcmp(wordList[i], "<") == 0) ++(*characterInput);
        ++i;
    }
}

int redirectIO(char **wordList, int *posIO) {
    int characterInput = 0;
    int characterInput1 = 0;
    int characterInput2 = 0;
    int characterIOFlag = 0;
    int i = 0;
    countNumberCharactersIO(&characterInput, &characterInput1, &characterInput2, wordList);
    while ((characterInput > 0) || (characterInput1 > 0) || (characterInput2 > 0)) {
        int fd = 0;
        if (strcmp(wordList[i], ">") == 0) {
            characterInput1 -= 1;
            if (wordList[i + 1] != NULL) {
                fd = open(wordList[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            }
            if ((fd == -1) || (wordList[i + 1] == NULL)) {
                getError(&wordList);
            }
            characterIOFlag = 1;
            dup2(fd, 1);
            close(fd);
        } else if (strcmp(wordList[i], ">>") == 0) {
            characterInput2 -= 1;
            if (wordList[i + 1] != NULL) {
                fd = open(wordList[i + 1], O_APPEND | O_CREAT | O_WRONLY, 0666);
            }
            if ((fd == -1) || (wordList[i + 1] == NULL)) {
                getError(&wordList);
            }
            characterIOFlag = 1;
            dup2(fd, 1);
            close(fd);

        } else if (strcmp(wordList[i], "<") == 0) {
            characterInput -= 1;
            if (wordList[i + 1] != NULL) {
                fd = open(wordList[i + 1], O_RDONLY);
            }
            if ((fd == -1) || (wordList[i + 1] == NULL)) {
                getError(&wordList);
            }
            characterIOFlag = 1;
            dup2(fd, 0);
            close(fd);
        }
        ++i;
    }
    if ((wordList[i + 1] != NULL) && (posIO != NULL)) {
        (*posIO) = i + 1;
    }
    return characterIOFlag;
}

void executeCommand(char **wordList, int command, int *fd, int characterCount, int commandFlag, int modeFlag) {
    int errorFlag = 0;
    if (strcmp(wordList[command], "cd") == 0) {
        if (wordList[command + 1] == NULL) {
            errorFlag = chdir(getenv("HOME"));
        } else
            errorFlag = chdir(wordList[command + 1]);
        if (errorFlag == -1) {
            printf("Directory not found %s\n", wordList[command + 1]);
            errorFlag = 1;
            write(fileDesctriptorState[1], &errorFlag, sizeof(int));
        }
        printf("The current directory is %s\n", wordList[command + 1]);
        errorFlag = 0;
        write(fileDesctriptorState[1], &errorFlag, sizeof(int));

    } else {
        int pid;
        pid = fork();
        if (pid == 0) {
            if (modeFlag == 0) {
                signal(SIGINT, SIG_DFL);
            }
            if (commandFlag == 1) {
                if (characterCount >= 0) dup2(fd[1], 1);
                close(fd[0]);
                close(fd[1]);
            }
            if (commandFlag == 0) {
                if (checkCharactersIO(wordList) == 0) {
                    errorFlag = execvp(wordList[0], wordList);
                } else {
                    if ((strcmp((wordList[0]), ">") == 0) || (strcmp(wordList[0], ">>") == 0) ||
                        (strcmp(wordList[0], "<") == 0)) {
                        int posIO = 0;
                        redirectIO(wordList, &posIO);

                        if (posIO != 0) {
                            errorFlag = execvp(wordList[posIO], &(wordList[posIO]));
                        } else errorFlag = -1;
                    } else {
                        int i = 0;
                        while ((strcmp((wordList[i]), ">") != 0) && (strcmp(wordList[i], ">>") != 0) &&
                               (strcmp(wordList[i], "<") != 0)) {
                            ++i;
                        }
                        redirectIO(wordList, NULL);
                        wordList[i] = NULL;

                        errorFlag = execvp(wordList[0], wordList);
                    }
                }
            } else {
                errorFlag = execvp(wordList[command], wordList + (command * sizeof(char)));
            }
            if (errorFlag == -1) {
                if (commandFlag == 1) {
                    kill(getppid(), SIGUSR1);
                } else
                    printf("Wrong operation\n");
                exit(1);
            }
        } else {
            if (commandFlag == 1) {
                dup2(fd[0], 0);
                close(fd[0]);
                close(fd[1]);
            }
            if (pid == -1) {
                printf("Error\n");
                if (commandFlag == 1) kill(getppid(), SIGUSR1);
            }
            int status, result;
            waitpid(pid, &status, 0);
            result = WIFEXITED(status);
            if ((characterCount < 0) || (commandFlag == 0)) {
                if (result != 0) {
                    if (WEXITSTATUS(status)) result = 1;
                    else result = 0;
                    write(fileDesctriptorState[1], &result, sizeof(int));
                } else {
                    result = 1;
                    write(fileDesctriptorState[1], &result, sizeof(int));
                }
            }
        }
    }
}

void sigusr1Handler(int s) {
    printf("The operation will fail\n");
}

void startConveyor(char **wordList, int processFlag) {
    int pid, i = 0, characterOrCount = 0, characterIOFlag = 0;
    int positionIO = 0, fileDesctriptor[2], position;
    if ((pid = fork()) == 0) {
        if (processFlag == 0) signal(SIGINT, SIG_DFL);
        if (checkCharactersIO(wordList)) {
            characterIOFlag = redirectIO(wordList, &positionIO);
        }
        position = positionIO;
        while (wordList[i] != NULL) {
            if (strcmp(wordList[i], "|") == 0) characterOrCount += 1;
            ++i;
        }
        if (characterOrCount != 0 && characterIOFlag == 1) {
            printf("You did not set the conveyor correctly(0)\n");

            exit(1);
        }
        if (strcmp(wordList[position], "|") == 0) {
            printf("You did not set the conveyor correctly(1)\n");
            exit(1);
        }
        while (position <= count) {
            i = 0;
            if (characterOrCount > 0) {
                while (strcmp(wordList[position + i], "|") != 0) {
                    ++i;
                }
                if ((position + i) == (count - 1)) {
                    printf("You did not set the conveyor correctly(2)\n");
                    break;
                }
                free(wordList[position + i]);
                wordList[position + i] = NULL;
                characterOrCount -= 1;
            } else {
                while (wordList[position + i] != NULL) {
                    ++i;
                }
                characterOrCount -= 1;
            }
            pipe(fileDesctriptor);
            signal(SIGUSR1, sigusr1Handler);
            executeCommand(wordList, position, fileDesctriptor, characterOrCount, 1, processFlag);
            position = position + i + 1;
        }
        while (wait(NULL) != -1);
        exit(0);
    } else if (pid == -1) {
            printf("Сonveyor failed\n");
        }
        else {
            wait(NULL);
        }

}

void checkСonveyorOperation(char **wordList, int processFlag) {
    getLineCount(&count, wordList);
    int pipeCount = checkCharacterPipe(wordList);
    if (processFlag) {
        if (pipeCount) {
            startConveyor(wordList, 1);
        } else {
            executeCommand(wordList, 0, NULL, 0, 0, 1);
        }
    } else {
        if (pipeCount) {
            startConveyor(wordList, 0);
        } else {
            executeCommand(wordList, 0, NULL, 0, 0, 0);
        }
    }
}

void getBackgroundMode(char **wordList, int wordsCount) {
    int pid;
    if (fork() == 0) {
        pid = fork();
        if (pid == 0) {
            int fd;
            fd = open("/dev/null", O_RDONLY);
            dup2(fd, 0);
            close(fd);
            while (getppid() != 1);
            getLineCount(&count, wordList);
            checkСonveyorOperation(wordList, 1);
            for (int i = 0; i < 10; i++) {
                printf("%d\n", i);
                sleep(1);
            }
            exit(1);
        } else {
            if (pid == -1)
                printf("Error\n");
            exit(1);
        }
    } else while (wait(NULL) != -1);
}

int checkOperations(char **wordList) {
    int i = 0, operationsCount = 0;
    while (wordList[i] != NULL) {
        if ((strcmp(wordList[i], "&") == 0) || (strcmp(wordList[i], ";") == 0)) {
            ++operationsCount;
        }
        ++i;
    }
    if ((strcmp(wordList[i - 1], "&") == 0) || (strcmp(wordList[i - 1], ";") == 0)) return (operationsCount - 1);
    else return (operationsCount);
}

void parseDifferentActions(char **wordList, int processFlag) {
    int i = 0, nextLine = 0, specialCharactersFlag = 0, specialCharactersCount = 0;
    int state = 0, statePrevios, stateNow = 0;
    while (wordList[i] != NULL) {
        if ((strcmp(wordList[i], "&&") == 0) || (strcmp(wordList[i], "||") == 0)) {
            if (wordList[i + 1] == NULL) {
                specialCharactersFlag = 0;
                free(wordList[i]);
                wordList[i] = NULL;
                break;
            }
            specialCharactersFlag = 1;
            specialCharactersCount += 1;
        }
        ++i;
    }
    i = 0;
    if (specialCharactersFlag == 0) {
        if (processFlag == 1) checkСonveyorOperation(wordList, 1);
        else checkСonveyorOperation(wordList, 0);
        read(fileDesctriptorState[0], &state, sizeof(int));
    } else {
        while (specialCharactersCount >= 0) {
            statePrevios = stateNow;
            while (wordList[i] != NULL) {
                if ((strcmp(wordList[i], "&&") == 0) || (strcmp(wordList[i], "||") == 0)) break;
                else ++i;
            }
            if (wordList[i] == NULL) specialCharactersCount -= 1;
            else {
                if (strcmp(wordList[i], "&&") == 0) {
                    specialCharactersCount -= 1;
                    free(wordList[i]);
                    wordList[i] = NULL;
                    stateNow = 0;
                } else {
                    specialCharactersCount -= 1;
                    free(wordList[i]);
                    wordList[i] = NULL;
                    stateNow = 1;
                }
            }

            if (state == statePrevios) {
                if (processFlag == 1) checkСonveyorOperation(&(wordList[nextLine]), 1);
                else checkСonveyorOperation(&(wordList[nextLine]), 0);
                read(fileDesctriptorState[0], &state, sizeof(int));
            }
            i += 1;
            nextLine = i;
        }
    }

}


void checkBackgroundMode(char **wordList, int wordsCount) {
    int operationsCount, nextLine = 0, modeFlag = 0, errorFlag = 0;
    pipe(fileDesctriptorState);
    operationsCount = checkOperations(wordList);
    for (int i = 0; i < operationsCount + 1; i++) {
        int j = nextLine;
        modeFlag = 0;
        while (wordList[j] != NULL) {
            if ((strcmp(wordList[j], ";") == 0) || (strcmp(wordList[j], "&") == 0)) {
                if (strcmp(wordList[j], "&") == 0) {
                    free(wordList[j]);
                    wordList[j] = NULL;
                    if ((errorFlag = checkCharacterPipe(&(wordList[j + 1])))) break;
                    if (wordList[nextLine] != NULL) {
                        getBackgroundMode(&(wordList[nextLine]), wordsCount);
                    }
                    nextLine = j + 1;
                } else {
                    free(wordList[j]);
                    wordList[j] = NULL;
                    if (wordList[nextLine] != NULL) parseDifferentActions(&(wordList[nextLine]), 0);
                    nextLine = j + 1;
                }
                modeFlag = 1;
                break;
            }
            ++j;
        }
        if (errorFlag == 1) {
            printf("Error when working with a conveyor\n");
            break;
        }
        if (modeFlag == 0) {
            parseDifferentActions(&(wordList[nextLine]), 0);
        }
    }
    close(fileDesctriptorState[1]);
    close(fileDesctriptorState[0]);
}


int logicCheckCharacters(char nextCharacter) {
    int specialCharacterFlag;
    if (nextCharacter == '&')
         specialCharacterFlag = 1;
    else if (nextCharacter == '|')
        specialCharacterFlag = 1;
    else if (nextCharacter == '>')
        specialCharacterFlag = 1;
    else specialCharacterFlag = 0;
    return specialCharacterFlag;

}

int checkSpetialChars(int character) {
    int specialCharacterFlag;
    if (character == ';') {
        specialCharacterFlag = 1;
    }
   else if (character == '(') {
        specialCharacterFlag = 1;
    } else if (character == ')') {
        specialCharacterFlag = 1;
        }
    else if (character == '<') {
        specialCharacterFlag = 1;
        }
    else specialCharacterFlag = 0;
return specialCharacterFlag;

}

int checkControlWords(char nextCharacter, char nnCharacter, int flag) {
   int specialCharacterFlag;
    if (logicCheckCharacters(nextCharacter) && flag == 0)
        specialCharacterFlag = 1;
    else if (logicCheckCharacters(nnCharacter)) {
        if (nextCharacter == nnCharacter)
            specialCharacterFlag = 1;
    }
   else if (nnCharacter == '\n') {
        if (checkSpetialChars(nextCharacter))
            specialCharacterFlag = 2;
    }
    else specialCharacterFlag = 0;
    return specialCharacterFlag;
}

int isDelimiter(char nextCharacter) {
    if (nextCharacter == ' ')
        return 1;
    else return 0;
}

int searchQuotes(int i, char *inputString) {
    for (; i < strlen(inputString); i++) {
        if (inputString[i] == '"')
            return 0;
    }
    return 1;
}

int getWords(char *inputString) {
    char **wordList = NULL;
    int wordsCount = 0, charactersCount = 0, isNewWord = 1;
    int lengthString = strlen(inputString);
    int quotesParityOnFlag = 1, quotesCount = 0, logicFlag;
    int wordsControlForFlag = 0, controlWordsFlag, characterSpecialFlag = 0;
    char nextCharacter, nextNextCharacter;
    for (int i = 0; i < lengthString; i++) {
        nextCharacter = inputString[i];
        controlWordsFlag = checkControlWords(nextCharacter, '\n', 0);
        if ((controlWordsFlag == 1) && characterSpecialFlag == 0) {
            nextNextCharacter = inputString[i + 1];
            wordsControlForFlag = checkControlWords(nextCharacter, nextNextCharacter, 1);
        }
        logicFlag = (wordsControlForFlag == 1 || controlWordsFlag == 1 || controlWordsFlag == 2);
        if (logicFlag && characterSpecialFlag == 0) {
            if (wordsCount > 0) {
                wordList[wordsCount - 1][charactersCount] = '\0';
            }
            wordsCount++;
            wordList = realloc(wordList, (wordsCount) * sizeof(char *));
            if (wordList == NULL) {
                printMemoryError();
            }
            wordList[wordsCount - 1] = malloc(lengthString * sizeof(char));
            if (wordList[wordsCount - 1] == NULL) {
                printMemoryError();
            }
            charactersCount = 0;
            if (wordsControlForFlag == 1) {
                wordList[wordsCount - 1][0] = nextCharacter;
                wordList[wordsCount - 1][1] = nextNextCharacter;
                i++;
                charactersCount = 2;
                wordsControlForFlag = 0;
                isNewWord = 1;
                continue;
            }
            wordList[wordsCount - 1][charactersCount] = nextCharacter;
            charactersCount++;
            isNewWord = 1;
            continue;
        }
        if (nextCharacter == '"') {
            characterSpecialFlag = 1;
            if (quotesCount == 2) {
                quotesCount = 0;
            }
            quotesCount++;
            if (quotesCount == 2) {
                quotesParityOnFlag = 1;
                characterSpecialFlag = 0;
                quotesCount = 0;
                continue;
            }
            if (nextCharacter == (nextNextCharacter = inputString[i + 1])) {
                quotesCount++;
                i = i + 1;
                continue;
            }
            quotesParityOnFlag = searchQuotes(i + 1, inputString);
            if (quotesParityOnFlag == 0) {
                nextCharacter = inputString[++i];
            }
        }
        if (isDelimiter(nextCharacter) == 1 && quotesParityOnFlag == 1) {
            isNewWord = 1;
            continue;
        }
        if (isDelimiter(nextCharacter) == 0 && isNewWord == 1) {
            if (wordsCount > 0) {
                wordList[wordsCount - 1][charactersCount] = '\0';
            }
            wordsCount++;
            wordList = realloc(wordList, (wordsCount) * sizeof(char *));
            if (wordList == NULL) {
                printMemoryError();
            }
            wordList[wordsCount - 1] = malloc(lengthString * sizeof(char));
            if (wordList[wordsCount - 1] == NULL) {
                printMemoryError();
            }
            isNewWord = 0;
            charactersCount = 0;
        }
        if (isNewWord == 0) {
            wordList[wordsCount - 1][charactersCount] = nextCharacter;
            charactersCount++;

        }
    }
    if (wordsCount > 0) {
        wordList[wordsCount - 1][charactersCount] = '\0';
    }
    if (wordList != NULL) {
        wordsCount++;
        wordList = realloc(wordList, (wordsCount) * sizeof(char *));
        wordList[wordsCount - 1] = NULL;
        checkBackgroundMode(wordList, wordsCount);
    }
    count = wordsCount;
    cleanList(wordList);
    if (quotesCount == 1) return 1;
    return 0;
}

char *getInputString(void) {
    int length = stringChunk;
    int stringLength, position = 0;
    char *inputString = malloc(stringChunk * sizeof(char));
    if (inputString == NULL) {
        printMemoryError();
    }
    while (fgets(inputString + position, stringChunk, stdin)) {
        stringLength = strlen(inputString);
        if (inputString[stringLength - 1] != '\n') {
            position = stringLength;
            length += stringChunk;
            inputString = realloc(inputString, length * sizeof(char));
            if (inputString == NULL) {
                printMemoryError();
            }
        } else {
            inputString[stringLength - 1] = '\0';
            return inputString;
        }
    }
    free(inputString);
    return NULL;
}

int getLines() {
    char *inputString = NULL;
    int quotesOnFlag = 0;
    while (!feof(stdin)) {
        printf("Entered command: \n");
        inputString = getInputString();
        if (inputString == NULL) return 1;

        quotesOnFlag = getWords(inputString);
        free(inputString);
        if (quotesOnFlag == 1) {
            printf("Wrong number of quotes. Continue...\n");
            continue;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, SIG_IGN);
    int memoryOnflag = 0, fd, inputFlag;
    if (argc > 2) {
        printf("Wrong number of arguments\n");
        exit(1);
    }
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            printf("File did not open. Want to continue typing from standard input? 1 - yes\n");
            inputFlag = getchar();
            if (inputFlag != '1')
                exit(2);
            getchar();
        } else {
            dup2(fd, 0);
            close(fd);
        }
    }
    memoryOnflag = getLines();
    if (memoryOnflag == 0)
        return 1;

    return 0;
}
