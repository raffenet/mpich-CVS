del mpich2gcc.def
del mpich2gccd.def
del mpich2gccf.def
del mpich2gccfd.def
Release\impgen.exe ..\lib\mpich2.dll > mpich2gcc.def
Release\impgen.exe ..\lib\mpich2d.dll > mpich2gccd.def
Release\impgen.exe ..\lib\mpich2fg.dll > mpich2gccf.def
Release\impgen.exe ..\lib\mpich2fgd.dll > mpich2gccfd.def
dlltool --dllname ..\lib\mpich2.dll --def mpich2gcc.def --output-lib ..\lib\libmpich2.a
dlltool --dllname ..\lib\mpich2d.dll --def mpich2gccd.def --output-lib ..\lib\libmpich2d.a
dlltool --dllname ..\lib\mpich2fg.dll --def mpich2gccf.def --output-lib ..\lib\libmpich2fg.a
dlltool --dllname ..\lib\mpich2fgd.dll --def mpich2gccfd.def --output-lib ..\lib\libmpich2fgd.a
del mpich2gcc.def
del mpich2gccd.def
del mpich2gccf.def
del mpich2gccfd.def
