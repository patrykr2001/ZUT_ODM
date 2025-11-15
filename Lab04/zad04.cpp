#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>

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

// Function to check if a number is prime
bool isPrime(long long n)
{
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    
    for (long long i = 5; i * i <= n; i += 6)
    {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
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
            
            if (isPrime(num))
            {
                image[pixelIndex] = threadColor[0];
                image[pixelIndex + 1] = threadColor[1];
                image[pixelIndex + 2] = threadColor[2];
            }
            else
            {
                image[pixelIndex] = 50;
                image[pixelIndex + 1] = 50;
                image[pixelIndex + 2] = 50;
            }
        }
    }
}

// Function to compute horizontal strips
void computeHorizontalStrip(int stripId, int numStrips, unsigned char* image)
{
    int startY = (SIZE * stripId) / numStrips;
    int endY = (SIZE * (stripId + 1)) / numStrips;
    
    unsigned char threadColor[3];
    float hue = (float)stripId / (float)numStrips;
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
        for (int x = 0; x < SIZE; x++)
        {
            long long num = getSpiralNumber(x, y);
            int pixelIndex = (y * SIZE + x) * 3;
            
            if (isPrime(num))
            {
                image[pixelIndex] = threadColor[0];
                image[pixelIndex + 1] = threadColor[1];
                image[pixelIndex + 2] = threadColor[2];
            }
            else
            {
                image[pixelIndex] = 50;
                image[pixelIndex + 1] = 50;
                image[pixelIndex + 2] = 50;
            }
        }
    }
}

int main()
{
    printf("\n=== Ulam Spiral - Comparison: Nested vs Horizontal Parallelism ===\n");
    printf("Image resolution: %d x %d pixels\n", SIZE, SIZE);
    printf("Comparing 2x2 nested parallelism vs 4-thread horizontal division\n\n");
    
    imageNested = new unsigned char[SIZE * SIZE * 3];
    imageHorizontal = new unsigned char[SIZE * SIZE * 3];
    
    for (int i = 0; i < SIZE * SIZE * 3; i++)
    {
        imageNested[i] = 50;
        imageHorizontal[i] = 50;
    }
    
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
    
    // Write images
    printf("=== Writing images to files ===\n");
    
    char filename1[] = "ulam_spiral_nested_2x2.ppm";
    FILE* fp1 = fopen(filename1, "wb");
    if (fp1 != NULL)
    {
        fprintf(fp1, "P6\n# Nested 2x2\n%d\n%d\n%d\n", SIZE, SIZE, MaxColorComponentValue);
        fwrite(imageNested, 1, SIZE * SIZE * 3, fp1);
        fclose(fp1);
        printf("Nested image saved to %s\n", filename1);
    }
    
    char filename2[] = "ulam_spiral_horizontal_4.ppm";
    FILE* fp2 = fopen(filename2, "wb");
    if (fp2 != NULL)
    {
        fprintf(fp2, "P6\n# Horizontal 4\n%d\n%d\n%d\n", SIZE, SIZE, MaxColorComponentValue);
        fwrite(imageHorizontal, 1, SIZE * SIZE * 3, fp2);
        fclose(fp2);
        printf("Horizontal image saved to %s\n", filename2);
    }
    
    delete[] imageNested;
    delete[] imageHorizontal;
    
    // Performance Summary
    printf("\n=== Performance Summary ===\n");
    printf("Method                          | Time (s) | Relative Performance\n");
    printf("----------------------------------------------------------------\n");
    printf("Nested Parallelism (2x2)        | %7.3f  | baseline\n", durationNested);
    printf("Horizontal Division (4 threads) | %7.3f  | ", durationHorizontal);
    
    if (durationNested < durationHorizontal)
    {
        printf("%.2fx slower\n", durationHorizontal / durationNested);
        printf("\nNested parallelism is FASTER by %.2fx\n", durationHorizontal / durationNested);
    }
    else
    {
        printf("%.2fx faster\n", durationNested / durationHorizontal);
        printf("\nHorizontal division is FASTER by %.2fx\n", durationNested / durationHorizontal);
    }
    
    printf("\n=== Analysis ===\n");
    printf("Nested: 2x2 blocks using nested parallel regions\n");
    printf("Horizontal: %d horizontal strips\n", TOTAL_THREADS);
    printf("Both methods use %d total threads\n", TOTAL_THREADS);
    
    return 0;
}
