#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>

// Global variables
const int SIZE = 9999;  // Size of the spiral
const int iXmax = SIZE; 
const int iYmax = SIZE;

// Configuration for nested parallel regions
const int THREADS_X = 2;  // Number of threads in X direction
const int THREADS_Y = 2;  // Number of threads in Y direction
const int TOTAL_THREADS = THREADS_X * THREADS_Y;

// Color definitions
const int MaxColorComponentValue = 255; 

// Image buffers for comparison
unsigned char* imageNested;
unsigned char* imageHorizontal;
unsigned char* imageScheduler;

struct ScheduleConfig
{
    omp_sched_t type;
    int chunkSize;
    const char* label;
    const char* filename;
};

const ScheduleConfig SCHEDULE_CONFIGS[] =
{
    { omp_sched_static, 0,   "static (default chunk)", "ulam_spiral_schedule_static_default.ppm" },
    { omp_sched_static, 100, "static (chunk = 100)",  "ulam_spiral_schedule_static_100.ppm" },
    { omp_sched_dynamic, 1,  "dynamic (chunk = 1)",   "ulam_spiral_schedule_dynamic_1.ppm" },
    { omp_sched_dynamic, 100,"dynamic (chunk = 100)",  "ulam_spiral_schedule_dynamic_100.ppm" },
    { omp_sched_guided, 0,   "guided",                 "ulam_spiral_schedule_guided.ppm" },
    { omp_sched_auto, 0,     "auto",                   "ulam_spiral_schedule_auto.ppm" }
};

const int NUM_SCHEDULES = sizeof(SCHEDULE_CONFIGS) / sizeof(SCHEDULE_CONFIGS[0]);

void computeThreadColor(int threadId, int totalThreads, unsigned char* threadColor);
void writeImagePPM(const char* filename, const char* comment, const unsigned char* image);
double runSchedulerExperiment(const ScheduleConfig& config, unsigned char* image);

// Function to check if a number is prime and return number of iterations
int isPrime(long long n)
{
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    
    int iterations = 0;
    for (long long i = 5; i * i <= n; i += 6)
    {
        iterations++;
        if (n % i == 0 || n % (i + 2) == 0)
            return 0;
    }
    return iterations;
}

// Function to calculate spiral coordinates and return the number at position (x, y)
long long getSpiralNumber(int x, int y)
{
    int cx = x - SIZE / 2;
    int cy = y - SIZE / 2;
    
    int ring = (int)fmax(abs(cx), abs(cy));
    
    if (ring == 0) return 1;
    
    long long maxNumInPrevRing = (2 * ring - 1) * (2 * ring - 1);
    long long numInRing = maxNumInPrevRing;
    
    if (cx == ring)
    {
        numInRing += ring + cy;
    }
    else if (cy == ring)
    {
        numInRing += 3 * ring - cx;
    }
    else if (cx == -ring)
    {
        numInRing += 5 * ring - cy;
    }
    else
    {
        numInRing += 7 * ring + cx;
    }
    
    return numInRing;
}

// Function to compute one block with nested parallelism
void computeQuadrantNested(int blockX, int blockY, unsigned char* image)
{
    int midX = SIZE / 2;
    int midY = SIZE / 2;
    
    int startX = blockX * midX;
    int endX = (blockX + 1) * midX;
    int startY = blockY * midY;
    int endY = (blockY + 1) * midY;
    
    int threadId = blockY * THREADS_X + blockX;
    
    unsigned char threadColor[3];
    float hue = (float)threadId / 4.0f;
    float saturation = 1.0f;
    float value = 1.0f;
    
    int h_i = (int)(hue * 6);
    float f = hue * 6 - h_i;
    float p = value * (1 - saturation);
    float q = value * (1 - f * saturation);
    float t = value * (1 - (1 - f) * saturation);
    
    float r, g, b;
    switch(h_i)
    {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
    }
    
    threadColor[0] = (unsigned char)(r * 255);
    threadColor[1] = (unsigned char)(g * 255);
    threadColor[2] = (unsigned char)(b * 255);
    
    for (int y = startY; y < endY; y++)
    {
        for (int x = startX; x < endX; x++)
        {
            long long num = getSpiralNumber(x, y);
            int pixelIndex = (y * SIZE + x) * 3;
            
            int iterations = isPrime(num);
            if (iterations > 0)
            {
                // Intensywność koloru zależna od liczby iteracji
                float intensity = fminf(1.0f, iterations / 50.0f);
                image[pixelIndex] = (unsigned char)(threadColor[0] * intensity);
                image[pixelIndex + 1] = (unsigned char)(threadColor[1] * intensity);
                image[pixelIndex + 2] = (unsigned char)(threadColor[2] * intensity);
            }
            else
            {
                // Jasno szare tło dla liczb niepierw
                image[pixelIndex] = 200;
                image[pixelIndex + 1] = 200;
                image[pixelIndex + 2] = 200;
            }
        }
    }
}

void computeThreadColor(int threadId, int totalThreads, unsigned char* threadColor)
{
    float hue = (totalThreads > 0) ? (float)threadId / (float)totalThreads : 0.0f;
    float saturation = 1.0f;
    float value = 1.0f;

    int h_i = (int)(hue * 6);
    float f = hue * 6 - h_i;
    float p = value * (1 - saturation);
    float q = value * (1 - f * saturation);
    float t = value * (1 - (1 - f) * saturation);

    float r, g, b;
    switch(h_i)
    {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
    }

    threadColor[0] = (unsigned char)(r * 255);
    threadColor[1] = (unsigned char)(g * 255);
    threadColor[2] = (unsigned char)(b * 255);
}

void writeImagePPM(const char* filename, const char* comment, const unsigned char* image)
{
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        printf("Failed to write %s\n", filename);
        return;
    }

    fprintf(fp, "P6\n# %s\n%d\n%d\n%d\n", comment, SIZE, SIZE, MaxColorComponentValue);
    fwrite(image, 1, SIZE * SIZE * 3, fp);
    fclose(fp);
    printf("Image saved to %s (%s)\n", filename, comment);
}

// Function to compute horizontal strips
void computeHorizontalStrip(int stripId, int numStrips, unsigned char* image)
{
    int startY = (SIZE * stripId) / numStrips;
    int endY = (SIZE * (stripId + 1)) / numStrips;
    
    unsigned char threadColor[3];
    computeThreadColor(stripId, numStrips, threadColor);
    
    for (int y = startY; y < endY; y++)
    {
        for (int x = 0; x < SIZE; x++)
        {
            long long num = getSpiralNumber(x, y);
            int pixelIndex = (y * SIZE + x) * 3;
            
            int iterations = isPrime(num);
            if (iterations > 0)
            {
                // Intensywność koloru zależna od liczby iteracji
                float intensity = fminf(1.0f, iterations / 50.0f);
                image[pixelIndex] = (unsigned char)(threadColor[0] * intensity);
                image[pixelIndex + 1] = (unsigned char)(threadColor[1] * intensity);
                image[pixelIndex + 2] = (unsigned char)(threadColor[2] * intensity);
            }
            else
            {
                // Jasno szare tło dla liczb niepierw
                image[pixelIndex] = 200;
                image[pixelIndex + 1] = 200;
                image[pixelIndex + 2] = 200;
            }
        }
    }
}

double runSchedulerExperiment(const ScheduleConfig& config, unsigned char* image)
{
    omp_set_nested(0);
    omp_set_num_threads(TOTAL_THREADS);
    omp_set_schedule(config.type, config.chunkSize);

    size_t bufferSize = (size_t)SIZE * (size_t)SIZE * 3;
    memset(image, 200, bufferSize);

    double startTime = omp_get_wtime();

    #pragma omp parallel
    {
        int threadId = omp_get_thread_num();
        int totalThreads = omp_get_num_threads();
        unsigned char threadColor[3];
        computeThreadColor(threadId, totalThreads, threadColor);

        #pragma omp for schedule(runtime)
        for (int y = 0; y < SIZE; y++)
        {
            for (int x = 0; x < SIZE; x++)
            {
                long long num = getSpiralNumber(x, y);
                int pixelIndex = (y * SIZE + x) * 3;

                int iterations = isPrime(num);
                if (iterations > 0)
                {
                    // Intensywność koloru zależna od liczby iteracji
                    float intensity = fminf(1.0f, iterations / 50.0f);
                    image[pixelIndex] = (unsigned char)(threadColor[0] * intensity);
                    image[pixelIndex + 1] = (unsigned char)(threadColor[1] * intensity);
                    image[pixelIndex + 2] = (unsigned char)(threadColor[2] * intensity);
                }
                else
                {
                    // Jasno szare tło dla liczb niepierw
                    image[pixelIndex] = 200;
                    image[pixelIndex + 1] = 200;
                    image[pixelIndex + 2] = 200;
                }
            }
        }
    }

    double endTime = omp_get_wtime();
    double duration = endTime - startTime;

    char chunkBuffer[16];
    if (config.chunkSize > 0)
    {
        snprintf(chunkBuffer, sizeof(chunkBuffer), "%d", config.chunkSize);
    }
    else if (config.type == omp_sched_auto)
    {
        snprintf(chunkBuffer, sizeof(chunkBuffer), "n/a");
    }
    else
    {
        snprintf(chunkBuffer, sizeof(chunkBuffer), "default");
    }

    printf("Schedule %-28s | chunk %7s | %7.3f s\n", config.label, chunkBuffer, duration);

    writeImagePPM(config.filename, config.label, image);

    return duration;
}

int main()
{
    printf("\n=== Ulam Spiral - Comparison: Nested vs Horizontal Parallelism ===\n");
    printf("Image resolution: %d x %d pixels\n", SIZE, SIZE);
    printf("Comparing 2x2 nested parallelism vs 4-thread horizontal division\n\n");
    
    imageNested = new unsigned char[SIZE * SIZE * 3];
    imageHorizontal = new unsigned char[SIZE * SIZE * 3];
    imageScheduler = new unsigned char[SIZE * SIZE * 3];
    
    size_t bufferSize = (size_t)SIZE * (size_t)SIZE * 3;
    memset(imageNested, 200, bufferSize);
    memset(imageHorizontal, 200, bufferSize);
    memset(imageScheduler, 200, bufferSize);
    
    // Method 1: Nested Parallelism (2x2 blocks)
    printf("=== Method 1: Nested Parallelism (2x2 blocks) ===\n");
    
    omp_set_nested(1);
    
    double startTimeNested = omp_get_wtime();
    
    #pragma omp parallel for num_threads(THREADS_Y)
    for (int blockY = 0; blockY < THREADS_Y; blockY++)
    {
        #pragma omp parallel for num_threads(THREADS_X)
        for (int blockX = 0; blockX < THREADS_X; blockX++)
        {
            #pragma omp critical
            {
                printf("Processing block (%d, %d)\n", blockX, blockY);
            }
            
            computeQuadrantNested(blockX, blockY, imageNested);
        }
    }
    
    double endTimeNested = omp_get_wtime();
    double durationNested = endTimeNested - startTimeNested;
    
    printf("Nested parallelism complete in %.3f seconds\n\n", durationNested);
    
    // Method 2: Horizontal Division
    printf("=== Method 2: Horizontal Division (%d threads) ===\n", TOTAL_THREADS);
    
    omp_set_nested(0);
    omp_set_num_threads(TOTAL_THREADS);
    
    double startTimeHorizontal = omp_get_wtime();
    
    #pragma omp parallel
    {
        int threadId = omp_get_thread_num();
        
        #pragma omp single
        {
            printf("Starting horizontal computation with %d threads...\n", omp_get_num_threads());
        }
        
        computeHorizontalStrip(threadId, TOTAL_THREADS, imageHorizontal);
        
        #pragma omp critical
        {
            printf("Thread %d finished strip %d\n", threadId, threadId);
        }
    }
    
    double endTimeHorizontal = omp_get_wtime();
    double durationHorizontal = endTimeHorizontal - startTimeHorizontal;
    
    printf("Horizontal division complete in %.3f seconds\n\n", durationHorizontal);

    // Scheduler comparison using runtime scheduling
    printf("=== Method 3: Runtime Schedule Comparison (%d total threads) ===\n", TOTAL_THREADS);
    printf("Testing different OpenMP schedules with schedule(runtime) ...\n");

    double scheduleDurations[NUM_SCHEDULES];
    for (int i = 0; i < NUM_SCHEDULES; i++)
    {
        scheduleDurations[i] = runSchedulerExperiment(SCHEDULE_CONFIGS[i], imageScheduler);
    }
    printf("Scheduler output images saved for each configuration above.\n\n");

    int fastestScheduleIndex = 0;
    double fastestScheduleTime = scheduleDurations[0];
    for (int i = 1; i < NUM_SCHEDULES; i++)
    {
        if (scheduleDurations[i] < fastestScheduleTime)
        {
            fastestScheduleTime = scheduleDurations[i];
            fastestScheduleIndex = i;
        }
    }

    printf("Fastest runtime schedule: %s (%.3f seconds)\n\n", SCHEDULE_CONFIGS[fastestScheduleIndex].label, fastestScheduleTime);
    
    // Write images
    printf("=== Writing images to files ===\n");
    
    writeImagePPM("ulam_spiral_nested_2x2.ppm", "Nested 2x2", imageNested);
    writeImagePPM("ulam_spiral_horizontal_4.ppm", "Horizontal 4 threads", imageHorizontal);
    
    delete[] imageNested;
    delete[] imageHorizontal;
    delete[] imageScheduler;
    
    // Performance Summary
    printf("\n=== Performance Summary ===\n");
    printf("Method                          | Time (s) | Relative to nested\n");
    printf("----------------------------------------------------------------\n");
    printf("Nested Parallelism (2x2)        | %7.3f  | baseline\n", durationNested);
    if (durationHorizontal >= durationNested)
    {
        printf("Horizontal Division (4 threads) | %7.3f  | %6.2fx slower\n", durationHorizontal, durationHorizontal / durationNested);
    }
    else
    {
        printf("Horizontal Division (4 threads) | %7.3f  | %6.2fx faster\n", durationHorizontal, durationNested / durationHorizontal);
    }

    for (int i = 0; i < NUM_SCHEDULES; i++)
    {
        double duration = scheduleDurations[i];
        if (duration >= durationNested)
        {
            printf("Runtime schedule %-16s | %7.3f  | %6.2fx slower\n", SCHEDULE_CONFIGS[i].label, duration, duration / durationNested);
        }
        else
        {
            printf("Runtime schedule %-16s | %7.3f  | %6.2fx faster\n", SCHEDULE_CONFIGS[i].label, duration, durationNested / duration);
        }
    }

    if (durationNested < durationHorizontal)
    {
        printf("\nNested parallelism is FASTER than static strips by %.2fx\n", durationHorizontal / durationNested);
    }
    else
    {
        printf("\nHorizontal division is FASTER than nested by %.2fx\n", durationNested / durationHorizontal);
    }

    double scheduleVsHorizontal = fastestScheduleTime / durationHorizontal;
    if (fastestScheduleTime < durationHorizontal)
    {
        printf("%s beats horizontal strips by %.2fx\n", SCHEDULE_CONFIGS[fastestScheduleIndex].label, durationHorizontal / fastestScheduleTime);
    }
    else
    {
        printf("Horizontal strips remain faster than %s by %.2fx\n", SCHEDULE_CONFIGS[fastestScheduleIndex].label, scheduleVsHorizontal);
    }

    if (fastestScheduleTime < durationNested)
    {
        printf("%s also outperforms nested blocks by %.2fx\n", SCHEDULE_CONFIGS[fastestScheduleIndex].label, durationNested / fastestScheduleTime);
    }
    else
    {
        printf("Nested blocks stay ahead of %s by %.2fx\n", SCHEDULE_CONFIGS[fastestScheduleIndex].label, fastestScheduleTime / durationNested);
    }
    
    printf("\n=== Analysis ===\n");
    printf("Nested: 2x2 blocks using nested parallel regions\n");
    printf("Horizontal: %d horizontal strips\n", TOTAL_THREADS);
    printf("Runtime: schedule(runtime) sweep across static/dynamic/guided/auto\n");
    printf("Both methods use %d total threads\n", TOTAL_THREADS);
    
    return 0;
}
