#include <stdio.h>
#include <malloc.h>

// Struct Types
struct Block // An allocated block of memory
{
    int id;
    int addressStart;
    int addressEnd;
};

struct LinkedList// A list of all currently-allocated blocks of memory
{
    struct LinkedList* last;
    struct Block block;
    struct LinkedList* next;
};

// Global Variables
int pm_size; // Size of physical memory
int pm_allocated; // Amount of physical memory in use
int holeFillingAlgorithm; // Hole fitting algorithm chosen (0 = first, 1 = best)
struct LinkedList* allocations; // All allocations made thus far
struct LinkedList* allocationsLast; // The back/last allocation in the list
struct LinkedList* holes; // All the holes available currently

/********************************************************************/
void DeallocateLinkedList(struct LinkedList *node)
{
    // Return on the last/null element
    if (node == NULL) return;
    else
    {
        // Deallocate the next node
        DeallocateLinkedList(node->next);
        // Free this node
        free(node);
    }
    return;
}
/********************************************************************/
void TakeParameters() {
    // Declare variables
    int isInputBad;

    // Take the size of the physical memory
    do
    {
        isInputBad = 0;

        printf("Enter size of physical memory: ");
        scanf("%d", &pm_size);

        // Error Checking
        if (pm_size <= 0)
        {
            // Print the error
            printf("ERROR: Primary memory size must be greater than 0!\n");
            // Restart this question
            isInputBad = 1;
        }
        // Clear the input
        fflush(stdin);
    } while (isInputBad);

    // Take the hole-filling algorithm choice
    do
    {
        isInputBad = 0;

        printf("Enter hole-fitting algorithm (0=first fit, 1=best_fit): ");
        scanf("%d", &holeFillingAlgorithm);

        // Error Checking
        if (holeFillingAlgorithm != 0 && holeFillingAlgorithm != 1)
        {
            // Print the error
            printf("ERROR: Hole fitting algorithm choice must be either 0 or 1!\n");
            // Restart this question
            isInputBad = 1;
        }
        // Clear the input
        fflush(stdin);
    } while (isInputBad);

    // Default the allocated physical memory to 0
    pm_allocated = 0;
    // Initialize the LinkedList for holes
    holes = malloc(sizeof(struct LinkedList));
    // Default the size to be the entirety of the physical memory
    holes->block.addressStart = 0;
    holes->block.addressEnd = pm_size;
    holes->next = NULL;
    holes->last = NULL;
}
/********************************************************************/
void PrintAllocationTable() {
    struct LinkedList* currentAllocation;

    // Print the table header
    printf("\nID\tStart\tEnd\n-------------------\n");

    // Initialize the first allocation iterated to be the head of the LinkedList
    currentAllocation = allocations;

    // Iterate over and print each allocated block
    while (currentAllocation != NULL && currentAllocation->block.id != -1)
    {
        // Print the current allocation
        printf("%d\t%d\t%d\n",
               currentAllocation->block.id,
               currentAllocation->block.addressStart,
               currentAllocation->block.addressEnd);

        // Move the current allocation pointer to the next in the list
        currentAllocation = currentAllocation->next;
    }
    // Print a linebreak
    printf("");
}
/********************************************************************/
void AllocateBlockHelper(int id, int size, struct LinkedList* filledHole)
{
    // Declare variables
    struct LinkedList* currentBlock;

    // Store a pointer to our filled out allocation block
    struct LinkedList* newBlock = malloc(sizeof(struct LinkedList));
    // Add our block to the allocation list
    newBlock->block.id = id;
    newBlock->block.addressStart = filledHole->block.addressStart;
    newBlock->block.addressEnd = filledHole->block.addressStart + size;
    newBlock->next = NULL;
    newBlock->last = NULL;

    // Try to store the allocation
    if (allocations == NULL)
    {
        // Update both pointers
        allocations = newBlock;
        allocationsLast = newBlock;
    }
    else
    {
        // Find where the new block will be inserted
        currentBlock = allocations;
        while (currentBlock != NULL)
        {
            // Check if we can put the new hole at the start of the list
            if (newBlock->block.addressEnd <= currentBlock->block.addressStart)
            {
                // Update the hole after
                newBlock->next = currentBlock;
                currentBlock->last = newBlock;
                // Update the collection pointer
                allocations = newBlock;
                break;
            }
            // Check our new block is after the current block
            else if (newBlock->block.addressStart >= currentBlock->block.addressEnd)
            {
                // Check if we're at the end of the blocks list
                if (currentBlock->next == NULL)
                {
                    // Just append the block
                    newBlock->last = currentBlock;
                    currentBlock->next = newBlock;
                    allocationsLast = newBlock;
                    break;
                }
                // Check if we're in between this and the next block
                else if (newBlock->block.addressEnd <= currentBlock->next->block.addressStart)
                {
                    // Insert the new hole in between these two
                    // Update the hole after the current, before the after
                    newBlock->next = currentBlock->next; // Current to Next
                    newBlock->next->last = newBlock; // Next to Current
                    // Update the hole before & vice versa
                    newBlock->last = currentBlock; // Current to Last
                    currentBlock->next = newBlock; // Last to Current
                    break; // Break out of the while-loop as we found our insertion point
                }
            }

            // Move the iterator
            currentBlock = currentBlock->next;
        }
    }
    // Update the amount of used memory
    pm_allocated += size;

    // Update/Remove the Hole
    // Check if the hole has been filled fully
     if (allocationsLast->block.addressEnd == filledHole->block.addressEnd)
    {
        // Remove the hole entirely
        // Check if we're at the front of the list
        if (filledHole == holes)
        {
            // Move the hole pointer forward, potentially null-ing it!
            holes = holes->next;
        }
    }
    // Otherwise, fill part of the hole
    else
    {
        // Move the start of the hole forward to just after the block
        filledHole->block.addressStart = newBlock->block.addressEnd;
    }
}
void TakeAllocateBlock() {
    struct LinkedList* currentBlock;
    struct LinkedList* currentHole;
    int newBlockId;
    int newBlockSize;
    int isInputBad;

    // Take the new block's id
    do
    {
        isInputBad = 0;

        printf("Enter block id: ");
        scanf("%d", &newBlockId);

        // Error Checking
        if (newBlockId < 0)
        {
            // Print the error
            printf("ERROR: ID must be positive!\n");
            // Restart this question
            isInputBad = 1;
            continue;
        }

        // Default the current LinkedList iterator to the head of the allocations
        currentBlock = allocations;
        // Check each allocation for a duplicate ID
        while (currentBlock != NULL)
        {
            if (newBlockId == currentBlock->block.id)
            {
                // Print the error
                printf("ERROR: ID must not be duplicate!\n");
                // Restart this question
                isInputBad = 1;
                // Break out of the while-loop
                break;
            }

            // Move the iterator forward
            currentBlock = currentBlock->next;
        }

        // Clear the input
        fflush(stdin);
    } while (isInputBad);

    // Take the new block's size
    do
    {
        isInputBad = 0;

        printf("Enter block size: ");
        scanf("%d", &newBlockSize);

        // Error Checking
        if (newBlockSize <= 0)
        {
            // Print the error
            printf("ERROR: The size of the block must be greater than 0!\n");
            // Restart this question
            isInputBad = 1;
            continue;
        }
        if (newBlockSize + pm_allocated > pm_size)
        {
            // Print the error
            printf("ERROR: Not enough memory in system to support a block of size %d!\n", newBlockSize);
            // Restart this question
            isInputBad = 1;
            continue;
        }

        // Flag the input as bad unless there exists one workable allocation
        isInputBad = 1;
        // Default the current LinkedList iterator to the head of the holes
        // Iterate over the list
        currentHole = holes;
        while (currentHole != NULL)
        {
            // Check if block can fit
            if (currentHole->block.addressStart + newBlockSize <= currentHole->block.addressEnd)
            {
                // There exists at least one hole that can fit this new block
                isInputBad = 0;
                break;
            }

            // Continue iterating
            currentHole = currentHole->next;
        }
        // Catch if there's no holes large enough to fit the block
        if (isInputBad)
        {
            printf("ERROR: No holes large enough to fit a block of size %d!\n", newBlockSize);
        }

        // Clear the input
        fflush(stdin);
    } while (isInputBad);

    // Store a pointer to the to-be-filled hole
    struct LinkedList* filledHole = NULL;

    // Branch to our hole-fitting algorithm
    if (holeFillingAlgorithm == 0)
    { // First-fit
        // Set our iteration pointer to the head of the holes LinkedList
        currentHole = holes;
        // Iterate over each hole until we find the first one that fits
        while (currentHole != NULL)
        {
            // Check if the block can fit in the current hole
            if (currentHole->block.addressStart + newBlockSize <= currentHole->block.addressEnd)
            {
                // Allocate the memory at the given hole
                filledHole = currentHole;
                break;
            }

            // Move the iterator forward
            currentHole = currentHole->next;
        }
    }
    else
    { // Best fit
        // Set our iteration pointer to the head of the holes LinkedList
        currentHole = holes;
        // Iterate over each hole until we find the smallest one that fits
        while (currentHole != NULL)
        {
            // Check if the block can fit in the current hole
            // AND that it's smaller than the current hole (or we don't have any 'best hole' yet)
            if (currentHole->block.addressStart + newBlockSize <= currentHole->block.addressEnd
            && (filledHole == NULL
                || currentHole->block.addressEnd - currentHole->block.addressStart < filledHole->block.addressEnd - filledHole->block.addressStart))
            {
                // Allocate the memory at the given hole
                filledHole = currentHole;
            }

            // Move the iterator forward
            currentHole = currentHole->next;
        }
    }

    // Fill the chosen hole
    AllocateBlockHelper(newBlockId, newBlockSize, filledHole);
    // Print the allocation table
    PrintAllocationTable();
    return;
}
/********************************************************************/
void TakeDeallocateBlock() {
    // Declare variables
    struct LinkedList* currentHole;
    struct LinkedList* removedBlock;
    struct LinkedList* newHole;
    int removedBlockId;
    int isInputBad;

    // Take the new block's id
    do
    {
        isInputBad = 0;

        printf("Enter block id: ");
        scanf("%d", &removedBlockId);

        // Error Checking
        if (removedBlockId < 0)
        {
            // Print the error
            printf("ERROR: ID must be positive!\n");
            // Restart this question
            isInputBad = 1;
        }

        // Default the current LinkedList iterator to the head of the allocations
        removedBlock = allocations;
        // Check each allocation for a duplicate ID
        while (removedBlock != NULL)
        {
            // Check if the ID does actually exist
            if (removedBlockId == removedBlock->block.id)
            {
                // Break out of the while-loop
                break;
            }
            // Check if we hit the end of the list
            else if (removedBlock->next == NULL)
            {
                // Flag the input as bad
                isInputBad = 1;
                // Print the error
                printf("ERROR: ID not valid!\n");
                // Restart the question
                break;
            }
            // Move the iterator forward
            removedBlock = removedBlock->next;
        }
        // Clear the input
        fflush(stdin);
    } while (isInputBad);


    // Create the new hole
    newHole = malloc(sizeof(struct LinkedList));
    newHole->block.addressStart = removedBlock->block.addressStart;
    newHole->block.addressEnd = removedBlock->block.addressEnd;
    newHole->next = NULL;
    newHole->last = NULL;
    // Check if the first hole is null (so we can just alloc to the front)
    if (holes == NULL)
    {
        // Set the new hole to the front of the list
        holes = newHole;
    }
    // Find the hole where this fits
    else
    {
        // Find where freed memory will be placed
        currentHole = holes;
        while (currentHole != NULL)
        {
            // Check if we can put the new hole at the start of the list
            if (newHole->block.addressEnd <= currentHole->block.addressStart)
            {
                // Update the hole after
                newHole->next = currentHole;
                currentHole->last = newHole;
                // Update the collection pointer
                holes = newHole;
                break;
            }
            // Check our new hole is after the current hole
            else if (newHole->block.addressStart >= currentHole->block.addressEnd)
            {
                // Check if we're at the end of the holes list
                if (currentHole->next == NULL)
                {
                    // Just append the hole
                    newHole->last = currentHole;
                    currentHole->next = newHole;
                    break;
                }
                // Check if we're in between this and the next hole
                else if (newHole->block.addressEnd <= currentHole->next->block.addressStart)
                {
                    // Insert the new hole in between these two
                    // Update the hole after the current, before the after
                    newHole->next = currentHole->next; // Current to Next
                    newHole->next->last = newHole; // Next to Current
                    // Update the hole before & vice versa
                    newHole->last = currentHole; // Current to Last
                    currentHole->next = newHole; // Last to Current
                    break; // Break out of the while-loop as we found our insertion point
                }
            }

            // Move the iterator
            currentHole = currentHole->next;
        }
    }

    // Adjust the pointers
    if (removedBlock == allocations)
    {
        if (removedBlock->last != NULL) allocations = removedBlock->last;
        else if (removedBlock->next != NULL) allocations = removedBlock->next;
    }
    // Adjust the pointers between the left and right
    if (removedBlock->next != NULL) removedBlock->next->last = removedBlock->last;
    if (removedBlock->last != NULL) removedBlock->last->next = removedBlock->next;

    // Update the available memory
    pm_allocated -= removedBlock->block.addressEnd - removedBlock->block.addressStart;
    // Free the memory
    free(removedBlock);

    // Reconnect contiguous blocks
    currentHole = holes;
    while (currentHole != NULL)
    {
        // Check if there's a next hole AND it's contiguous
        while (currentHole->next != NULL
                && currentHole->block.addressEnd == currentHole->next->block.addressStart)
        {
            // Merge these two holes
            struct LinkedList* removedHole = currentHole->next;

            // Resize the hole to the correct
            currentHole->block.addressEnd = currentHole->next->block.addressEnd;

            // Update the pointers
            currentHole->next = currentHole->next->next;
            currentHole->next->last = currentHole;

            // Destroy the merged hole
            free(removedHole);
        }

        // Move the iteration variable
        currentHole = currentHole->next;
    }

    // Print the allocation table
    PrintAllocationTable();

    return;
}
/********************************************************************/
void DefragmentMemory() {
    // Declare variables
    struct LinkedList* currentBlock;
    int currentBlockSize;

    // Move all allocations to be next to one-another
    // Loop over each block
    currentBlock = allocations;
    while (currentBlock != NULL)
    {
        // Store the length of this block
        currentBlockSize = currentBlock->block.addressEnd - currentBlock->block.addressStart;

        // Check if this is the first block in the list
        if (currentBlock == allocations)
        {
            // Move the block to the front of memory
            currentBlock->block.addressStart = 0;
            // Update the end of the memory
            currentBlock->block.addressEnd = currentBlockSize;
        }
        else
        {
            // Move this block to the end of the previous block
            currentBlock->block.addressStart = currentBlock->last->block.addressEnd;
            // Update the end of the memory
            currentBlock->block.addressEnd = currentBlock->block.addressStart + currentBlockSize;
        }

        // Enumerate forward on the list
        currentBlock = currentBlock->next;
    }

    // Remove all holes
    DeallocateLinkedList(holes);
    // Check if there's room for a hole at the end of memory
    if (allocationsLast->block.addressEnd < pm_size)
    {
        // Create a new hole to place in the gap
        holes = malloc(sizeof(struct LinkedList));
        // Configure the hole
        holes->block.addressStart = allocationsLast->block.addressEnd;
        holes->block.addressEnd = pm_size;
        // Set the pointers to null
        holes->next = NULL;
        holes->last = NULL;
    }

    // Print the allocation table
    PrintAllocationTable();

    return;
}
/********************************************************************/
void Quit()
{
    // Deallocate linked list (if null)
    DeallocateLinkedList(holes);
    DeallocateLinkedList(allocations);
}
/***************************************************************/
int main() {
    int userInput = 0;

    while (userInput != 5)
    {
        // Take user input for menu option
        printf("\nMemory allocation\n"
               "-----------------\n"
               "1) Enter parameters\n"
               "2) Allocate memory for block\n"
               "3) Deallocate memory for block\n"
               "4) Defragment memory\n"
               "5) Quit program\n"
               "\n"
               "Enter selection: ");
        scanf("%d", &userInput);

        // Handle the user input
        switch (userInput)
        {
            case 1: // The user is trying to set parameters
                TakeParameters();
                break;
            case 2: // The user is trying to allocate a new block of memory
                TakeAllocateBlock();
                break;
            case 3: // The user is trying to deallocate an existing block of memory
                TakeDeallocateBlock();
                break;
            case 4: // The user is trying to defragment memory
                DefragmentMemory();
                break;
            case 5: // The user is trying to quit
                Quit();
                break;
            default: // The user is trying to do something unsupported
                printf("\nError: Input not recognized, must be from options above.");
        }
    }

    // Exit the program successfully!
    return 1;
}