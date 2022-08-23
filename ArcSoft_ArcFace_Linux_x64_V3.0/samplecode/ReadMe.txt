ArcFace 3.0 SDK Linux_x64版本

ASFTestDemo使用说明：
1.Demo运行环境：
	Linux_x64系统；
	GLIBC 2.17及以上；
	GLIBCXX 3.4.19及以上；
	GCC 4.8.2及以上；
	cmake 3.0及以上；

2.执行过程：
	a).将ASFTestDemo工程拷贝到Linux系统下；
	b).需要将SDK包目录下中“lib”文件夹中的两个.so文件拷贝到/ASFTestDemo/linux_so文件目录下；
	c).建议将SDK包目录下中“inc”文件夹中的.h文件替换掉/ASFTestDemo/inc下的文件；
	d).下载SDK时，将从官网中获取的APPID/SDKKEY更新到samplecode.cpp文件中；
	e).在ASFTestDemo目录下新建一个build文件夹；（在build目录下编译）
	f).进入到/ASFTestDemo/build文件目录下,执行“cmake ..”命令，找到上一级的CMakeLists.txt文件编译，makefile文件会生成在build目录下；
	g).在/ASFTestDemo/build路径下执行“make”命令，生成可执行文件；
	h).在/ASFTestDemo/build路径下执行“./arcsoft_face_engine_test”命令,运行程序（./images文件夹下提供了三张用于测试的图片）；

3.注意事项：
	a).该demo提供图片为 NV21 格式的裸数据，图片保存在/ASFTestDemo/build/images路径下；
	b).图片宽度需要满足4的倍数，YUYV/I420/NV21/NV12/GRAY格式的图片高度为2的倍数，BGR24格式的图片高度不限制；
	c).jpg等格式读图需要做图像解码，推荐使用opencv第三方库进行读取；
	
