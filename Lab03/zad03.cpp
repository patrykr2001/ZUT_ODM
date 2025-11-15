#include <stdio.h>
#include <math.h>
#include <omp.h>

// Global variables
/* screen ( integer) coordinate */
const int iXmax = 10000; 
const int iYmax = 10000;
/* world ( double) coordinate = parameter plane*/
const double CxMin = -2.5;
const double CxMax = 1.5;
const double CyMin = -2.0;
const double CyMax = 2.0;
/* */
double PixelWidth = (CxMax - CxMin) / iXmax;
double PixelHeight = (CyMax - CyMin) / iYmax;
/* color component ( R or G or B) is coded from 0 to 255 */
/* it is 24 bit color RGB file */
const int MaxColorComponentValue = 255; 
char *comment = "# "; /* comment should start with # */

/* */
const int IterationMax = 200;
/* bail-out value , radius of circle ;  */
const double EscapeRadius = 2;
double ER2 = EscapeRadius * EscapeRadius;

// Schedule types for testing
const char* scheduleNames[] = {"static (default)", "static,1", "static,100", "dynamic", "dynamic,1", "dynamic,100", "guided", "auto"};
const int numSchedules = 8;

// Fixed number of threads for schedule comparison
const int FIXED_THREADS = 8;

// Allocate memory for all images - array of pointers to images
unsigned char* images[numSchedules];
double executionTimes[numSchedules];

// Function to compute one row of the Mandelbrot set
// This function is called by each thread for different rows
void computeRow(int iY, unsigned char* image)
{
    // Local variables - private to each thread
    double Cx, Cy;
    double Zx, Zy;
    double Zx2, Zy2;
    int Iteration;
    int iX;
    int pixelIndex;
    
    // Get thread ID for coloring
    int threadId = omp_get_thread_num();
    int numThreads = omp_get_num_threads();
    
    // Generate a unique color for this thread
    unsigned char threadColor[3];
    // Use HSV to RGB conversion for distinct colors
    float hue = (float)threadId / numThreads;
    float saturation = 0.8f;
    float value = 0.95f;
    
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
    
    Cy = CyMin + iY * PixelHeight;
    if (fabs(Cy) < PixelHeight/2) Cy = 0.0; /* Main antenna */
    
    for(iX = 0; iX < iXmax; iX++)
    {
        Cx = CxMin + iX * PixelWidth;
        /* initial value of orbit = critical point Z= 0 */
        Zx = 0.0;
        Zy = 0.0;
        Zx2 = Zx * Zx;
        Zy2 = Zy * Zy;
        
        /* Mandelbrot iteration */
        for (Iteration = 0; Iteration < IterationMax && ((Zx2 + Zy2) < ER2); Iteration++)
        {
            Zy = 2 * Zx * Zy + Cy;
            Zx = Zx2 - Zy2 + Cx;
            Zx2 = Zx * Zx;
            Zy2 = Zy * Zy;
        }
        
        /* compute pixel color (24 bit = 3 bytes) */
        pixelIndex = (iY * iXmax + iX) * 3;
        if (Iteration == IterationMax)
        {
            /*  interior of Mandelbrot set = black */
            image[pixelIndex] = 0;
            image[pixelIndex + 1] = 0;
            image[pixelIndex + 2] = 0;
        }
        else
        {
            /* exterior of Mandelbrot set = colored by thread */
            image[pixelIndex] = threadColor[0];     /* Red */
            image[pixelIndex + 1] = threadColor[1];  /* Green */
            image[pixelIndex + 2] = threadColor[2];  /* Blue */
        }
    }
}

int main()
{
    // Allocate memory for each image
    for (int i = 0; i < numSchedules; i++)
    {
        images[i] = new unsigned char[iXmax * iYmax * 3];
    }
    
    // Set fixed number of threads for all tests
    omp_set_num_threads(FIXED_THREADS);
    
    printf("\n=== Testing different OpenMP schedule strategies with %d threads ===\n", FIXED_THREADS);
    printf("Image resolution: %d x %d pixels\n", iXmax, iYmax);
    printf("Maximum iterations: %d\n\n", IterationMax);
    
    // Test each schedule type
    for (int schedIdx = 0; schedIdx < numSchedules; schedIdx++)
    {
        printf("\n=== Schedule: %s ===\n", scheduleNames[schedIdx]);
        
        // Measure execution time with omp_get_wtime
        double startTime = omp_get_wtime();
        
        unsigned char* image = images[schedIdx];
        
        // Select appropriate schedule based on index
        switch(schedIdx)
        {
            case 0: // static (default)
                #pragma omp parallel for shared(image) schedule(static)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 1: // static,1
                #pragma omp parallel for shared(image) schedule(static, 1)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 2: // static,100
                #pragma omp parallel for shared(image) schedule(static, 100)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 3: // dynamic (default chunk size)
                #pragma omp parallel for shared(image) schedule(dynamic)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 4: // dynamic,1
                #pragma omp parallel for shared(image) schedule(dynamic, 1)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 5: // dynamic,100
                #pragma omp parallel for shared(image) schedule(dynamic, 100)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 6: // guided
                #pragma omp parallel for shared(image) schedule(guided)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
                
            case 7: // auto
                #pragma omp parallel for shared(image) schedule(auto)
                for(int iY = 0; iY < iYmax; iY++)
                {
                    computeRow(iY, image);
                }
                break;
        }
        
        double endTime = omp_get_wtime();
        double duration = endTime - startTime;
        
        executionTimes[schedIdx] = duration;
        printf("Computation complete in %.3f seconds\n", executionTimes[schedIdx]);
    }
    
    printf("\n=== Writing all images to files ===\n");
    
    // Write all images to files after all computations
    for (int i = 0; i < numSchedules; i++)
    {
        char filename[100];
        // Create safe filename from schedule name
        const char* schedName = scheduleNames[i];
        char safeScheduleName[50];
        int j = 0;
        for (int k = 0; schedName[k] != '\0' && j < 49; k++)
        {
            if (schedName[k] == ' ')
                safeScheduleName[j++] = '_';
            else if (schedName[k] == ',')
                safeScheduleName[j++] = '_';
            else if (schedName[k] == '(')
                continue;
            else if (schedName[k] == ')')
                continue;
            else
                safeScheduleName[j++] = schedName[k];
        }
        safeScheduleName[j] = '\0';
        
        sprintf(filename, "mandelbrot_schedule_%s.ppm", safeScheduleName);
        
        /*create new file,give it a name and open it in binary mode  */
        FILE* fp = fopen(filename,"wb"); /* b -  binary mode */
        /*write ASCII header to the file*/
        fprintf(fp,"P6\n %s\n %d\n %d\n %d\n",comment,iXmax,iYmax,MaxColorComponentValue);
        
        /* write image data bytes to the file*/
        fwrite(images[i], 1, iXmax * iYmax * 3, fp);
        
        fclose(fp);
        
        printf("Image saved to %s (computed in %.3f seconds)\n", filename, executionTimes[i]);
    }
    
    // Free allocated memory
    for (int i = 0; i < numSchedules; i++)
    {
        delete[] images[i];
    }
    
    printf("\n=== Performance Summary ===\n");
    printf("Schedule Strategy              | Time (s) | Speedup vs static\n");
    printf("-----------------------------------------------------------\n");
    for (int i = 0; i < numSchedules; i++)
    {
        printf("%-30s | %7.3f  |", scheduleNames[i], executionTimes[i]);
        if (i > 0)
        {
            float speedup = (float)executionTimes[0] / executionTimes[i];
            printf(" %.2fx", speedup);
        }
        else
        {
            printf(" baseline");
        }
        printf("\n");
    }
    
    // Find best schedule
    int bestIdx = 0;
    for (int i = 1; i < numSchedules; i++)
    {
        if (executionTimes[i] < executionTimes[bestIdx])
            bestIdx = i;
    }
    
    printf("Best schedule: %s (%.3f seconds)\n", scheduleNames[bestIdx], executionTimes[bestIdx]);
    
    return 0;
}
