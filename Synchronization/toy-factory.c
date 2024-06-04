/*
 * This source file, part of a toy factory simulation, focuses on the production and processing of gifts 
 * from ordering to delivery. It defines three primary roles through threads: an assembler, a wrapper, 
 * and Santa. 
 *
 * The assembler thread is responsible for creating and assembling gifts, placing them onto an assembly 
 * line. The wrapper thread takes these gifts off the assembly line, wraps them, and prepares them for 
 * delivery. The assembly line itself is implemented as a circular buffer, managed through semaphores 
 * to handle the synchronization between assembler and wrapper threads, ensuring there are available 
 * slots for new gifts and gifts ready to be wrapped. 
 *
 * Santa's thread, although included, is not fully implemented and serves as a placeholder for potential 
 * extension, such as managing the sleigh loading and delivering the wrapped gifts. The program utilizes 
 * POSIX semaphores for synchronization, dynamic memory allocation for creating gifts, and pthreads for 
 * concurrent execution of assembly and wrapping processes.
 */

#include "toy-factory.h"

#define ASSEMBLY_LINE_CAPACITY (8)
#define SLEIGHT_CAPACITY (100)

typedef struct
{
    Gift *array[ASSEMBLY_LINE_CAPACITY];
    int first;
    int last;
} assembly_line_t;

assembly_line_t *assembly_line_create()
{
    assembly_line_t *assembly_line = malloc(sizeof(assembly_line_t));
    assembly_line->first = 0;
    assembly_line->last = 0;
    return assembly_line;
}

typedef struct
{
    assembly_line_t *assembly_line;
    sem_t *n_assembly_free_spaces;
    sem_t *n_assembly_full_spaces;
} assembler_thread_arg;

void *elf_assembler_thread(void *arg)
{
    assembler_thread_arg *ass_arg = (assembler_thread_arg *)arg;
    while (1)
    {
        if (TEMP_FAILURE_RETRY(sem_wait(ass_arg->n_assembly_free_spaces)) == -1)
        {
            switch (errno)
            {
                case EAGAIN:
                case EINTR:
                    continue;
            }
            ERR("sem_trywait");
        }

        Gift *gift = GetNextGiftOrder();
        AssembleGift(gift);
        ass_arg->assembly_line->array[ass_arg->assembly_line->last] = gift;
        ass_arg->assembly_line->last = (ass_arg->assembly_line->last + 1) % ASSEMBLY_LINE_CAPACITY;

        if (sem_post(ass_arg->n_assembly_full_spaces) == -1)
            ERR("sem_post");
    }
    free(ass_arg);
    pthread_exit(NULL);
}

typedef struct
{
    assembly_line_t *assembly_line;
    sem_t *n_assembly_free_spaces;
    sem_t *n_assembly_full_spaces;
} wrapper_thread_arg;

void *elf_wrapper_thread(void *arg)
{
    wrapper_thread_arg *wrp_arg = (wrapper_thread_arg *)arg;
    while (1)
    {
        if (TEMP_FAILURE_RETRY(sem_wait(wrp_arg->n_assembly_full_spaces)) == -1)
        {
            switch (errno)
            {
                case EAGAIN:
                case EINTR:
                    continue;
            }
            ERR("sem_trywait");
        }

        WrapGift(wrp_arg->assembly_line->array[wrp_arg->assembly_line->first]);
        MillGift(wrp_arg->assembly_line->array[wrp_arg->assembly_line->first]);
        (wrp_arg->assembly_line->first) = ((wrp_arg->assembly_line->first) + 1) % ASSEMBLY_LINE_CAPACITY;

        if (sem_post(wrp_arg->n_assembly_free_spaces) == -1)
            ERR("sem_post");
    }
    free(wrp_arg);
    pthread_exit(NULL);
}

typedef struct
{
} santa_thread_arg;

void create_threads(assembly_line_t *assembly_line, sem_t *n_assembly_free_spaces, sem_t *n_assembly_full_spaces,
                    pthread_t *assembler_p, pthread_t *wrapper_p, pthread_t *santa_p);

void *santa_sleight_thread(void *arg)
{
    santa_thread_arg *stn_arg = (santa_thread_arg *)arg;
    UNUSED(stn_arg);
    pthread_exit(NULL);
}

int main()
{
    // the assembly line
    assembly_line_t *assembly_line = assembly_line_create();
    // create and initialize semaphores
    sem_t n_assembly_free_spaces;
    sem_t n_assembly_full_spaces;
    if (sem_init(&n_assembly_free_spaces, 0, ASSEMBLY_LINE_CAPACITY) != 0)
        ERR("sem_init");
    if (sem_init(&n_assembly_full_spaces, 0, 0) != 0)
        ERR("sem_init");

    pthread_t assembler_p;
    pthread_t wrapper_p;
    pthread_t santa_p;

    create_threads(assembly_line, &n_assembly_free_spaces, &n_assembly_full_spaces, &assembler_p, &wrapper_p, &santa_p);

    if (pthread_join(assembler_p, NULL))
    {
        ERR("Joining threads");
    }
    if (pthread_join(wrapper_p, NULL))
    {
        ERR("Joining threads");
    }
    if (pthread_join(santa_p, NULL))
    {
        ERR("Joining threads");
    }

    // Gift *assembly_line[ASSEMBLY_LINE_CAPACITY] = {0};
    // Gift *sleight[SLEIGHT_CAPACITY] = {0};
    // for (int i = 0; i < ASSEMBLY_LINE_CAPACITY; ++i)
    //{
    //     assembly_line[i] = GetNextGiftOrder();
    //     AssembleGift(assembly_line[i]);
    //     WrapGift(assembly_line[i]);
    //     sleight[i] = assembly_line[i];
    //     assembly_line[i] = NULL;
    // }
    // DeliverGifts(sleight, ASSEMBLY_LINE_CAPACITY);

    free(assembly_line);
    return 0;
}

void create_threads(assembly_line_t *assembly_line, sem_t *n_assembly_free_spaces, sem_t *n_assembly_full_spaces,
                    pthread_t *assembler_p, pthread_t *wrapper_p, pthread_t *santa_p)
{
    // assembler_elf
    assembler_thread_arg *assembler_args = malloc(sizeof(assembler_thread_arg));
    assembler_args->n_assembly_full_spaces = n_assembly_full_spaces;
    assembler_args->n_assembly_free_spaces = n_assembly_free_spaces;
    assembler_args->assembly_line = assembly_line;
    wrapper_thread_arg *wrapper_args = malloc(sizeof(wrapper_thread_arg));
    wrapper_args->n_assembly_full_spaces = n_assembly_full_spaces;
    wrapper_args->n_assembly_free_spaces = n_assembly_free_spaces;
    wrapper_args->assembly_line = assembly_line;

    if (pthread_create(assembler_p, NULL, elf_assembler_thread, assembler_args))
        ERR("Couldn't create thread");
    if (pthread_create(wrapper_p, NULL, elf_wrapper_thread, wrapper_args))
        ERR("Couldn't create thread");
    if (pthread_create(santa_p, NULL, santa_sleight_thread, NULL))
        ERR("Couldn't create thread");
}
