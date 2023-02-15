#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <omp.h>

#include "ppm.h"

// Image from:
// http://7-themes.com/6971875-funny-flowers-pictures.html

typedef struct {
     float red,green,blue;
} AccuratePixel;

typedef struct {
     uint16_t x, y;
     AccuratePixel *data;
} AccurateImage;

// Convert ppm to high precision format.
AccurateImage *convertToAccurateImage(PPMImage *image) {
	// Make a copy
	AccurateImage *imageAccurate;
	imageAccurate = (AccurateImage *)malloc(sizeof(AccurateImage));
	imageAccurate->data = (AccuratePixel*)malloc(image->x * image->y * sizeof(AccuratePixel));
	omp_set_num_threads(4);
	#pragma omp parallel for
	for(uint32_t i = 0; i < image->x * image->y; i++) {
		imageAccurate->data[i].red   = (float) image->data[i].red;
		imageAccurate->data[i].green = (float) image->data[i].green;
		imageAccurate->data[i].blue  = (float) image->data[i].blue;
	}
	imageAccurate->x = image->x;
	imageAccurate->y = image->y;
	
	return imageAccurate;
}

PPMImage * convertToPPPMImage(AccurateImage *imageIn) {
    PPMImage *imageOut;
    imageOut = (PPMImage *)malloc(sizeof(PPMImage));
    imageOut->data = (PPMPixel*)malloc(imageIn->x * imageIn->y * sizeof(PPMPixel));

    imageOut->x = imageIn->x;
    imageOut->y = imageIn->y;
	omp_set_num_threads(4);
	#pragma omp parallel for
    for(uint32_t i = 0; i < imageIn->x * imageIn->y; i++) {
        imageOut->data[i].red = imageIn->data[i].red;
        imageOut->data[i].green = imageIn->data[i].green;
        imageOut->data[i].blue = imageIn->data[i].blue;
    }
    return imageOut;
}

// blur one color channel
void blurIteration(AccurateImage *imageOut, AccurateImage *imageIn, int size) {
	// TODO optimizations to be made here!
	// Iterate over each pixel
	int numberOfValuesInEachRow = imageIn->x;
	omp_set_num_threads(4);
	#pragma omp parallel for
	for(uint16_t senterY = 0; senterY < imageIn->y; senterY++) {
		// TODO do they need to be inside here?
		float sum_r = 0;
		float sum_g = 0;
		float sum_b = 0;
		int countIncluded = 0;

		int startY = size;
		int endY = size;
		

		if(senterY < size){
			startY = senterY;
		}

		if(senterY + size >= imageIn->y){
			endY = imageIn->y - senterY - 1;
		}
		// if((senterY + i) < 0 || senterY + i >= imageIn->y) continue;	
		
		for(uint16_t senterX = 0; senterX < imageIn->x; senterX++) {
			// TODO this can be improved (will take some work though)
			// TODO can save the value above for even faster computations(?)
			
			if(senterX != 0){
				if(((senterX + size) >= imageIn->x && (senterX - size - 1) < 0)){
					continue;
				}
				int con1 = (senterX - size - 1) >= 0;
				int con2 = (senterX + size) < imageIn->x;
				if(con1){
					countIncluded -= (startY + endY + 1);
				}
				if(con2){
					countIncluded += (startY + endY + 1);
				}

				for(int i = -startY; i <= endY; i++) {
					// Remove x values	
					if(con1){
						int offsetOfThePixelRemove = (numberOfValuesInEachRow * (senterY + i) + (senterX - size - 1));
						sum_r -= imageIn->data[offsetOfThePixelRemove].red;
						sum_g -= imageIn->data[offsetOfThePixelRemove].green;
						sum_b -= imageIn->data[offsetOfThePixelRemove].blue;		
					}
					
					if(con2){
						int offsetOfThePixelAdd = (numberOfValuesInEachRow * (senterY + i) + (senterX + size));
						sum_r += imageIn->data[offsetOfThePixelAdd].red;
						sum_g += imageIn->data[offsetOfThePixelAdd].green;
						sum_b += imageIn->data[offsetOfThePixelAdd].blue;
					}
					
					
				}
			}
			else {
				countIncluded = 0;
				for(int y = -startY; y <= endY; y++) {
					int currentY = senterY + y;
							
					int currentYCalc = numberOfValuesInEachRow * currentY;
					for(int x = 0; x <= size; x++) {
						// if(senterX + x >= imageIn->x){
						// 	break;
						// }
						// Check if we are outside the bounds
						int currentX = senterX + x;
						// Now we can begin
						
						int offsetOfThePixel = (currentYCalc + currentX);

						sum_r += imageIn->data[offsetOfThePixel].red;
						sum_g += imageIn->data[offsetOfThePixel].green;
						sum_b += imageIn->data[offsetOfThePixel].blue;

						// Keep track of how many values we have included
						countIncluded++;
					}
				}
			}
			
			// Now we compute the final value
			float value_r = sum_r / countIncluded;
			float value_g = sum_g / countIncluded;
			float value_b = sum_b / countIncluded;


			// Update the output image
			int offsetOfThePixel = (numberOfValuesInEachRow * senterY + senterX);
			
			imageOut->data[offsetOfThePixel].red = value_r;
			imageOut->data[offsetOfThePixel].green = value_g;
			imageOut->data[offsetOfThePixel].blue = value_b;
		}

	}
	
}


// Perform the final step, and return it as ppm.
PPMImage * imageDifference(AccurateImage *imageInSmall, AccurateImage *imageInLarge) {
	PPMImage *imageOut;
	imageOut = (PPMImage *)malloc(sizeof(PPMImage));
	imageOut->data = (PPMPixel*)malloc(imageInSmall->x * imageInSmall->y * sizeof(PPMPixel));
	
	imageOut->x = imageInSmall->x;
	imageOut->y = imageInSmall->y;
	omp_set_num_threads(4);
	#pragma omp parallel for
	for (uint32_t i = 0; i < imageInSmall->x * imageInSmall->y; i++) {
        imageOut->data[i].red   = ((int)(imageInLarge->data[i].red - imageInSmall->data[i].red)) % 255;
        imageOut->data[i].green = ((int)(imageInLarge->data[i].green - imageInSmall->data[i].green)) % 255;
        imageOut->data[i].blue  = ((int)(imageInLarge->data[i].blue - imageInSmall->data[i].blue)) % 255;
    }
	return imageOut;
}


int main(int argc, char** argv) {
    // read image
    PPMImage *image;
    // select where to read the image from
    if(argc > 1) {
        // from file for debugging (with argument)
        image = readPPM("flower.ppm");
    } else {
        // from stdin for cmb
        image = readStreamPPM(stdin);
    }
	
	
	AccurateImage *imageAccurate1_tiny = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_tiny = convertToAccurateImage(image);
	AccurateImage *imageAccurate1_small = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_small = convertToAccurateImage(image);
	AccurateImage *imageAccurate1_medium = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_medium = convertToAccurateImage(image);
	AccurateImage *imageAccurate1_large = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_large = convertToAccurateImage(image);
	
	// Process the tiny case:
	
	int size = 2;
	blurIteration(imageAccurate2_tiny, imageAccurate1_tiny, size);
	blurIteration(imageAccurate1_tiny, imageAccurate2_tiny, size);
	blurIteration(imageAccurate2_tiny, imageAccurate1_tiny, size);
	blurIteration(imageAccurate1_tiny, imageAccurate2_tiny, size);
	blurIteration(imageAccurate2_tiny, imageAccurate1_tiny, size);
	
	
	

	
	// Process the small case:
	
	size = 3;
	blurIteration(imageAccurate2_small, imageAccurate1_small, size);
	blurIteration(imageAccurate1_small, imageAccurate2_small, size);
	blurIteration(imageAccurate2_small, imageAccurate1_small, size);
	blurIteration(imageAccurate1_small, imageAccurate2_small, size);
	blurIteration(imageAccurate2_small, imageAccurate1_small, size);
	

    // an intermediate step can be saved for debugging like this
//    writePPM("imageAccurate2_tiny.ppm", convertToPPPMImage(imageAccurate2_tiny));
	
	
	
	// Process the medium case:
	size = 5;
	blurIteration(imageAccurate2_medium, imageAccurate1_medium, size);
	blurIteration(imageAccurate1_medium, imageAccurate2_medium, size);
	blurIteration(imageAccurate2_medium, imageAccurate1_medium, size);
	blurIteration(imageAccurate1_medium, imageAccurate2_medium, size);
	blurIteration(imageAccurate2_medium, imageAccurate1_medium, size);

	
	
	
	// Do each color channel
	size = 8;
	blurIteration(imageAccurate2_large, imageAccurate1_large, size);
	blurIteration(imageAccurate1_large, imageAccurate2_large, size);
	blurIteration(imageAccurate2_large, imageAccurate1_large, size);
	blurIteration(imageAccurate1_large, imageAccurate2_large, size);
	blurIteration(imageAccurate2_large, imageAccurate1_large, size);
	
	// calculate difference
	PPMImage *final_tiny = imageDifference(imageAccurate2_tiny, imageAccurate2_small);
    PPMImage *final_small = imageDifference(imageAccurate2_small, imageAccurate2_medium);
    PPMImage *final_medium = imageDifference(imageAccurate2_medium, imageAccurate2_large);
	// Save the images.
    if(argc > 1) {
        writePPM("flower_tiny.ppm", final_tiny);
        writePPM("flower_small.ppm", final_small);
        writePPM("flower_medium.ppm", final_medium);
    } else {
        writeStreamPPM(stdout, final_tiny);
        writeStreamPPM(stdout, final_small);
        writeStreamPPM(stdout, final_medium);
    }
	
}

