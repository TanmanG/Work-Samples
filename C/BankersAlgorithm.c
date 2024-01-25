#include <stdio.h>
#include <malloc.h>
#include <string.h>

int resourceCount; // The number of resources allocated.
int processCount; // The number of processes allocated.

int* resources; // Array representing the total qty of each resource.
int* available; // Array representing the available qty of each resource.

int** maxClaim; // The max number of each resource (column) each process (row) will need.
int** allocated; // The current number of each resource (column) each process (row) is using.
int** needed; // The current number of each resource (column) each process (row) still needs.


/***************************************************************/
void PrintResources()
{
    // Declare variables
    int resourceIndex;

    // Print the table header
    printf("\n\tUnits\tAvailable\n------------------------\n");
    // Print each row
    for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
    {
        printf("r%d\t%d\t%d\n", resourceIndex, resources[resourceIndex], available[resourceIndex]);
    }
}

void PrintProcesses()
{
    // Declare variables
    int resourceIndex;
    int processIndex;
    int index;
    char* printString;
    char buffer[100];
    int** table;

    // Compute how many columns (tabs) we'll need for each category
    printString = malloc(sizeof(char) * 1024);
    printString[0] = '\0';
    for (processIndex = 0; processIndex <= resourceCount; processIndex++)
        printString = strcat(printString, "\t");

    // Build the upper table header
    printf("\n\tMax%s%s%s%s", printString, "Current", printString, "Potential\n");

    // Build the lower table header
    // Start the new line
    free(printString);
    printString = malloc(sizeof(char) * 1024);
    printString[0] = '\0';
    // Construct each block of headers
    for (index = 0; index < 4 * resourceCount; index++)
    {
        // Check whether we're in a separator column
        if (index % 4 == 0)
        {
            // Add a separator
            printString = strcat(printString, "\t");
        }
        else
        {
            // Add a resource-number string and a separator, using modulo to keep proper count of r
            sprintf(buffer, "r%d\t", index % 4 - 1);
            strcat(printString, buffer);
        }
    }
    // Output the lower table header
    printf(printString);

    // Build the divider
    free(printString);
    printString = malloc(sizeof(char) * 1024);
    printString[0] = '\n';
    printString[1] = '\0';
    for (processIndex = 0; processIndex <= 8 * 4 * resourceCount + 1; processIndex++)
        printString = strcat(printString, "-");
    // Output the divider
    printf(printString);

    // Build the table
    for (processIndex = 0; processIndex < processCount; processIndex++)
    {
        // Start the new line
        free(printString);
        printString = malloc(sizeof(char) * 1024);
        printString[0] = '\0';
        // Append the process number
        sprintf(buffer, "\np%d", processIndex);
        strcat(printString, buffer);

        // Update the read table
        table = maxClaim;

        // Append each column
        for (index = 0; index < 3; index++)
        {
            if (index == 0) table = maxClaim;
            if (index == 1) table = allocated;
            if (index == 2) table = needed;

            // Append each resource column
            for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
            {
                // Shift the tab, then print the value
                sprintf(buffer, "\t%d", table[processIndex][resourceIndex]);
                strcat(printString, buffer);
            }
            // Shift the empty column
            strcat(printString, "\t");
        }

        // Print the row and move to the next one
        printf(printString);
    }
}

void TakeParameters()
{
    // Declare variables
    int processIndex;
    int resourceIndex;

    int isInputBad = 0;

    // Take the number of processes
    do
    {
        isInputBad = 0;

        printf("Enter number of processes: ");
        scanf("%d", &processCount);

        // Error Checking
        if (processCount <= 0)
        {
            // Print the error
            printf("ERROR: Number of processes must be at least 1!\n");
            // Restart this question
            isInputBad = 1;
        }
        // Clear the input
        fflush(stdin);
    } while (isInputBad);

    // Take the number of resources
    do
    {
        isInputBad = 0;

        printf("Enter number of resources: ");
        scanf("%d", &resourceCount);

        // Error Checking
        if (resourceCount <= 0)
        {
            // Print the error
            printf("ERROR: Number of processes must be at least 1!\n");
            // Restart this question
            isInputBad = 1;
        }
        // Clear the input
        fflush(stdin);
    } while (isInputBad);


    // Instantiate each array and matrices
    resources = malloc(resourceCount * sizeof(int));
    available = malloc(resourceCount * sizeof(int));

    // Instantiate the rows
    maxClaim = (int**) malloc(processCount * sizeof(int*));
    allocated = (int**) malloc(processCount * sizeof(int*));
    needed = (int**) malloc(processCount * sizeof(int*));
    // Instantiate the columns
    for (processIndex = 0; processIndex < processCount; processIndex++)
    {
        maxClaim[processIndex] = (int*) malloc(resourceCount * sizeof(int));
        allocated[processIndex] = (int*) malloc(resourceCount * sizeof(int));
        needed[processIndex] = (int*) malloc(resourceCount * sizeof(int));
    }

    // Take the max number of each resource
    do
    {
        isInputBad = 0;

        printf("Enter number of units for resources (r0 to r%d): ", processCount - 1);

        for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
        {
            scanf("%d", &resources[resourceIndex]);
            available[resourceIndex] = resources[resourceIndex];

            // Error Checking
            if (resources[resourceIndex] <= 0)
            {
                // Print the error
                printf("ERROR: All resources must be at least 1!\n");
                // Restart this question
                isInputBad = 1;
                // Clear the input
                fflush(stdin);
                // Break
                break;
            }
        }

        // Clear the input
        fflush(stdin);
    } while (isInputBad);


    // For each Process i, take the max use for each resource j (space delineated)
    for (processIndex = 0; processIndex < processCount; processIndex++)
    {
        do
        {
            isInputBad = 0;

            printf("Enter maximum number of units process p%d will request from each resource (r0 to r%d) ",
                   processIndex, processCount - 1);

            // Store the max usage
            for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
            {
                // Store the max
                scanf("%d", &maxClaim[processIndex][resourceIndex]);

                // Error Checking
                if (maxClaim[processIndex][resourceIndex] < 0)
                {
                    // Print the error
                    printf("ERROR: Each resource max must be greater than or equal to 0!\n");
                    // Restart this question
                    isInputBad = 1;
                }

                if (isInputBad)
                {
                    // Clear the input
                    fflush(stdin);
                    // Break
                    break;
                }
            }

            // Clear the input
            fflush(stdin);
        } while (isInputBad);
    }


    // For each Process i, take the current use for each resource j (space delineated)
    for (processIndex = 0; processIndex < processCount; processIndex++)
    {
        do
        {
            isInputBad = 0;

            printf("Enter number of units of each resource (r0 to r%d) allocated to process p%d: ",
                   processCount - 1, processIndex);

            // Store the current usage
            for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
            {
                // Store the current allocated resources
                scanf("%d", &allocated[processIndex][resourceIndex]);
                // Compute and store the needed resources
                needed[processIndex][resourceIndex] = maxClaim[processIndex][resourceIndex] - allocated[processIndex][resourceIndex];
                // Update the available resources
                available[resourceIndex] -= allocated[processIndex][resourceIndex];

                // Error Checking
                if (allocated[processIndex][resourceIndex] < 0)
                {
                    // Print the error
                    printf("ERROR: Each resource use must be greater than or equal to 0!\n");
                    // Restart this question
                    isInputBad = 1;
                }

                // Flush and break if the input was bad
                if (isInputBad)
                {
                    // Clear the input
                    fflush(stdin);
                    // Break
                    break;
                }
            }

            // Clear the input
            fflush(stdin);
        } while (isInputBad);
    }

    // Print the resource table
    PrintResources();

    // Print the process table
    PrintProcesses();
}

void FindSafeSequence()
{
    // Declare variables
    int processIndex;
    int resourceIndex;

    int* completed;
    int* availableLocal;

    int allCompleted;
    int anyCompleted;
    int canSequence;

    char buffer[100];
    char* printStringA;
    char* printStringB;

    // Instantiate the local available vector;
    availableLocal = malloc(resourceCount * sizeof(int));
    // Copy over the values
    for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
    {
        availableLocal[resourceIndex] = available[resourceIndex];
    }
    // Instantiate the completed tracking array
    completed = malloc(processCount * sizeof(int));
    for (processIndex = 0; processIndex < processCount; processIndex++)
    {
        completed[processIndex] = 0;
    }

    // Iterate over each process and check if they can be executed
    do
    {
        // Reset the all-completed flag
        allCompleted = 1;
        // Reset the any-completed flag
        anyCompleted = 0;

        // Check if any process is not done yet
        for (processIndex = 0; processIndex < processCount; processIndex++)
        {
            if (!completed[processIndex]) allCompleted = 0;
        }

        // Try to sequence each process
        for (processIndex = 0; processIndex < processCount; processIndex++)
        {
            // Reset the flag indicating we can sequence this process
            canSequence = 1;

            // Continue if we've sequenced this already
            if (completed[processIndex]) continue;

            // Check if we can sequence this process
            for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
            {
                // Check if we don't have enough resources to
                if (needed[processIndex][resourceIndex] > availableLocal[resourceIndex])
                {
                    // Indicate we can't sequence this process
                    canSequence = 0;
                    // Break
                    break;
                }
            }

            // Get the string for the needed and available resources
            printStringA = malloc(sizeof(char) * 1024);
            printStringA[0] = '\0';
            printStringB = malloc(sizeof(char) * 1024);
            printStringB[0] = '\0';
            for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
            {
                // Write the needed and available resources to their respective strings through the buffer.
                sprintf(buffer, " %d", needed[processIndex][resourceIndex]);
                strcat(printStringA, buffer);
                sprintf(buffer, " %d", availableLocal[resourceIndex]);
                strcat(printStringB, buffer);
            }

            // Branch if we can sequence
            if (canSequence)
            {
                // Flag that we've completed a process this cycle
                anyCompleted = 1;
                // Flag that this process is complete
                completed[processIndex] = 1;
                // Free the resources that were previously allocated
                for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
                {
                    availableLocal[resourceIndex] += allocated[processIndex][resourceIndex];
                }

                printf("\nChecking: <%s > <= <%s > :p%d safely sequenced", printStringA, printStringB, processIndex);
            }
            else
            {
                // Print that we've sequenced unsuccessfully
                printf("\nChecking: <%s > <= <%s > :p%d could not be sequenced", printStringA, printStringB, processIndex);
            }
        }

    } while (anyCompleted);

    if (!allCompleted)
    {
        // Print could not find safe sequence?
        printf("\nDeadlock reached!");
    }
}

void Quit()
{
    // Free all used memory
    free(resources);
    free(available);
    free(maxClaim);
    free(allocated);
    free(needed);
    printf("\nQuitting program...");
}

/***************************************************************/
int main() {
    int userInput = 0;

    while (userInput != 3)
    {
        // Take user input for menu option
        printf("\n\n\nBanker's Algorithm\n"
               "------------------\n"
               "1) Enter parameters\n"
               "2) Determine safe sequence\n"
               "3) Quit program\n"
               "\n"
               "Enter selection: ");
        scanf("%d", &userInput);

        // Handle the user input
        switch (userInput)
        {
            case 1: // The user is trying to set parameters
                TakeParameters();
                break;
            case 2: // The user is trying to find the safe sequence
                FindSafeSequence();
                break;
            case 3: // The user is trying to quit
                Quit();
                break;
            default: // The user is trying to do something unsupported
                printf("\nError: Input not recognized, must be from options above.");
        }
    }

    // Exit the program successfully!
    return 1;
}
