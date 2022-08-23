# cgo-call-arcsoft
cgo调用虹软视觉sdk，实现人脸识别

# 我的动态库架构
根据业务场景，自己裁剪libsamplecode.cpp代码，制作成so供上层调用。如图所示
![image.png](https://github.com/wuxiangege/cgo-call-arcsoft/blob/main/img/1.png)