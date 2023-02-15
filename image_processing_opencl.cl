__kernel void naive_kernel(
        __global const float* restrict in_image,
        __global float* restrict out_image,
        __private int size
)
{
    
    // global 2D NDRange sizes (size of the image)
    int num_cols = get_global_size(0);
    int num_rows = get_global_size(1);
    // point in currently being executed (each pixel)
    int senterX = get_global_id(0);
    int senterY = get_global_id(1);

    // For each pixel we compute a box blur
    float3 sum = (float3) (0.0f);
    int countIncluded = 0;
    int startY = size;
    int endY = size;
    int startX = size;
    int endX = size;
    if(senterY < size){
        startY = senterY;
    }
    if(senterY + size >= num_rows){
        endY = num_rows - senterY - 1;
    }

   
    for(int y = -startY; y <= endY; y++) {

        int currentY = senterY + y;
        if(senterX < size){
            startX = senterX;
        }
        if(senterX + size >= num_cols){
            endX = num_cols - senterX - 1;
        }
            
        for(int x = -startX; x <= endX; x++) {
        
            int currentX = senterX + x;
            

            // Now we can begin
            int offsetOfThePixel = (num_cols * currentY + currentX);
            float3 tmp = vload3(offsetOfThePixel, in_image);
            sum += tmp;
            // Keep track of how many values we have included
            countIncluded++;
        }
    }

    // Now we compute the final value
    float3 value = sum / countIncluded;

    // Update the output image
    int offsetOfThePixel = (num_cols * senterY + senterX);
    vstore3(value, offsetOfThePixel, out_image);
}
