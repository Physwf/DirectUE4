# DirectUE4

UE4渲染框架学习作品，基于4.20;

### 截图

![image](https://github.com/Physwf/resources/blob/master/screenshot/DirectUE4.jpg)

### 功能列表

#### 1.管线  
* PrePass  
* ShadowPass(Dyanamic Point Light Only)  
* BasePass  
* Lights  
    ~~NoneShadowLights~~  
    ~~IndirectLight~~  
    ShadowedLight  
* Atmosphere
* Postprocessing  
    TAA  
    Tonemapping  

#### 2.顶点工厂
* LocalVertexFactory  
* GPUSkinVertexFactory  
* ~~LandscapeVertexFactory~~ 
* ~~ParticalxxxVertexFactory~~ 

#### 3.Shader编译
#### 4.Uniform管理
#### 5.默认材质

### 使用

1.依赖项安装:
  * DirectXTex https://github.com/microsoft/DirectXTex.git
  * fbxSDK(2019.0)  

2.设置环境变量:   

    DirectXTexDir=D:/DirectXTex/  
    FBX_SDK_HOME=C:\Program Files\Autodesk\FBX\FBX SDK\2019.0 

3.编译代码:  

    $ git clone https://github.com/Physwf/DirectUE4.git
    $ cd DirectUE4  
    $ mkdir build
    $ cd build
    $ cmake ..
    Open DirectUE4.sln
    Set DirectUE as Startup Project
    Set Work Directory=$(OutDir)
    Ctrl+Shift+B

4.资源安装  

    $ git clone https://github.com/Physwf/resources.git
    $ copy resources/dx11demo to DirectUE4/build/Debug
    $ copy resources/Mannequin to DirectUE4/build/Debug
    $ copy resources/Primitives to DirectUE4/build/Debug
    
5.run
