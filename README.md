# cgo-call-arcsoft

cgo 调用[虹软视觉](https://ai.arcsoft.com.cn/ucenter/resource/build/index.html#/login?redirect=https%3A%2F%2Fai.arcsoft.com.cn%2Fucenter%2Fresource%2Fbuild%2Findex.html%23%2Fapplication) sdk，实现人脸识别

# 我的动态库架构

根据业务场景，自己裁剪 libsamplecode.cpp 代码，制作成 so 供上层调用。如图所示
![image.png](https://github.com/wuxiangege/cgo-call-arcsoft/blob/main/img/1.png)

# 应用

```go
package main

/*
#cgo CFLAGS: -I. -Iinc
#cgo LDFLAGS: -Llinux_so -lsamplecode -lstdc++
#include <stdlib.h>
#include "samplecode.h"
*/
import "C"

import (
	"encoding/base64"
	"fmt"
	"runtime"
	"time"
	"unsafe"
)

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())
	for i := 0; i < 1000000; i++ {
		CimcapFaceFeatureExtract()
		fmt.Printf("-------%d++++++++++++\n", i)
	}
}

// CimcapFaceFeatureExtract 思可普人脸特征提取
func CimcapFaceFeatureExtract() {
	pic := C.CString("./images/hjh.jpg")
	defer C.free(unsafe.Pointer(pic)) //销毁

	// 特征值提取
	start := time.Now()
	f := C.CimcapFaceFeatureExtract(pic)
	fmt.Printf("difference = %v\n", time.Now().Sub(start))
	defer C.free(unsafe.Pointer(f.feature)) //销毁

	// 人脸特征信息长度
	featureSize := int32(f.featureSize)
	fmt.Printf("人脸特征信息长度:%d\n", featureSize)

	// 人脸特征信息
	feature := C.GoBytes(unsafe.Pointer(f.feature), C.int(featureSize))
	enc := base64.StdEncoding.EncodeToString(feature)
	fmt.Printf("人脸特征信息=[%s]\n", enc)
}

// ASFLibCallSample 虹软视觉库调用示例
func ASFLibCallSample() {
	// 功能1:特征值获取
	// 传入一张照片
	pic := C.CString("./images/hjh.jpg")
	defer C.free(unsafe.Pointer(pic)) //销毁

	// 特征值提取
	start := time.Now()
	f := C.CimcapFaceFeatureExtract(pic)
	fmt.Printf("difference = %v\n", time.Now().Sub(start))
	defer C.free(unsafe.Pointer(f.feature)) //销毁

	// 人脸特征信息长度
	featureSize := int32(f.featureSize)
	fmt.Printf("人脸特征信息长度:%d\n", featureSize)

	// 人脸特征信息
	feature := C.GoBytes(unsafe.Pointer(f.feature), C.int(featureSize))
	enc := base64.StdEncoding.EncodeToString(feature)
	fmt.Printf("人脸特征信息=[%s]\n", enc)

	// 功能2:对比两张人脸
	// 对比两张照片
	var outner C.CIMCAP_FaceFeature
	outner.featureSize = C.int(f.featureSize)
	outner.feature = (*C.uchar)(unsafe.Pointer(&feature[0]))
	confidenceLevel := float32(C.CimcapFaceFeatureCompare(outner, outner))
	fmt.Printf("可信度等级=%f\n", confidenceLevel)
}
```

![image.png](https://github.com/wuxiangege/cgo-call-arcsoft/blob/main/img/2.png)
