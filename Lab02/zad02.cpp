/* 
 C++ program with C++11 threads:
 --------------------------------
  1. draws Mandelbrot set for Fc(z)=z*z +c
  using Mandelbrot algorithm ( boolean escape time )
 -------------------------------         
 2. technique of creating ppm file is  based on the code of Claudio Rocchini
 http://en.wikipedia.org/wiki/Image:Color_complex_plot.jpg
 create 24 bit color graphic file ,  portable pixmap file = PPM 
 see http://en.wikipedia.org/wiki/Portable_pixmap
 to see the file use external application ( graphic viewer)
  */
 #include <stdio.h>
 #include <math.h>
 #include <thread>
 #include <vector>
 #include <algorithm>
 #include <chrono>
 #include <string>

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

 // Number of test configurations
 const int numConfigs = 5; // 1, 2, 4, 8, 16 threads

 // Allocate memory for all images - array of pointers to images
 unsigned char* images[numConfigs];
 int threadCounts[numConfigs] = {1, 2, 4, 8, 16};
 long long executionTimes[numConfigs];

 // Function to compute a range of rows for the Mandelbrot set
 void computeRows(unsigned char* image, int startRow, int endRow, int threadId, int totalThreads)
 {
     // Local variables used in thread function
     double Cx, Cy;
     double Zx, Zy;
     double Zx2, Zy2;
     int Iteration;
     
     // Generate a unique color for this thread
     unsigned char threadColor[3];
     // Use HSV to RGB conversion for distinct colors
     float hue = (float)threadId / totalThreads;
     float saturation = 0.7f;
     float value = 0.9f;
     
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
     
     for(int iY = startRow; iY < endRow; iY++)
     {
         Cy = CyMin + iY * PixelHeight;
         if (fabs(Cy) < PixelHeight/2) Cy = 0.0; /* Main antenna */
         
         for(int iX = 0; iX < iXmax; iX++)
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
             int pixelIndex = (iY * iXmax + iX) * 3;
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
 }

 int main()
 {
        // Allocate memory for each image
        for (int i = 0; i < numConfigs; i++)
        {
            images[i] = new unsigned char[iXmax * iYmax * 3];
        }
        
        // Loop through different thread counts: 1, 2, 4, 8, 16
        int configIndex = 0;
        for (unsigned int numThreads = 1; numThreads <= 16; numThreads *= 2)
        {
            printf("\n=== Testing with %d thread(s) ===\n", numThreads);
            
            // Measure execution time
            auto startTime = std::chrono::high_resolution_clock::now();
            
            std::vector<std::thread> threads;
            
            // Divide rows among threads
            int rowsPerThread = iYmax / numThreads;
            
            // Create and launch threads
            for (unsigned int t = 0; t < numThreads; t++)
            {
                int startRow = t * rowsPerThread;
                int endRow = (t == numThreads - 1) ? iYmax : (t + 1) * rowsPerThread;
                
                threads.push_back(std::thread(computeRows, images[configIndex], startRow, endRow, t, numThreads));
            }
            
            // Wait for all threads to complete
            for (auto& thread : threads)
            {
                thread.join();
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            executionTimes[configIndex] = duration.count();
            printf("Computation complete in %lld ms\n", executionTimes[configIndex]);
            
            configIndex++;
        }
        
        printf("\n=== Writing all images to files ===\n");
        
        // Write all images to files after all computations
        for (int i = 0; i < numConfigs; i++)
        {
            char filename[100];
            sprintf(filename, "mandelbrot_%d_threads.ppm", threadCounts[i]);
            
            /*create new file,give it a name and open it in binary mode  */
            FILE* fp = fopen(filename,"wb"); /* b -  binary mode */
            /*write ASCII header to the file*/
            fprintf(fp,"P6\n %s\n %d\n %d\n %d\n",comment,iXmax,iYmax,MaxColorComponentValue);
            
            /* write image data bytes to the file*/
            fwrite(images[i], 1, iXmax * iYmax * 3, fp);
            
            fclose(fp);
            
            printf("Image saved to %s (computed in %lld ms)\n", filename, executionTimes[i]);
        }
        
        // Free allocated memory
        for (int i = 0; i < numConfigs; i++)
        {
            delete[] images[i];
        }
        
        printf("\n=== All tests completed ===\n");
        printf("\nPerformance Summary:\n");
        for (int i = 0; i < numConfigs; i++)
        {
            printf("%d thread(s): %lld ms", threadCounts[i], executionTimes[i]);
            if (i > 0)
            {
                float speedup = (float)executionTimes[0] / executionTimes[i];
                printf(" (speedup: %.2fx)", speedup);
            }
            printf("\n");
        }
        
        return 0;
 }