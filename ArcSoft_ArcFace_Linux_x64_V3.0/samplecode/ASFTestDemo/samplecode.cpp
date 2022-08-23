#include <iostream>  
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <opencv2/opencv.hpp>

#include "arcsoft_face_sdk.h"
#include "amcomdef.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "samplecode.h"

using namespace std;
using namespace cv;

// 1年之后，如果这里过期，更换新鲜的APPID和SDKKEY
#define APPID "Go5Qxfp7Mx7rC9xw6PsvSDgHD6jiKPrdLWYjPXf5TPVC"
#define SDKKEY "3U2TKAfqs2wE51S6ysySwxAkzLQmgjMtoKYUBMHzvnKM"

#define NSCALE 16 
#define FACENUM	5

#define SafeFree(p) { if ((p)) free(p); (p) = NULL; }
#define SafeArrayDelete(p) { if ((p)) delete [] (p); (p) = NULL; } 
#define SafeDelete(p) { if ((p)) delete (p); (p) = NULL; } 

//时间戳转换为日期格式
void timestampToTime(char* timeStamp, char* dateTime, int dateTimeSize){
	time_t tTimeStamp = atoll(timeStamp);
	struct tm* pTm = gmtime(&tTimeStamp);
	strftime(dateTime, dateTimeSize, "%Y-%m-%d %H:%M:%S", pTm);
}

//图像颜色格式转换
int ColorSpaceConversion(MInt32 width, MInt32 height, MInt32 format, MUInt8* imgData, ASVLOFFSCREEN& offscreen){
	offscreen.u32PixelArrayFormat = (unsigned int)format;
	offscreen.i32Width = width;
	offscreen.i32Height = height;
	
	switch (offscreen.u32PixelArrayFormat)
	{
	case ASVL_PAF_RGB24_B8G8R8:
		offscreen.pi32Pitch[0] = offscreen.i32Width * 3;
		offscreen.ppu8Plane[0] = imgData;
		break;
	case ASVL_PAF_I420:
		offscreen.pi32Pitch[0] = width;
		offscreen.pi32Pitch[1] = width >> 1;
		offscreen.pi32Pitch[2] = width >> 1;
		offscreen.ppu8Plane[0] = imgData;
		offscreen.ppu8Plane[1] = offscreen.ppu8Plane[0] + offscreen.i32Height*offscreen.i32Width;
		offscreen.ppu8Plane[2] = offscreen.ppu8Plane[0] + offscreen.i32Height*offscreen.i32Width * 5 / 4;
		break;
	case ASVL_PAF_NV12:
	case ASVL_PAF_NV21:
		offscreen.pi32Pitch[0] = offscreen.i32Width;
		offscreen.pi32Pitch[1] = offscreen.pi32Pitch[0];
		offscreen.ppu8Plane[0] = imgData;
		offscreen.ppu8Plane[1] = offscreen.ppu8Plane[0] + offscreen.pi32Pitch[0] * offscreen.i32Height;
		break;
	case ASVL_PAF_YUYV:
	case ASVL_PAF_DEPTH_U16:
		offscreen.pi32Pitch[0] = offscreen.i32Width * 2;
		offscreen.ppu8Plane[0] = imgData;
		break;
	case ASVL_PAF_GRAY:
		offscreen.pi32Pitch[0] = offscreen.i32Width;
		offscreen.ppu8Plane[0] = imgData;
		break;
	default:
		return 0;
	}
	return 1;
}

//opencv方式裁剪图片
void CutIplImage(IplImage* src, IplImage* dst, int x, int y){
	CvSize size = cvSize(dst->width, dst->height);            //区域大小
	cvSetImageROI(src, cvRect(x, y, size.width, size.height));//设置源图像ROI
	cvCopy(src, dst);    //复制图像
	cvResetImageROI(src);//源图像用完后，清空ROI
}


// 单人脸特征提取
CIMCAP_FaceFeature CimcapFaceFeatureExtract(char* filename) {
	// 激活SDK
	printf("\n************* ArcFace SDK Info *****************\n"); 
	MRESULT res = MOK;
	ASF_ActiveFileInfo activeFileInfo = { 0 };
	res = ASFGetActiveFileInfo(&activeFileInfo);
	if (res != MOK)
	{
		printf("ASFGetActiveFileInfo fail: %ld\n", res);
	}
	else{
		//这里仅获取了有效期时间，还需要其他信息直接打印即可
		char startDateTime[32];
		timestampToTime(activeFileInfo.startTime, startDateTime, 32);
		printf("startTime: %s\n", startDateTime);
		
		char endDateTime[32];
		timestampToTime(activeFileInfo.endTime, endDateTime, 32);
		printf("endTime: %s\n", endDateTime);
	}
	
	const ASF_VERSION version = ASFGetVersion();
	printf("\nVersion:%s\n", version.Version);
	printf("BuildDate:%s\n", version.BuildDate);
	printf("CopyRight:%s\n", version.CopyRight);
	printf("OpenCV Version:%s\n", CV_VERSION); // 打印OpenCV版本号

	printf("\n************* Face Recognition *****************\n");	
	res = ASFOnlineActivation(MPChar(APPID), MPChar(SDKKEY));
	if (MOK != res && MERR_ASF_ALREADY_ACTIVATED != res){
		printf("ASFOnlineActivation fail: %ld\n", res);
	}
	else{
		printf("ASFOnlineActivation sucess: %ld\n", res);
	}
	
	// 引擎初始化
	MHandle handle = NULL;
	MInt32 mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_LIVENESS | ASF_IR_LIVENESS;
	res = ASFInitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_ONLY, NSCALE, FACENUM, mask, &handle);
	if (res != MOK){
		printf("ASFInitEngine fail: %ld\n", res);
	}
	else{
		printf("ASFInitEngine sucess: %ld\n", res);
	}
	
	// 读取照片(绝对路径)
	IplImage* originalImg = cvLoadImage(filename); 
	
	// 切割照片尺寸以适应4的倍数
	IplImage* img = cvCreateImage(cvSize(originalImg->width - originalImg->width % 4, originalImg->height), IPL_DEPTH_8U, originalImg->nChannels);
	CutIplImage(originalImg, img, 0, 0);
	
	// 人脸特征提取
	if (img) {
		ASF_MultiFaceInfo detectedFaces1 = { 0 }; // 默认照片中只有1个人
		ASF_SingleFaceInfo SingleDetectedFaces1 = { 0 };
		ASF_FaceFeature feature = { 0 };
		CIMCAP_FaceFeature copyfeature = { 0 };
		res = ASFDetectFaces(handle, img->width, img->height, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)img->imageData, &detectedFaces1);
		if (MOK == res)
		{
			SingleDetectedFaces1.faceRect.left = detectedFaces1.faceRect[0].left;
			SingleDetectedFaces1.faceRect.top = detectedFaces1.faceRect[0].top;
			SingleDetectedFaces1.faceRect.right = detectedFaces1.faceRect[0].right;
			SingleDetectedFaces1.faceRect.bottom = detectedFaces1.faceRect[0].bottom;
			SingleDetectedFaces1.faceOrient = detectedFaces1.faceOrient[0];
			
			//单人脸特征提取
			res = ASFFaceFeatureExtract(handle, img->width, img->height, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)img->imageData, &SingleDetectedFaces1, &feature);
			if (res == MOK){
				copyfeature.featureSize = feature.featureSize;
				copyfeature.feature = (unsigned char *)malloc(feature.featureSize);
				
				memset(copyfeature.feature, 0, feature.featureSize);                
				memcpy(copyfeature.feature, (unsigned char *)feature.feature, feature.featureSize); // 在上层GO应用中销毁

				// 反初始化
				res = ASFUninitEngine(handle);
				if (res != MOK){
					printf("ASFUninitEngine fail: %d\n", res);
				}else{
					printf("ASFUninitEngine sucess: %d\n", res);    
				}           
				return copyfeature;
			}
			else {
				printf("ASFFaceFeatureExtract 1 fail: %ld\n", res);
			}
		}
	}
	
	cvReleaseImage(&img); 
	cvReleaseImage(&originalImg); 
}

// 人脸特征比对，输出比对相似度
float CimcapFaceFeatureCompare(CIMCAP_FaceFeature outner, CIMCAP_FaceFeature inner) {
	printf("\n************* ArcFace SDK Info *****************\n");
	MRESULT res = MOK;
	ASF_ActiveFileInfo activeFileInfo = { 0 };
	res = ASFGetActiveFileInfo(&activeFileInfo);
	if (res != MOK)
	{
		printf("ASFGetActiveFileInfo fail: %ld\n", res);
	}
	else
	{
		//这里仅获取了有效期时间，还需要其他信息直接打印即可
		char startDateTime[32];
		timestampToTime(activeFileInfo.startTime, startDateTime, 32);
		printf("startTime: %s\n", startDateTime);
		char endDateTime[32];
		timestampToTime(activeFileInfo.endTime, endDateTime, 32);
		printf("endTime: %s\n", endDateTime);
	}

	//SDK版本信息
	const ASF_VERSION version = ASFGetVersion();
	printf("\nVersion:%s\n", version.Version);
	printf("BuildDate:%s\n", version.BuildDate);
	printf("CopyRight:%s\n", version.CopyRight);
	printf("OpenCV Version:%s\n", CV_VERSION);

	printf("\n************* Face Recognition *****************\n");	
	res = ASFOnlineActivation(MPChar(APPID), MPChar(SDKKEY));
	if (MOK != res && MERR_ASF_ALREADY_ACTIVATED != res){
		printf("ASFOnlineActivation fail: %ld\n", res);
	}else{
		printf("ASFOnlineActivation sucess: %ld\n", res);
	}
	
	//初始化引擎
	MHandle handle = NULL;
	MInt32 mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_LIVENESS | ASF_IR_LIVENESS;
	res = ASFInitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_ONLY, NSCALE, FACENUM, mask, &handle);
	if (res != MOK){
		printf("ASFInitEngine fail: %ld\n", res);
	}else{
		printf("ASFInitEngine sucess: %ld\n", res);
	}
	
	// 外部特征值重组
	ASF_FaceFeature outnerFeature = { 0 };
	outnerFeature.featureSize = outner.featureSize;
	outnerFeature.feature = (MByte *)malloc(outner.featureSize);
	memset(outnerFeature.feature, 0, outner.featureSize);                
	memcpy(outnerFeature.feature, (MByte *)outner.feature, outner.featureSize); 
				
	// 内部特征值重组
	ASF_FaceFeature innerFeature = { 0 };
	innerFeature.featureSize = inner.featureSize;
	innerFeature.feature = (MByte *)malloc(inner.featureSize);
	memset(innerFeature.feature, 0, inner.featureSize);                
	memcpy(innerFeature.feature, (MByte *)inner.feature, inner.featureSize); 
	
	// 特征值对比
	MFloat confidenceLevel;
	res = ASFFaceFeatureCompare(handle, &outnerFeature, &innerFeature, &confidenceLevel);
	if (res != MOK){
		printf("ASFFaceFeatureCompare fail: %ld\n", res);
	}else{
		printf("ASFFaceFeatureCompare sucess: %lf\n", confidenceLevel);
		return (float)confidenceLevel;
	}
	
	// 销毁
	SafeFree(outnerFeature.feature)
	SafeFree(innerFeature.feature)
	
	// 反初始化
	res = ASFUninitEngine(handle);
	if (res != MOK){
		printf("ASFUninitEngine fail: %ld\n", res);
	}else{
		printf("ASFUninitEngine sucess: %ld\n", res);	
	}
}

// 基础例子
void basic(){
	// 激活SDK
	printf("\n************* ArcFace SDK Info *****************\n");
	MRESULT res = MOK;
	ASF_ActiveFileInfo activeFileInfo = { 0 };
	res = ASFGetActiveFileInfo(&activeFileInfo);
	if (res != MOK){
		printf("ASFGetActiveFileInfo fail: %ld\n", res);
	}
	else{
		//这里仅获取了有效期时间，还需要其他信息直接打印即可
		char startDateTime[32];
		timestampToTime(activeFileInfo.startTime, startDateTime, 32);
		printf("startTime: %s\n", startDateTime);
		char endDateTime[32];
		timestampToTime(activeFileInfo.endTime, endDateTime, 32);
		printf("endTime: %s\n", endDateTime);
	}

	//SDK版本信息
	const ASF_VERSION version = ASFGetVersion();
	printf("\nVersion:%s\n", version.Version);
	printf("BuildDate:%s\n", version.BuildDate);
	printf("CopyRight:%s\n", version.CopyRight);
	printf("OpenCV Version:%s\n", CV_VERSION);

	printf("\n************* Face Recognition *****************\n");	
	res = ASFOnlineActivation(MPChar(APPID), MPChar(SDKKEY));
	if (MOK != res && MERR_ASF_ALREADY_ACTIVATED != res){
		printf("ASFOnlineActivation fail: %ld\n", res);
	}
	else{
		printf("ASFOnlineActivation sucess: %ld\n", res);
	}
	
	//初始化引擎
	MHandle handle = NULL;
	MInt32 mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE | ASF_LIVENESS | ASF_IR_LIVENESS;
	res = ASFInitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_ONLY, NSCALE, FACENUM, mask, &handle);
	if (res != MOK)
		printf("ASFInitEngine fail: %ld\n", res);
	else
		printf("ASFInitEngine sucess: %ld\n", res);
	
	//读取2张测试照片
	IplImage* originalImg1 = cvLoadImage("/etc/4.jpg"); //测试照片的存放位置
	IplImage* originalImg2 = cvLoadImage("/etc/4.jpg");
	
	//切割成以适应4的倍数
	//切割第1张
	IplImage* img1 = cvCreateImage(cvSize(originalImg1->width - originalImg1->width % 4, originalImg1->height), IPL_DEPTH_8U, originalImg1->nChannels);
	CutIplImage(originalImg1, img1, 0, 0);
	//切割第2张
	IplImage* img2 = cvCreateImage(cvSize(originalImg2->width - originalImg2->width % 4, originalImg2->height), IPL_DEPTH_8U, originalImg2->nChannels);
	CutIplImage(originalImg2, img2, 0, 0);
	
	if (img1 && img2)
	{
		// 第一张人脸数据提取
		ASF_MultiFaceInfo detectedFaces1 = { 0 };//多人脸信息；
		ASF_SingleFaceInfo SingleDetectedFaces1 = { 0 };
		ASF_FaceFeature feature1 = { 0 };
		ASF_FaceFeature copyfeature1 = { 0 };
		res = ASFDetectFaces(handle, img1->width, img1->height, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)img1->imageData, &detectedFaces1);
		if (MOK == res)
		{
			SingleDetectedFaces1.faceRect.left = detectedFaces1.faceRect[0].left;
			SingleDetectedFaces1.faceRect.top = detectedFaces1.faceRect[0].top;
			SingleDetectedFaces1.faceRect.right = detectedFaces1.faceRect[0].right;
			SingleDetectedFaces1.faceRect.bottom = detectedFaces1.faceRect[0].bottom;
			SingleDetectedFaces1.faceOrient = detectedFaces1.faceOrient[0];
			
			//单人脸特征提取(需要单独摘出来)
			res = ASFFaceFeatureExtract(handle, img1->width, img1->height, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)img1->imageData, &SingleDetectedFaces1, &feature1);
			if (res == MOK)
			{
				//拷贝feature
				copyfeature1.featureSize = feature1.featureSize;
				copyfeature1.feature = (MByte *)malloc(feature1.featureSize);
				
				memset(copyfeature1.feature, 0, feature1.featureSize);                //特征值长度
				memcpy(copyfeature1.feature, feature1.feature, feature1.featureSize); //特征值
			}
			else
				printf("ASFFaceFeatureExtract 1 fail: %ld\n", res);
		}
		
		// 第二张人脸数据提取
		ASF_MultiFaceInfo	detectedFaces2 = { 0 };
		ASF_SingleFaceInfo SingleDetectedFaces2 = { 0 };
		ASF_FaceFeature feature2 = { 0 };
		res = ASFDetectFaces(handle, img2->width, img2->height, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)img2->imageData, &detectedFaces2);
		if (MOK == res)
		{
			SingleDetectedFaces2.faceRect.left = detectedFaces2.faceRect[0].left;
			SingleDetectedFaces2.faceRect.top = detectedFaces2.faceRect[0].top;
			SingleDetectedFaces2.faceRect.right = detectedFaces2.faceRect[0].right;
			SingleDetectedFaces2.faceRect.bottom = detectedFaces2.faceRect[0].bottom;
			SingleDetectedFaces2.faceOrient = detectedFaces2.faceOrient[0];
 
			res = ASFFaceFeatureExtract(handle, img2->width, img1->height, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)img1->imageData, &SingleDetectedFaces2, &feature2);
			if (MOK != res)
				printf("ASFFaceFeatureExtract 2 fail: %ld\n", res);
		}
		else
			printf("ASFDetectFaces 2 fail: %ld\n", res);
		
		// 第1张 vs 第2张， 人脸特征比对(需要单独摘出来)
		MFloat confidenceLevel;
		res = ASFFaceFeatureCompare(handle, &copyfeature1, &feature2, &confidenceLevel);
		if (res != MOK)
			printf("ASFFaceFeatureCompare fail: %ld\n", res);
		else
			printf("ASFFaceFeatureCompare sucess: %lf\n", confidenceLevel);
		
		//反初始化
		res = ASFUninitEngine(handle);
		if (res != MOK)
			printf("ASFUninitEngine fail: %ld\n", res);
		else
			printf("ASFUninitEngine sucess: %ld\n", res);
	}
	else
	{
		printf("No pictures found.\n");
	}
	
	cvReleaseImage(&img1); 
	cvReleaseImage(&originalImg1); 
	cvReleaseImage(&img2); 
	cvReleaseImage(&originalImg2); 
	
	getchar();
}

