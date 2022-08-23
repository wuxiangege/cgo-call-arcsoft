# cgo-call-arcsoft

cgo 调用[虹软视觉](https://ai.arcsoft.com.cn/ucenter/resource/build/index.html#/login?redirect=https%3A%2F%2Fai.arcsoft.com.cn%2Fucenter%2Fresource%2Fbuild%2Findex.html%23%2Fapplication) sdk，实现人脸识别

# 我的动态库架构

根据业务场景，自己裁剪 libsamplecode.cpp 代码，制作成 so 供上层调用。如图所示
![image.png](https://github.com/wuxiangege/cgo-call-arcsoft/blob/main/img/1.png)

# 环境搭建

说明，因为虹软 sdk 的 linux 版是 cpp 写的，所以需要 gcc，glibc 的支持；又因为本 sdk 依赖 opencv,所以还需要事先安装好 opecv 和 cmake。但我在包装 so 的时候，使用的是 make, 因为本人更熟练。

- 编译器 GCC 4.8.2 及以上
- 库依赖 GLIBC 2.17 及以上
- 库依赖 GLIBCXX 3.4.19 及以上
- cmake-3.18.0-rc2.tar.gz
- 操作系统 CentOS Linux release 7.6.1810 (Core)

## 安装 cmake

注意虹软文档里的推荐版本号

```shell
tar xvf cmake-3.18.0-rc2.tar.gz
cd cmake-3.18.0-rc2.tar
./configure
gmake
make install
cmake --version
```

## 安装 opencv

这里会遇到墙的问题，可以参考大神的解决方案，[源码编译 OpenCV 卡在 ippicv](https://joveh-h.blog.csdn.net/article/details/102362725?spm=1001.2101.3001.6650.4&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-4-102362725-blog-109458079.pc_relevant_multi_platform_featuressortv2dupreplace&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-4-102362725-blog-109458079.pc_relevant_multi_platform_featuressortv2dupreplace&utm_relevant_index=5)

```shell
wget https://codeload.github.com/opencv/opencv/zip/3.4.10
unzip opencv-3.4.10.zip
cd opencv-3.4.10
mkdir build
cd build
cmake ..
make
make install
```

## 创建动态链接库文件

这样我们制作应用的时候，系统就可以自动找到它

```shell
cd /etc/ld.so.conf.d
touch opencv.conf
echo "/usr/local/lib64" > opencv.conf //opencv的so

echo "/usr/local/lib/linux_so" > asf.conf //二次封装后的so
```

## 设置 pc 文件的搜索路径

```shell
vi /etc/profile
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig
```

## 设计 Makefile

```makefile
CXX      := g++
CXXFLAGS := `pkg-config opencv --libs --cflags opencv`
INCLUDE  := -I./inc
LIBPATH  := -Llinux_so -larcsoft_face_engine -larcsoft_face
SHARE    := -Wall -g -fPIC -shared -o
OBJS     := samplecode.cpp
TARGET   := libsamplecode.so

all:$(TARGET)
$(TARGET):$(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(INCLUDE) $(LIBPATH) $(SHARE) $(TARGET)

.PHONY: clean
clean:
	rm -f *.o *.so
```

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
