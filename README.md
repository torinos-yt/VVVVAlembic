# VVVV Alembic
Reading Alembic file(*.abc).  
Alembic : 1.7.12  
IlmBase : 2.2  
Support Platform : Only x64, Release   

include SlimDX.dll that built on visual studio 2015.  

## Alpha Version : Note
Currently only supports animation meshes with a single geometry with normals and UVs.  

It supports quad porigon and n-gons by triangulating when importing, but it is recommended to import a triangles only mesh.

Tested only .abc File by Houdini Rop Alembic Node.

## Installation
puts release package or built whole source repository in VVVV packs folder.