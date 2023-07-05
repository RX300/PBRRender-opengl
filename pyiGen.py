import os, sys

# pyd所在路径
pyd_path = os.path.join(".","build","Release")
sys.path.append(pyd_path)
#打印搜素路径
print(sys.path)
module_name =  "renderPyApi"
print(module_name)
exec("import %s" % module_name)

from pybind11_stubgen import ModuleStubsGenerator

module = ModuleStubsGenerator(module_name)
module.parse()
module.write_setup_py = False

#让生成的pyi文件和pyd文件在同一目录下
pyi_path = os.path.join(pyd_path,module_name)
with open("%s.pyi" % pyi_path, "w") as fp:
    fp.write("#\n# Automatically generated file, do not edit!\n#\n\n")
    fp.write("\n".join(module.to_lines()))