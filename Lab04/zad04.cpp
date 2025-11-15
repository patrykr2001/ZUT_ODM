#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>

// Global variables
const int SIZE = 9999;  // Size of the spiral (should be odd for centered spiral)
const int iXmax = SIZE; 
const int iYmax = SIZE;

// Color definitions
const int MaxColorComponentValue = 255; 
char *comment = "# Ulam Spiral - OpenMP with quadrant division";

// Image buffer
unsigned char* image;

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
// Using the Ulam spiral algorithm
long long getSpiralNumber(int x, int y)
{
    // Convert to centered coordinates
    int cx = x - SIZE / 2;
    int cy = y - SIZE / 2;
    
    // Determine which "ring" we're on
    int ring = (int)fmax(abs(cx), abs(cy));
    
    if (ring == 0) return 1; // Center
    
    // Calculate the number at this position
    // The spiral starts at 1 in the center and goes right, then spirals counter-clockwise
    long long maxNumInPrevRing = (2 * ring - 1) * (2 * ring - 1);
    long long numInRing = maxNumInPrevRing;
    
    // We're on the right edge of the ring
    if (cx == ring)
    {
        numInRing += ring + cy;
    }
    // Top edge
    else if (cy == ring)
    {
        numInRing += 3 * ring - cx;
    }
    // Left edge
    else if (cx == -ring)
    {
        numInRing += 5 * ring - cy;
    }
    // Bottom edge
    else // cy == -ring
    {
        numInRing += 7 * ring + cx;
    }
    
    return numInRing;
}

// Function to compute one quadrant of the spiral
// quadrant: 0=top-right, 1=top-left, 2=bottom-left, 3=bottom-right
void computeQuadrant(int quadrant, int threadId)
{
    int startX, endX, startY, endY;
    
    int midX = SIZE / 2;
    int midY = SIZE / 2;
    
    // Define quadrant boundaries
    switch(quadrant)
    {
        case 0: // Top-right
            startX = midX;
            endX = SIZE;
            startY = 0;
            endY = midY;
            break;
        case 1: // Top-left
            startX = 0;
            endX = midX;
            startY = 0;
            endY = midY;
            break;
        case 2: // Bottom-left
            startX = 0;
            endX = midX;
            startY = midY;
            endY = SIZE;
            break;
        case 3: // Bottom-right
            startX = midX;
            endX = SIZE;
            startY = midY;
            endY = SIZE;
            break;
    }
    
    // Generate a unique color for this thread/quadrant
    unsigned char threadColor[3];
    float hue = (float)threadId / 4.0f;
    float saturation = 1.0f;
    float value = 1.0f;
    
    // Simple HSV to RGB conversion
    int h_i = (int)(hue * 6);
    float f = hue * 6 - h_i;
    float p = value * (1 - saturation);
    float q = value * (1 - f * saturation);
    float t = value * (1 - (1 - f) * saturation);
    
    float r, g, b;
    switch(h_i) {
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
    
    // Process each pixel in this quadrant
    for (int y = startY; y < endY; y++)
    {
        for (int x = startX; x < endX; x++)
        {
            long long num = getSpiralNumber(x, y);
            int pixelIndex = (y * SIZE + x) * 3;
            
            if (isPrime(num))
            {
                // Prime numbers - colored by quadrant
                image[pixelIndex] = threadColor[0];     // Red
                image[pixelIndex + 1] = threadColor[1]; // Green
                image[pixelIndex + 2] = threadColor[2]; // Blue
            }
            else
            {
                // Non-prime numbers - dark gray
                image[pixelIndex] = 50;
                image[pixelIndex + 1] = 50;
                image[pixelIndex + 2] = 50;
            }
        }
    }
}

int main()
{
    printf("\n=== Ulam Spiral Generator with OpenMP (Quadrant Division) ===\n");
    printf("Image resolution: %d x %d pixels\n", SIZE, SIZE);
    printf("Using 4 threads (one per quadrant)\n\n");
    
    // Allocate memory for the image
    image = new unsigned char[SIZE * SIZE * 3];
    
    // Initialize image to white
    for (int i = 0; i < SIZE * SIZE * 3; i++)
    {
        image[i] = 255;
    }
    
    // Set number of threads to 4 (one per quadrant)
    omp_set_num_threads(4);
    
    double startTime = omp_get_wtime();
    
    // Parallel computation - each thread processes one quadrant
    #pragma omp parallel
    {
        int threadId = omp_get_thread_num();
        
        #pragma omp single
        {
            printf("Starting computation with %d threads...\n", omp_get_num_threads());
        }
        
        // Each thread computes its assigned quadrant
        computeQuadrant(threadId, threadId);
        
        #pragma omp critical
        {
            printf("Thread %d finished quadrant %d\n", threadId, threadId);
        }
    }
    
    double endTime = omp_get_wtime();
    double duration = endTime - startTime;
    
    printf("\nComputation complete in %.3f seconds\n", duration);
    
    // Write image to file
    printf("Writing image to file...\n");
    
    char filename[] = "ulam_spiral_quadrants.ppm";
    FILE* fp = fopen(filename, "wb");
    
    if (fp == NULL)
    {
        printf("Error: Could not create file %s\n", filename);
        delete[] image;
        return 1;
    }
    
    // Write PPM header
    fprintf(fp, "P6\n%s\n%d\n%d\n%d\n", comment, SIZE, SIZE, MaxColorComponentValue);
    
    // Write image data
    fwrite(image, 1, SIZE * SIZE * 3, fp);
    
    fclose(fp);
    
    printf("Image saved to %s\n", filename);
    
    // Free memory
    delete[] image;
    
    printf("\n=== Ulam Spiral Generation Complete ===\n");
    printf("Prime numbers are colored by quadrant\n");
    printf("Non-prime numbers are white/light gray\n");
    printf("Each quadrant was processed by a separate thread\n");
    
    return 0;
}
