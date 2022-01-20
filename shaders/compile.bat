for %%i in (*.spv) do (del /f /s /q %%i)
for %%i in (*.vert) do (C:\Lib\VulkanSDK\1.2.170.0\Bin32\glslc.exe %%i -o %%i.spv)
for %%i in (*.frag) do (C:\Lib\VulkanSDK\1.2.170.0\Bin32\glslc.exe %%i -o %%i.spv)
pause