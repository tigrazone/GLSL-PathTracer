
/***********************************************************************************
    Created:    17:9:2002
    FileName:     hdrloader.cpp
    Author:        Igor Kravtchenko
    
    Info:        Load HDR image and convert to a set of float32 RGB triplet.
************************************************************************************/

/* 
    This is a modified version of the original code. Addeed code to build marginal & conditional densities for IBL importance sampling
*/

#include "hdrloader.h"
#include "stb_image.h"

#include <memory.h>
#include <stdio.h>


static void RGBAtoRGB(float *image, int len, float *cols, int channels);


float Luminance(const Vec3 &c)
{
    return c.x*0.3f + c.y*0.6f + c.z*0.1f;
}

int LowerBound(const float* array, int lower, int upper, const float value)
{
    while (lower < upper)
    {
        int mid = lower + (upper - lower) / 2;

        if (array[mid] < value)
        {
            lower = mid + 1;
        }
        else
        {
            upper = mid;
        }
    }

    return lower;
}

void HDRLoader::buildDistributions(HDRData* res)
{
    int width  = res->width;
    int height = res->height;

    float *pdf2D = new float[width*height];
    float *cdf2D = new float[width*height];

    float *pdf1D = new float[height];
    float *cdf1D = new float[height];

    res->marginalDistData    = new Vec2[height];
    res->conditionalDistData = new Vec2[width*height];

    float colWeightSum = 0.0f;
	
	long jw = 0;
	long jw3 = 0;
	long i3;	
	
	float invSum;

    for (int j = 0; j < height; j++)
    {
        float rowWeightSum = 0.0f;

		i3 = 0;
        for (int i = 0; i < width; ++i)
        {
            float weight = Luminance(Vec3(res->cols[jw3 + i3], res->cols[jw3 + i3 + 1], res->cols[jw3 + i3 + 2]));

            rowWeightSum += weight;

            pdf2D[jw + i] = weight;
            cdf2D[jw + i] = rowWeightSum;
			
			i3 += 3;
        }
		
		invSum = 1.0f / rowWeightSum;

        /* Convert to range 0,1 */
        for (int i = 0; i < width; i++)
        {
            pdf2D[j*width + i] *= invSum;
            cdf2D[j*width + i] *= invSum;
        }

        colWeightSum += rowWeightSum;

        pdf1D[j] = rowWeightSum;
        cdf1D[j] = colWeightSum;
		
		jw += width;
		jw3 += width*3;
    }
	
	invSum = 1.0f / colWeightSum;
    
    /* Convert to range 0,1 */
    for (int j = 0; j < height; j++)
    {
        cdf1D[j] *= invSum;
        pdf1D[j] *= invSum;
    }

	float invHeight = 0.0f;
	invSum = 1.0f / (float)height;
	
    /* Precalculate row and col to avoid binary search during lookup in the shader */
    for (int i = 0; i < height; i++)
    {
        invHeight += invSum;
        int row = LowerBound(cdf1D, 0, height, invHeight);
        res->marginalDistData[i].x = row * invSum;
        res->marginalDistData[i].y = pdf1D[i];
    }

	float invWidth;
	invSum = 1.0f / (float)width;
	
	jw = 0;
	
    for (int j = 0; j < height; j++)
    {
		invWidth = 0.0f;
        for (int i = 0; i < width; i++)
        {
            invWidth += invSum;
            int col = LowerBound(cdf2D, jw, jw + width, invWidth) - jw;
            res->conditionalDistData[jw + i].x = col * invSum;
            res->conditionalDistData[jw + i].y = pdf2D[jw + i];
        }
		
		jw += width;
    }

    delete[] pdf2D;
    delete[] pdf1D;
    delete[] cdf2D;
    delete[] cdf1D;
}

HDRData* HDRLoader::load(const char *fileName)
{
    int width, height;
    float* image;
	
	const size_t buf_sz = 64*1024;
    char buf[buf_sz];	

	FILE *file;

    file = fopen(fileName, "rb");
    if (!file)
        return nullptr;

    HDRData* res = new HDRData;
	
	if(stbi_is_hdr_from_file(file))
	{
		fseek(file, 0, SEEK_SET);
		
		setvbuf(file, buf, _IOLBF, buf_sz);
		
		int num_channels;

		image = stbi_loadf_from_file(file, &width, &height, &num_channels, 4);

        size_t wh = width * height;

        res->width = width;
        res->height = height;

        printf("HDR width %d, height %d\n", width, height);

        float* cols = new float[wh * 3];

        res->cols = cols;

        printf("channels = %d\n", num_channels);

        RGBAtoRGB(image, wh, cols, num_channels > 3 ? num_channels : 4);

        buildDistributions(res);

        free(image);
    }
    else {
        fclose(file);
        return nullptr;
    }
	
    return res;
}



#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

HDRData* EXRLoader::load(const char *fileName)
{
  int width, height;
  float* image;
  const char* err = NULL;

  int ret = IsEXR(fileName);
  if (ret != TINYEXR_SUCCESS) {
    fprintf(stderr, "EXR: Header err. code %d\n", ret);
    return nullptr;
  }	

  EXRHeader header;
  EXRVersion version;
  
  ret = ParseEXRHeaderFromFile(&header, &version, fileName, &err);

  //image is RGBA
  ret = LoadEXR(&image, &width, &height, fileName, &err);
  if (ret != TINYEXR_SUCCESS) {
    if (err) {
      fprintf(stderr, "Load EXR err: %s(code %d)\n", err, ret);
    } else {
      fprintf(stderr, "Load EXR err: code = %d\n", ret);
    }
    FreeEXRErrorMessage(err);
    return nullptr;
  }

    HDRData* res = new HDRData;
	
	size_t wh = width * height;

    res->width = width;
    res->height = height;

	printf("EXR width %d, height %d\n", width, height);
	float *cols = new float[wh * 3];
	
    res->cols = cols;
	
	printf("channels = %d\n", header.num_channels);
	
	RGBAtoRGB(image, wh, cols, header.num_channels > 3 ? header.num_channels : 4);

    buildDistributions(res);
	
    free(image);
    return res;
}


void RGBAtoRGB(float *image, int len, float *cols, int channels)
{
    while (len-- > 0) {
        cols[0] = image[0];
        cols[1] = image[1];
        cols[2] = image[2];
        cols += 3;
        image += channels;
    }
}

