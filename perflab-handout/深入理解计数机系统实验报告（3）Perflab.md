## 前言

这个实验应该是对应着课程视频中的 [CSAPP 深入理解计算机系统 Lecture 10 Program Optimization](https://www.bilibili.com/video/BV1iW411d7hd?p=10) 说实话这样的一个重要的话题用一堂课的时间讲解未免有些许捉襟见肘

在这堂课上老爷子主要讲解了程序的常数级优化（即写出更适合计算机运行的程序）中的两个点：

1. 访问数据应该要尽量满足程序的局部性原理，也就是需要根据计算机金字塔式的存储模型去设计程序；

   举例：二维数组的行列访问书序颠倒后其性能差距主要是因为程序没有很好的空间局部性

2. 减少每次循环的计算量，把循环中的常用值作为常数保存

   举例：遍历字符串的时候使用`for(i = 0; i < strlen(s); i++)`这样的方式将程序的复杂度从线性降低到平方复杂度

这两个优化可以算是最为基础且最为重要的两点优化原则，但是面对Perllab实验想要达到的极致的优化，这样的优化方案还略显幼稚

> 实验文档：[简书 PerfLab 文档中文翻译](https://www.jianshu.com/p/4b1e741dfcb8)
>
> 参考资料：[CSAPP：PerfLab实验](https://blog.csdn.net/qq_41140987/article/details/105329531)

## 实验准备

这个实验的背景和要求都相对简单，阅读过实验文档后就能大致明白本次实验的目标是：通过所学到的底层知识对`kernels.c`文档中的`naive_rotate()`和`naive_smooth()`进行常数级优化使得编译出的程序更加适宜于机器运行。

同样这个实验也需要经常进行编译和清除上一次的编译产物，和`Datelab`一样，我们直接编写一个脚本`run.sh`集成这些命令（不要忘了添加可执行权限）

```bash
#/bin/bash
make clean
make driver
./driver
```



## 正式实验

原`naive_rotate()`

```c
void naive_rotate(int dim, pixel *src, pixel *dst) {
int i, j;
for(i=0; i < dim; i++)
for(j=0; j < dim; j++)
dst[RIDX(dim-1-j,i,dim)] = src[RIDX(i,j,dim)];
}
```



原`naive_smooth()`

```c
void naive_smooth(int dim, pixel *src, pixel *dst) {
int i, j;
for(i=0; i < dim; i++)
for(j=0; j < dim; j++)
dst[RIDX(i,j,dim)] = avg(dim, i, j, src);
}
```



重要的宏定义`RIDX(i,j,n)`

```c
#define RIDX(i,j,n) ((i)*(n)+(j))
```



这个宏定义实际上就是通过一维数组模拟二维数组，`grp[RIDX(i,j,n)]`其实就相当于访问了列长为第第二维大小为n的`grp[i][j]`

### rotate 优化一

对于`naive_rotate()`由于`dst`和`src`的行列访问的顺序天然相反的无法直接通过调整`i j`的顺序改变，而`naive_smooth`的行列访问是正确的并且能够合理利用`catch`中存取的内容，所以访问顺序没有优化的空间。

而另一个比较重要的点就是处理循环中的重复的计算，可以很明显的发现对于`naive_rotate`和`naive_smooth`来说都在`RIDX(i,j,n)`这个宏定义上出现了明显的重复计算，并且由于乘法操作所消耗的时钟周期要明显大于加减法消耗的时钟周期，我们可以利用循环的规律性将循环中进行的乘法转换为循环中进行的累加，所以得到了第一阶段的优化

```c
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    int dst_ridx, src_ridx;
    const int end_dst = (dim-1)*dim;
    dst_ridx = RIDX(dim-1, 0, dim), src_ridx = RIDX(0, 0, dim);

    for (i = 0; i < dim; i++){
        dst_ridx = end_dst+i;
        for (j = 0; j < dim; j++){
            dst[dst_ridx] = src[src_ridx];
            dst_ridx -= dim;
            src_ridx ++; 
        }   
    }   
}
```



第一阶段得到的分数为

```bash
Rotate: 4.8 (naive_rotate: Naive baseline implementation)
```

（注意这里的得分和运行的机器有关，同样的代码可能在您的机器上跑出来的分数会略有不同）

到这里，我能想到的优化已经结束了 ╮(╯▽╰)╭ 

下面优化二对于一个初涉性能优化的萌新来讲确实非常难能凭空想出，优化二只能现学现卖写啦

### rotate 优化二

将矩阵分块处理，对每个小块进行`rotate`操作

根据大佬的讲解这样做事为了让每一个小块都能让访问集中在一级缓存中就能实现访问提速的效果，实验中取了`BLOKSIZE = 8, BLOKSIZE = 16, BLOKSIZE = 32`这几种情况，当`BLOKSIZE = 16`时性能最佳（应该与实验所用的主机老旧 一级catch容量小有关）

然后大佬的优化还涉及到了一个访问顺序的改变，将`dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];`转化为`dst[RIDX(i, j, dim)] = src[RIDX(j, dim-i-1, dim)];`这里利用到了减少内循环中变化次数最大的`j`的计算次数来达到计算优化的目的

这两重优化再加上利用循环的将乘法运算转化为累加实现

最后`naive_rotate()`：

```c
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j, k, r;
    int BLOKSIZE = 16; 
    int dim1 = dim-1;
    int dst_base = 0, src_base;
    int dst_RIDX, src_RIDX;
    int BLOKSIZE_DIM = BLOKSIZE*dim;

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
```



最终得分

```bash
Rotate: 6.4 (rotate: Current working version)
```



### smooth 优化一

和前面的`rotate`优化一样，这里比较显然的就是将`RIDX()`的乘法转换为循环时的叠加以减少乘法器的使用

这样就能得到的第一版优化方案

```c
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    int dst_RIDX = 0;

    for (i = 0; i < dim; i++){
        for (j = 0; j < dim; j++){
            dst[dst_RIDX] = avg(dim, i, j, src);
            ++dst_RIDX;
        }   
    }   
}
```

优化得分为：

```bash
Smooth: 5.2 (naive_smooth: Naive baseline implementation)
```



### smooth 优化二

实验文档中有一段比较明显的提示：

> (Note:The functionavgis a local function and you can get rid of it altogether to implement smooth in some other way.)
>
> （注：**avg** 是一个本地函数，你可以完全弃之不用，以其他方法实现 **smooth** 。）

虽然这样是不符合面向对象的尽量将简化每一个方法所需要完成的任务，但是这毕竟是性能优化实验加之这个实验本身是面向过程的设计思路，所以这也是情有可原的😳

根据过去的编程经验，写出了第一次优化：

```c
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    const int dx[8] = {-1, -1, 1, 1, 0, 1, 0, -1};
    const int dy[8] = {-1, 1, -1, 1, 1, 0, -1, 0}; 
    
    int i, j, k;
    int dst_RIDX = 0;
    int new_i, new_j;
    int new_r, new_g, new_b;
    int tmp_RIDX;
    int count;

    for (i = 0; i < dim; i++){
        for (j = 0; j < dim; j++){
            new_r = src[dst_RIDX].red;
            new_g = src[dst_RIDX].green;
            new_b = src[dst_RIDX].blue;
            count = 1;

            for(k = 0; k < 8; k++){
                new_i = i+dx[k];
                new_j = j+dy[k];
                if(dim <= new_i || new_i < 0 || dim <= new_j || new_j < 0) continue;                tmp_RIDX = RIDX(new_i, new_j, dim);
                new_r += src[tmp_RIDX].red;
                new_g += src[tmp_RIDX].green;
                new_b += src[tmp_RIDX].blue;
                ++count;
            }

            dst[dst_RIDX].red = (unsigned short)(new_r/count);
            dst[dst_RIDX].green = (unsigned short)(new_g/count);
            dst[dst_RIDX].blue = (unsigned short)(new_b/count);

            ++dst_RIDX;
        }
    }
}
```



优化结果：

```bash
Smooth: 4.2 (smooth: Current working version)
```



### smooth 优化三

第二次优化的结果显然是不太理想的，甚至优化后会比优化前的效率更低

简单分析后可以发现，在判断访问是否越界的代码中`if(dim <= new_i || new_i < 0 || dim <= new_j || new_j < 0) continue;`使用了过多的判断在占多数的未超边界的访问中，这个拉低程序效率的幅度过大了导致整体的效率不达标

故将程序进一步调整，将四周特殊的部分与中间部分分离来开考虑

为了进一步地提升程序的效率，我们可以直接选用指针来指示待操作的像素点

然后选择9个像素点代表着我们待操作的像素点，这9个像素点不一定都有意义。这9个像素点为一组单位进行操作并且一起移向下一个位置（有点类似于掩码的想法 屏蔽掉其他的像素点只聚焦在待操作的9个像素点上）

![perflab_Smooth演示图](https://gitee.com/kong_lingyun/img_bed/raw/master/img/perflab_Smooth%E6%BC%94%E7%A4%BA%E5%9B%BE.png)

```c
void naive_smooth(int dim, pixel *src, pixel *dst)
{
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
```



为了极致地提高函数执行的效率，一些本应该从这个方法中剔除的作为单独的一个函数或者作为内联函数（比如待操作的像素点整体的移动）都直接由手工输入实现了（真的反人类😂 特别是在部分写错的情况下还要去逐个修改），优化这个函数的确是可以练习在`vim`下批量删改替换的技巧（goutou🤣）推荐

不过这点辛苦相对于这次的性能提高来说是相当值得的，最后的优化结果：

```bash
Smooth: 14.6 (smooth: Current working version)
```



## 实验总结

`Perllab`相对于`Datelab`和`Bomblab`而言任务量稍小，但是更加面向实际应用

实验最能提高程序效率的就是合理利用`catch`，经过本次实验也恰恰最能锻炼合理利用`catch`写出更加适宜于机器进行的程序

实验在`rotate`优化时的分块的想法确实很巧妙，通过这次实验也算是掌握了一种常数级优化的策略

> 文章作者: 扶明
> 文章链接: https://alden-elias.github.io/2022/02/12/%E6%B7%B1%E5%85%A5%E7%90%86%E8%A7%A3%E8%AE%A1%E6%95%B0%E6%9C%BA%E7%B3%BB%E7%BB%9F%E5%AE%9E%E9%AA%8C%E6%8A%A5%E5%91%8A%EF%BC%883%EF%BC%89Perflab/
> 版权声明: 本博客所有文章除特别声明外，均采用 CC BY-NC-SA 4.0 许可协议。转载请注明来自 扶明の小站！
