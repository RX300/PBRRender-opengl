import renderPyApi as Render
import time
from PIL import Image
import numpy as np
import torch as th
testRender = Render.PBRRender()
testRender.Init(1920, 1080,"test")
camera = Render.Camera(Render.glmvec3(0.0, 0.0, 3.0))
testRender.LoadCamera(camera)
testRender.RenderTestInit()
while(testRender.RenderTestShouldClose()!=True):
    testRender.RenderTestUpdate()
print('\n')
start_time = time.time()
img = testRender.ReadCurFrameBufferToNumpy()
elapsed_time = time.time() - start_time
print(f"读取帧缓冲数据耗时: {elapsed_time} 秒")

# # 使用PyTorch修改帧缓冲数据为红色
# start_time = time.time()
# img_tensor = th.from_numpy(img)
# img_tensor[:,:,0]=255
# img_tensor[:, :, 1:3] = 0  # 将G、B通道置为0
# elapsed_time = time.time() - start_time
# print(f"使用PyTorch修改帧缓冲数据: {elapsed_time} 秒")
#
# # 将tensor转成numpy
# start_time = time.time()
# modified_img = img_tensor.numpy()
# elapsed_time = time.time() - start_time
# print(f"将tensor转成numpy: {elapsed_time} 秒")

# 将帧缓冲数据转换为PyTorch张量并移动到GPU上
start_time = time.time()
img_tensor = th.from_numpy(img).to('cuda')
elapsed_time = time.time() - start_time
print(f"将帧缓冲数据转换为PyTorch张量并移动到GPU上: {elapsed_time} 秒")

# 使用PyTorch在GPU上修改帧缓冲数据
start_time = time.time()
img_tensor[:,:,0] = 255
img_tensor[:,:,1:3] = 0
elapsed_time = time.time() - start_time
print(f"使用PyTorch在GPU上修改帧缓冲数据: {elapsed_time} 秒")

# 将张量转换回NumPy数组
start_time = time.time()
modified_img = img_tensor.cpu().numpy()
elapsed_time = time.time() - start_time
print(f"将张量转换回NumPy数组: {elapsed_time} 秒")

start_time = time.time()
image = Image.fromarray(np.array(modified_img))
elapsed_time = time.time() - start_time
print(f"创建图像对象耗时: {elapsed_time} 秒")

image.save('output.png')
image.show()