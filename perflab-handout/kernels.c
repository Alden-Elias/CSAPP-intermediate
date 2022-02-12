/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following team struct 
 */
team_t team = {
    "Peng Jiajun",              /* Team name */

    "Peng Jiajun",     /* First member full name */
    "3257171856@qq.com",  /* First member email address */

    "",                   /* Second member full name (leave blank if none) */
    ""                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j, k, r;
    int BLOKSIZE = 16;
    int dim1 = dim-1;
    int dst_base = 0, src_base;
    int dst_RIDX, src_RIDX;

    for (i = 0; i < dim; i += BLOKSIZE){
        for (j = 0; j < dim; j += BLOKSIZE){
            dst_base = RIDX(i, j, dim);
            src_base = RIDX(j, dim1-i, dim);
            for(k = 0; k < BLOKSIZE; ++k){
                dst_RIDX = dst_base;
                src_RIDX = src_base;
                for(r = 0; r < BLOKSIZE; ++r){
	                dst[dst_RIDX] = src[src_RIDX];
                    ++dst_RIDX;
                    src_RIDX += dim;
                }
                dst_base += dim;
                --src_base;
            }
        }
    }
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    naive_rotate(dim, src, dst);
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
    /* ... Register additional test functions here */
}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */
static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    int min1 = min(i+1, dim-1);
    int min2 = min(j+1, dim-1);

    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min1; ii++){
    	for(jj = max(j-1, 0); jj <= min2; jj++){
	        accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);
        }
    }

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth 
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    
    // // 第二次优化
    // int i, j, k;
    // int dst_RIDX = 0;
    // int new_i, new_j;
    // int new_r, new_g, new_b;
    // int tmp_RIDX;
    // int count;

    // for (i = 0; i < dim; i++){
	    // for (j = 0; j < dim; j++){
    //         new_r = src[dst_RIDX].red;
    //         new_g = src[dst_RIDX].green;
    //         new_b = src[dst_RIDX].blue;
    //         count = 1;

    //         for(k = 0; k < 8; k++){
    //             new_i = i+dx[k];
    //             new_j = j+dy[k];
    //             if(dim <= new_i || new_i < 0 || dim <= new_j || new_j < 0) continue;
    //             tmp_RIDX = RIDX(new_i, new_j, dim);
    //             new_r += src[tmp_RIDX].red;
    //             new_g += src[tmp_RIDX].green;
    //             new_b += src[tmp_RIDX].blue;
    //             ++count;
    //         }

    //         dst[dst_RIDX].red = (unsigned short)(new_r/count);
    //         dst[dst_RIDX].green = (unsigned short)(new_g/count);
    //         dst[dst_RIDX].blue = (unsigned short)(new_b/count);

    //         ++dst_RIDX;
    //     }
    // }

    // 第三次优化
    int i ,j;
    pixel *tmp1, *tmp2, *tmp3, *tmp4, *tmp5, *tmp6, *tmp7, *tmp8, *tmp9;

    tmp1 = src-1-dim;
    tmp2 = src-dim;
    tmp3 = src+1-dim;
    tmp4 = src-1;
    tmp5 = src;
    tmp6 = src+1;
    tmp7 = src-1+dim;
    tmp8 = src+dim;
    tmp9 = src+1+dim;

    //左上角
    dst->red = (tmp5->red + tmp6->red + tmp8->red + tmp9->red)/4;
    dst->green = (tmp5->green + tmp6->green + tmp8->green + tmp9->green)/4;
    dst->blue = (tmp5->blue + tmp6->blue + tmp8->blue + tmp9->blue)/4;
    
    ++tmp1;
    ++tmp2;
    ++tmp3;
    ++tmp4;
    ++tmp5;
    ++tmp6;
    ++tmp7;
    ++tmp8;
    ++tmp9;
    ++dst;

    //上横线
    for(i = 2; i < dim; i++){

        dst->red = (tmp4->red + tmp5->red + tmp6->red + tmp7->red + tmp8->red + tmp9->red)/6;
        dst->green = (tmp4->green + tmp5->green + tmp6->green + tmp7->green + tmp8->green + tmp9->green)/6;
        dst->blue = (tmp4->blue + tmp5->blue + tmp6->blue + tmp7->blue + tmp8->blue + tmp9->blue)/6;
    
        ++tmp1;
        ++tmp2;
        ++tmp3;
        ++tmp4;
        ++tmp5;
        ++tmp6;
        ++tmp7;
        ++tmp8;
        ++tmp9;
        ++dst;

    }

    //右上角
    dst->red = (tmp4->red + tmp5->red + tmp7->red + tmp8->red)/4;
    dst->green = (tmp4->green + tmp5->green + tmp7->green + tmp8->green)/4;
    dst->blue = (tmp4->blue + tmp5->blue + tmp7->blue + tmp8->blue)/4;
    
    ++tmp1;
    ++tmp2;
    ++tmp3;
    ++tmp4;
    ++tmp5;
    ++tmp6;
    ++tmp7;
    ++tmp8;
    ++tmp9;
    ++dst;

    for(i = 2; i < dim; i++){
        
        //左侧
        dst->red = (tmp2->red + tmp3->red + tmp5->red + tmp6->red + tmp8->red + tmp9->red)/6;
        dst->green = (tmp2->green + tmp3->green + tmp5->green + tmp6->green + tmp8->green + tmp9->green)/6;
        dst->blue = (tmp2->blue + tmp3->blue + tmp5->blue + tmp6->blue + tmp8->blue + tmp9->blue)/6;
    
        ++tmp1;
        ++tmp2;
        ++tmp3;
        ++tmp4;
        ++tmp5;
        ++tmp6;
        ++tmp7;
        ++tmp8;
        ++tmp9;
        ++dst;

        for(j = 2; j < dim; j++){
            
            //中间主体
            dst->red = (tmp1->red + tmp2->red + tmp3->red + tmp4->red + tmp5->red + tmp6->red + tmp7->red + tmp8->red + tmp9->red)/9;
            dst->green = (tmp1->green + tmp2->green + tmp3->green + tmp4->green + tmp5->green + tmp6->green + tmp7->green + tmp8->green + tmp9->green)/9;
            dst->blue = (tmp1->blue + tmp2->blue + tmp3->blue + tmp4->blue + tmp5->blue + tmp6->blue + tmp7->blue + tmp8->blue + tmp9->blue)/9;

            ++tmp1;
            ++tmp2;
            ++tmp3;
            ++tmp4;
            ++tmp5;
            ++tmp6;
            ++tmp7;
            ++tmp8;
            ++tmp9;
            ++dst;
        }
        
        //右侧
        dst->red = (tmp1->red + tmp2->red + tmp4->red + tmp5->red + tmp7->red + tmp8->red)/6;
        dst->green = (tmp1->green + tmp2->green + tmp4->green + tmp5->green + tmp7->green + tmp8->green)/6;
        dst->blue = (tmp1->blue + tmp2->blue + tmp4->blue + tmp5->blue + tmp7->blue + tmp8->blue)/6;

        ++tmp1;
        ++tmp2;
        ++tmp3;
        ++tmp4;
        ++tmp5;
        ++tmp6;
        ++tmp7;
        ++tmp8;
        ++tmp9;
        ++dst;
        
    }


    //左下角
    dst->red = (tmp2->red + tmp3->red + tmp5->red + tmp6->red)/4;
    dst->green = (tmp2->green + tmp3->green + tmp5->green + tmp6->green)/4;
    dst->blue = (tmp2->blue + tmp3->blue + tmp5->blue + tmp6->blue)/4;
    
    ++tmp1;
    ++tmp2;
    ++tmp3;
    ++tmp4;
    ++tmp5;
    ++tmp6;
    ++tmp7;
    ++tmp8;
    ++tmp9;
    ++dst;

    //下横线
    for(i = 2; i < dim; i++){

        dst->red = (tmp1->red + tmp2->red + tmp3->red + tmp4->red + tmp5->red + tmp6->red)/6;
        dst->green = (tmp1->green + tmp2->green + tmp3->green + tmp4->green + tmp5->green + tmp6->green)/6;
        dst->blue = (tmp1->blue + tmp2->blue + tmp3->blue + tmp4->blue + tmp5->blue + tmp6->blue)/6;
    
        ++tmp1;
        ++tmp2;
        ++tmp3;
        ++tmp4;
        ++tmp5;
        ++tmp6;
        ++tmp7;
        ++tmp8;
        ++tmp9;
        ++dst;

    }

    //右下角
    dst->red = (tmp1->red + tmp2->red + tmp4->red + tmp5->red)/4;
    dst->green = (tmp1->green + tmp2->green + tmp4->green + tmp5->green)/4;
    dst->blue = (tmp1->blue + tmp2->blue + tmp4->blue + tmp5->blue)/4;
    
}

/*
 * smooth - Your current working version of smooth. 
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
    naive_smooth(dim, src, dst);
}


/********************************************************************* 
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_smooth_functions() {
    add_smooth_function(&smooth, smooth_descr);
    add_smooth_function(&naive_smooth, naive_smooth_descr);
    /* ... Register additional test functions here */
}

