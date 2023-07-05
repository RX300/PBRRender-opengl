#
# Automatically generated file, do not edit!
#

"""pybind11 render api"""
from __future__ import annotations
import renderPyApi
import typing
import numpy
_Shape = typing.Tuple[int, ...]

__all__ = [
    "Camera",
    "PBRRender",
    "glmvec3"
]


class Camera():
    """
    Camera class
    """
    def GetCameraPtr(self) -> Camera: 
        """
        Get the pointer of camera
        """
    def __init__(self, position: glmvec3 = ..., up: glmvec3 = ..., yaw: float = -90.0, pitch: float = 0.0) -> None: 
        """
        Camera constructor
        """
    pass
class PBRRender():
    """
    A class of Renderer
    """
    def Init(self, arg0: int, arg1: int, arg2: str) -> None: 
        """
        Init the renderer
        """
    def LoadCamera(self, arg0: Camera) -> None: 
        """
        Load a camera
        """
    @staticmethod
    def LoadScene(*args, **kwargs) -> typing.Any: 
        """
        Load a scene
        """
    def ReadCurFrameBufferToNumpy(self) -> numpy.ndarray[numpy.uint8]: 
        """
        Read the current framebuffer to numpy array
        """
    def RenderTest(self) -> None: 
        """
        Render the scene
        """
    def RenderTestInit(self) -> None: 
        """
        Init the render test
        """
    def RenderTestShouldClose(self) -> bool: 
        """
        Check if the render test should close
        """
    def RenderTestUpdate(self) -> None: 
        """
        Update the render test
        """
    def __init__(self) -> None: ...
    pass
class glmvec3():
    """
    glm::vec3 class
    """
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0) -> None: 
        """
        glm::vec3 constructor
        """
    @property
    def x(self) -> float:
        """
        :type: float
        """
    @x.setter
    def x(self, arg0: float) -> None:
        pass
    @property
    def y(self) -> float:
        """
        :type: float
        """
    @y.setter
    def y(self, arg0: float) -> None:
        pass
    @property
    def z(self) -> float:
        """
        :type: float
        """
    @z.setter
    def z(self, arg0: float) -> None:
        pass
    pass
