# -g    添加gdb调试选项
# -o    指定生成可执行文件的名称
# -fPIC 表明使用地址无关代码
# -I    i的大写，include头文件非标准库中存在的也不是在当前文件夹下的，需要将地址用-i(大写)包含
# -L    用到的函数或操作非标准库下的，需要将库存在的地址用-L包含，库文件一般为.a或.so文件
# -l    用在-L之后，后加库名称，-l库名(.a文件名)
# 
# #!/bin/bash
# g++ -g -shared -fPIC -I./ -I./inc -Llinux_so -larcsoft_face_engine -larcsoft_face `pkg-config opencv --libs --cflags opencv` samplecode.cpp -o libsamplecode.so
#

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

