del mpich2gcc.def
del mpich2gccd.def
del fmpich2gcc.def
del fmpich2gccd.def
Release\impgen.exe ..\lib\mpich2.dll > mpich2gcc.def
Release\impgen.exe ..\lib\mpich2d.dll > mpich2gccd.def
Release\impgen.exe ..\lib\fmpich2g.dll > fmpich2gcc.def
Release\impgen.exe ..\lib\fmpich2gd.dll > fmpich2gccd.def
dlltool --dllname mpich2.dll --def mpich2gcc.def --output-lib ..\lib\libmpich2.a
dlltool --dllname mpich2d.dll --def mpich2gccd.def --output-lib ..\lib\libmpich2d.a
dlltool --dllname fmpich2g.dll --def fmpich2gcc.def --output-lib ..\lib\libfmpich2g.a
dlltool --dllname fmpich2gd.dll --def fmpich2gccd.def --output-lib ..\lib\libfmpich2gd.a
del mpich2gcc.def
del mpich2gccd.def
del fmpich2gcc.def
del fmpich2gccd.def
