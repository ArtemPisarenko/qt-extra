# Qt Extras
This repository contains miscelaneous extra additions and improvements for official [Qt5 framework](https://www.qt.io) essentials.  
They applied to installed Qt library and used directly in application code (so it doesn't required Qt source tree or its rebuilding in any way).  
All code is cross-platform unless explicitly stated otherwise.

Why wouldn't be better to contribute them to official Qt codebase ?  
At least for one of the reasons:
* too small to be enough valuable to kick my ass and went through formal process of contribution;
* ugly or otherwise hackish.
As long as Qt being continiously improved and extended, some part of this work may become irrelevant.

## Contents
All code grouped to modules, each having separate directory.  
Modules breaking generally follows Qt modules or features breaking.
For example, "core" module is relevant to pure QtCore functionality.

## Documentation
All documentation presented in source code in doxygen format.  
See *.md files and c++ header files in source tree.  

## Usage
Each module used by:
* including corresponding *.pri file to your qmake project (if it exists)
* and #including relevant headers in its directory (except for *_p.h).
All modules APIs are placed in "QtExtra" namespace.
Preprocessor macro names are prefixed with "QT" for brevity (hoping to avoid conflicting with Qt namespace).

## Limitations
Qt versions 5.7 or later are supported.  
Code have to be built with minimum "C++11" mode.  
Tested on linux x86_64 platform with gcc 5.4.0.  

