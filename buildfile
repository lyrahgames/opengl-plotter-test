cxx.std = latest
using cxx
cxx{*}: extension = cpp
hxx{*}: extension = hpp

import libs = glfw3%lib{glfw3}
import libs += glbinding%lib{glbinding}
import libs += glm%lib{glm}

exe{main}: {hxx cxx}{**} $libs

cxx.poptions =+ "-I$src_base"