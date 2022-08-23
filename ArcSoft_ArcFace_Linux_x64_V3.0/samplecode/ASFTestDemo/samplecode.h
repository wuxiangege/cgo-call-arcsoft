#ifndef __SAMPLECODE_H
#define __SAMPLECODE_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char*   feature;        // 人脸特征信息
    signed   int     featureSize;    // 人脸特征信息长度 
}CIMCAP_FaceFeature;

extern CIMCAP_FaceFeature CimcapFaceFeatureExtract(char* filename);
extern float CimcapFaceFeatureCompare(CIMCAP_FaceFeature outner, CIMCAP_FaceFeature inner);
extern void basic();

#ifdef __cplusplus  
}
#endif
#endif


