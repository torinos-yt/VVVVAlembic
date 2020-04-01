# VVVV Alembic
Reading Alembic file(*.abc).  
Alembic : 1.7.12  
IlmBase : 2.2  
Support Platform : Only x64, Release  
Support Format : Ogawa

include SlimDX.dll that built on visual studio 2015.  

## Alpha Version : Note
Currently only supports animation meshes **with normals and UVs.**  

It supports quad porigon and n-gons by triangulating when importing, but it is recommended to import a triangles only mesh.

If export Alembic File using Cinema4D, Do not use the "Display Colors to Vertex Colors" option if it is not explicitly colored in the viewport.

## Installation   
puts release package or built whole source repository in VVVV packs folder.  

---
Base implementation : [@Perfume Dev Team](https://github.com/perfume-dev)  
#### [ofxAlembic](https://github.com/perfume-dev/ofxAlembic)  
Copyright (c) 2012 Perfume Dev Team. Under the MIT License. Please check the root of repository for details.