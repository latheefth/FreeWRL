/*
this version is for windows  vista,7,8 (see gdiPlusImageLoader.cpp forXP and lower )
*/



#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <wincodec.h>
#include <stdio.h>
#include <string.h>

extern "C"
{
#include "ImageLoader.h"
	//#include <GL/glew.h>
	//#ifdef GLEW_MX
	//GLEWContext * glewGetContext();
	//#endif
#include <config.h>
#include "display.h"
#include "opengl/textures.h"
}
/*
struct textureTableIndexStruct {
	struct X3D_Node*	scenegraphNode;
	int    nodeType;
	int    status;
	int    hasAlpha;
	GLuint OpenGLTexture;
	int    frames;
	char   *filename;
    int    x;
    int    y;
    unsigned char *texdata;
    GLint  Src;
    GLint  Trc;
};
*/
//static ULONG_PTR gdiplusToken;


//http://msdn.microsoft.com/en-us/library/windows/apps/ee690171.aspx 
//IWICBitmapSource(readonly)
//IWICBitmap(RW)
//wincode.h, windowscodecs.lib.dll - min support XPSP2
//WIC stands for Windows Imaging Component ..MSDN:
// http://msdn.microsoft.com/en-us/library/ee719902(v=vs.85).aspx
// http://archive.msdn.microsoft.com/win7wicsamples  WIC code gallery, win7+


template <typename T>
inline void SafeRelease(T *&p)
{
	if (NULL != p)
	{
		p->Release();
		p = NULL;
	}
}

IWICImagingFactory *m_pIWICFactory = NULL;
int initImageLoader()
{
  // GdiplusStartupInput gdiplusStartupInput;
   //GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	//fire up COM server
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	// Create WIC factory
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&m_pIWICFactory //IID_PPV_ARGS(&m_pIWICFactory)
		);

	return hr;
}
int shutdownImageLoader()
{
   //GdiplusShutdown(gdiplusToken);
	SafeRelease(m_pIWICFactory);
	m_pIWICFactory = NULL;
	return 0;
}

HRESULT ConvertBitmapSource(IWICBitmapSource *m_pOriginalBitmapSource, IWICBitmapSource **ppToRenderBitmapSource)
{
	*ppToRenderBitmapSource = NULL;
	UINT width, height;
	HRESULT hr = S_OK;

	// Get the client Rect
	hr = (m_pOriginalBitmapSource)->GetSize(&width, &height);

	if (SUCCEEDED(hr))
	{
		/*
		// Create a BitmapScaler
		IWICBitmapScaler *pScaler = NULL;
		hr = m_pIWICFactory->CreateBitmapScaler(&pScaler);
		// Initialize the bitmap scaler from the original bitmap map bits
		if (SUCCEEDED(hr))
		{
			hr = pScaler->Initialize(
				m_pOriginalBitmapSource,
				width, //rcClient.right - rcClient.left,
				height, //rcClient.bottom - rcClient.top,
				WICBitmapInterpolationModeFant);
		}
		*/

		IWICBitmapFlipRotator *pFlipper = NULL;
		hr = m_pIWICFactory->CreateBitmapFlipRotator(&pFlipper);
		// Initialize the bitmap scaler from the original bitmap map bits
		if (SUCCEEDED(hr))
		{
			hr = pFlipper->Initialize(
				m_pOriginalBitmapSource,
				WICBitmapTransformFlipVertical);
		}

		// Format convert the bitmap into 32bppBGR, a convenient 
		// pixel format for GDI rendering 
		if (SUCCEEDED(hr))
		{
			IWICFormatConverter *pConverter = NULL;

			hr = m_pIWICFactory->CreateFormatConverter(&pConverter);

			// Format convert to 32bppBGR
			if (SUCCEEDED(hr))
			{
				hr = pConverter->Initialize(
					pFlipper, //pScaler,                         // Input bitmap to convert
					GUID_WICPixelFormat32bppBGRA,     // Destination pixel format - I don't see ARGB. just BGRA or RGBA
					WICBitmapDitherTypeNone,         // Specified dither patterm
					NULL,                            // Specify a particular palette 
					0.f,                             // Alpha threshold
					WICBitmapPaletteTypeCustom       // Palette translation type
					);

				// Store the converted bitmap as ppToRenderBitmapSource 
				if (SUCCEEDED(hr))
				{
					hr = pConverter->QueryInterface(IID_PPV_ARGS(ppToRenderBitmapSource));
				}
			}

			SafeRelease(pConverter);
		}

		//SafeRelease(pScaler);
		SafeRelease(pFlipper);
	}

	return hr;
}

int loadImage(struct textureTableIndexStruct *tti, char *fname)
{
	if (!m_pIWICFactory) initImageLoader();

	// convert to wide char http://msdn.microsoft.com/en-us/library/ms235631(VS.80).aspx   
	//fname = "C:/source2/freewrl/freex3d/tests/helpers/brick.png";  
    //fname = "junk.jpg"; //test failure condition
	size_t origsize = strlen(fname) + 1;
	char* fname2 = (char*) malloc(origsize);
	strcpy(fname2,fname);
	for(unsigned int jj=0;jj<strlen(fname2);jj++)
		if(fname2[jj] == '/' ) fname2[jj] = '\\';

    const size_t newsize = 225;
    size_t convertedChars = 0;
    wchar_t wcstring[newsize];
    //mbstowcs_s(&convertedChars, wcstring, origsize, fname, _TRUNCATE);
#if _MSC_VER >= 1500
    mbstowcs_s(&convertedChars, wcstring, origsize, fname2, _TRUNCATE);
#else
    mbstowcs(wcstring, fname2, MB_CUR_MAX);
#endif

	free(fname2);

	IWICBitmapSource *m_pOriginalBitmapSource = NULL;

	HRESULT hr;
	IWICBitmapDecoder *pDecoder = NULL;

	// Step 2: Decode the source image to IWICBitmapSource

	// Create a decoder
	hr = m_pIWICFactory->CreateDecoderFromFilename(
		wcstring,                      // Image to be decoded
		NULL,                            // Do not prefer a particular vendor
		GENERIC_READ,                    // Desired read access to the file
		WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
		&pDecoder                        // Pointer to the decoder
		);

	IWICBitmapFrameDecode *pFrame = NULL;

	// Retrieve the first frame of the image from the decoder
	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pFrame);
	}

	// Retrieve IWICBitmapSource from the frame
	if (SUCCEEDED(hr))
	{
		SafeRelease(m_pOriginalBitmapSource);
		hr = pFrame->QueryInterface(
			IID_IWICBitmapSource,
			reinterpret_cast<void **>(&m_pOriginalBitmapSource));
	}

	IWICBitmapSource *pToRenderBitmapSource = NULL;

	// Step 3: Scale the original IWICBitmapSource to the client rect size
	// and convert the pixel format
	// except freewrl does its own scaling, so all I have to do is convert to ARGB, flip
	BOOL hasAlpha = TRUE;
	if (SUCCEEDED(hr))
	{
		//this pallet and pixelformat crap doesn't work for finding if image has alpha
		//- just hardwire all the images to hasAlpha =true
		//WICPixelFormatGUID pPixelFormat;
		//pToRenderBitmapSource->GetPixelFormat(&pPixelFormat);
		//hasAlpha = pPixelFormat & 
		//IWICPalette *pIPallet; 
		//hr = m_pIWICFactory->CreatePalette(&pIPallet);
		//m_pOriginalBitmapSource->CopyPalette(pIPallet);
		//pIPallet->HasAlpha(&hasAlpha);
		hr = ConvertBitmapSource(m_pOriginalBitmapSource, &pToRenderBitmapSource);
	}


	// http://msdn.microsoft.com/en-us/library/ee690179(v=vs.85).aspx  WICSource->CopyPixels()
	//HRESULT CopyPixels(
	//	[in, unique]  const WICRect *prc,
	//	[in]          UINT cbStride,
	//	[in]          UINT cbBufferSize,
	//	[out]         BYTE *pbBuffer
	//	);
	UINT width, height;
	hr = pToRenderBitmapSource->GetSize(&width, &height);
	int stride = width * 4;
	int totalbytes = stride * height; 
	unsigned char * blob = (unsigned char*)malloc(totalbytes);

	hr = pToRenderBitmapSource->CopyPixels(NULL, stride, totalbytes, blob); //NULL rectangle means take it all

	SafeRelease(pToRenderBitmapSource);
	SafeRelease(pDecoder);
	SafeRelease(pFrame);

   //deep copy data so browser owns it (and does its FREE_IF_NZ) and we can delete our copy here and forget about it
   tti->x = width;
   tti->y = height;
   tti->frames = 1;
   tti->texdata = blob; 
   if(!blob)
	   printf("ouch in %s L.%d - no image data\n",__FILE__,__LINE__);
   tti->hasAlpha = hasAlpha;

#ifdef verbose
   for(UINT row = 0; row < 23; ++row)
   {
      for(UINT col = 0; col < 5; ++col)
      {
         //printf("%x\n", *(UINT*)&(tti->texdata[(row * bitmapData->Stride / 4 + col)*tti->depth]));
         printf("%x\n", *(UINT*)&(tti->texdata[(row * tti->x + col)*4])); //tti->depth]));
      }
      printf("- - - - - - - - - - \n");
   }
#endif

   tti->filename = fname;
   if(0)
   {
	   shutdownImageLoader();
   }
   return 1;

}

