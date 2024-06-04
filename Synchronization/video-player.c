/*
 * This C program demonstrates a multi-threaded video player using pthreads for concurrent execution of decoding
 * , transforming, and displaying video frames. It is structured around the use of circular buffers for managing frames
 * at different stages of the playback process: decoding, transforming (e.g., filtering or resizing), and displaying.
 *
 * The circular_buffer structure is key to this program, designed to hold video frames with operations to push and pop
 * frames in a thread-safe manner, ensuring synchronization between producer (decoder, transformer) and consumer
 * (display) threads. Each buffer is protected by a mutex to prevent concurrent access issues.
 *
 * The decode_job function represents a thread's work to decode video frames and push them into the decoding buffer.
 * The transform_job function takes decoded frames, applies transformations, and pushes them into the transformation buffer.
 * The display_job function then takes transformed frames for display, ensuring a frame rate consistent with real-time
 * playback by controlling the pace of frame popping based on a calculated sleep interval.
 *
 */


#include <pthread.h>
#include <time.h>
#include "video-player.h"

#define mili_to_micro 1000
#define ms_in_s 0.001
#define s_to_us 1000000

typedef struct timespec timespec_t;
typedef struct circular_buffer
{
    video_frame* array[BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t n_elements;
    pthread_mutex_t mxArray;
} circular_buffer;

typedef struct both_buffers_t
{
    circular_buffer* decode_buffer;
    circular_buffer* transform_buffer;
} both_buffers_t;

circular_buffer* circular_buffer_create()
{
    circular_buffer* buffer = malloc(sizeof(circular_buffer));
    buffer->tail = 0;
    buffer->head = 0;
    buffer->n_elements = 0;
    if (pthread_mutex_init(&buffer->mxArray, NULL))
    {
        ERR("init mutex");
    }
    return buffer;
}
void wait_till_not_empty(circular_buffer* buffer)
{
    while (buffer->n_elements == 0)
    {
        for (int tt = usleep(mili_to_micro * 5); tt > 0; tt = usleep(tt))
        {
        }
    }
}
void wait_till_not_full(circular_buffer* buffer)
{
    while (buffer->n_elements == BUFFER_SIZE)
    {
        for (int tt = usleep(mili_to_micro * 5); tt > 0; tt = usleep(tt))
        {
        }
    }
}
void circular_buffer_push(circular_buffer* buffer, video_frame* frame)
{
    (buffer->array)[buffer->head] = frame;
    buffer->n_elements = buffer->n_elements + 1;
    // make it circular
    if (buffer->head + 1 < BUFFER_SIZE)
    {
        buffer->head = buffer->head + 1;
    }
    else
    {
        buffer->head = 0;
    }
}
video_frame* circular_buffer_pop(circular_buffer* buffer)
{
    video_frame* frame = (buffer->array)[buffer->tail];
    buffer->n_elements = buffer->n_elements - 1;
    // make it circular
    if (buffer->tail + 1 < BUFFER_SIZE)
    {
        buffer->tail = buffer->tail + 1;
    }
    else
    {
        buffer->tail = 0;
    }
    return frame;
}
void circular_buffer_destroy(circular_buffer* buffer)
{
    if (pthread_mutex_destroy(&buffer->mxArray))
    {
        ERR("Destroy mutex");
    }
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if ((buffer->array)[i])
        {
            free((buffer->array)[i]);
        }
    }
    free(buffer);
}
void* decode_job(void* args)
{
    circular_buffer* buffer = args;
    while (1)
    {
        video_frame* frame = decode_frame();
        wait_till_not_full(buffer);
        if (pthread_mutex_lock(&buffer->mxArray))
        {
            ERR("mutex lock");
        }
        circular_buffer_push(buffer, frame);
        if (pthread_mutex_unlock(&buffer->mxArray))
        {
            ERR("mutex unlock");
        }
    }
}
void* transform_job(void* args)
{
    both_buffers_t* bothBuffers = args;
    circular_buffer* decode_buffer = bothBuffers->decode_buffer;
    circular_buffer* transform_buffer = bothBuffers->transform_buffer;
    while (1)
    {
        video_frame* frame;
        // Get decoded frame
        wait_till_not_empty(decode_buffer);
        if (pthread_mutex_lock(&decode_buffer->mxArray))
        {
            ERR("mutex lock");
        }
        frame = circular_buffer_pop(decode_buffer);
        if (pthread_mutex_unlock(&decode_buffer->mxArray))
        {
            ERR("mutex unlock");
        }
        // Transform it
        transform_frame(frame);
        // Push Transformed frame
        wait_till_not_full(transform_buffer);
        if (pthread_mutex_lock(&transform_buffer->mxArray))
        {
            ERR("mutex lock");
        }
        circular_buffer_push(transform_buffer, frame);
        if (pthread_mutex_unlock(&transform_buffer->mxArray))
        {
            ERR("mutex unlock");
        }
    }
}
void* display_job(void* args)
{
    timespec_t start, current;
    while (1)
    {
        circular_buffer* buffer = args;
        video_frame* frame;

        if (clock_gettime(CLOCK_REALTIME, &current))
            ERR("Can't get time");
        if (ELAPSED(start, current) < 33 * ms_in_s)
        {
            for (int tt = usleep(ELAPSED(start, current) * s_to_us); tt > 0; tt = usleep(tt))
            {
            }
        }
        wait_till_not_empty(buffer);
        if (pthread_mutex_lock(&buffer->mxArray))
        {
            ERR("mutex lock");
        }
        frame = circular_buffer_pop(buffer);
        if (pthread_mutex_unlock(&buffer->mxArray))
        {
            ERR("mutex unlock");
        }
        if (clock_gettime(CLOCK_REALTIME, &start))
            ERR("Can't get time");
        display_frame(frame);
    }
}
int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    // Init the buffer
    circular_buffer* decode_buffer = circular_buffer_create();
    circular_buffer* transform_buffer = circular_buffer_create();
    both_buffers_t bothBuffers;
    bothBuffers.decode_buffer = decode_buffer;
    bothBuffers.transform_buffer = transform_buffer;

    pthread_t p1, p2, p3;
    if (pthread_create(&p1, NULL, decode_job, decode_buffer))
        ERR("Couldn't create thread");
    if (pthread_create(&p2, NULL, transform_job, &bothBuffers))
        ERR("Couldn't create thread");
    if (pthread_create(&p3, NULL, display_job, transform_buffer))
        ERR("Couldn't create thread");

    if (pthread_join(p1, NULL))
    {
        ERR("Joining threads");
    }
    if (pthread_join(p2, NULL))
    {
        ERR("Joining threads");
    }
    if (pthread_join(p3, NULL))
    {
        ERR("Joining threads");
    }

    circular_buffer_destroy(decode_buffer);
    circular_buffer_destroy(transform_buffer);

    exit(EXIT_SUCCESS);
}
