cd \prothon

del /F /S /Q win32\Prothon\pr\test\*.*
rmdir win32\Prothon\pr\test
del /F /S /Q win32\Prothon\pr\tutorial\*.*
rmdir win32\Prothon\pr\tutorial
del /F /S /Q win32\Prothon\pr\*.*
rmdir win32\Prothon\pr
del /F /S /Q win32\Prothon\*.*
rmdir win32\Prothon
del /F /S /Q win32\*.*
rmdir win32
mkdir win32\Prothon\pr\test
mkdir win32\Prothon\pr\tutorial

copy src\debug\prothon.exe 						win32\Prothon

copy modules\DBM\debug\DBM.dll 					win32\Prothon
copy modules\File\debug\File.dll 				win32\Prothon
copy modules\OS\debug\OS.dll 					win32\Prothon
copy modules\Prosist\debug\Prosist.dll			win32\Prothon
copy modules\Re\debug\Re.dll 					win32\Prothon
copy modules\SQLite\debug\SQLite.dll 			win32\Prothon

copy bin\rel.pth 								win32\Prothon
copy bin\msvcr71.dll 							win32\Prothon

copy readme.txt		 							win32\Prothon
copy changes.txt	 							win32\Prothon
copy LICENSE	 								win32\Prothon\LICENSE.txt

copy pr\test\*.pr 								win32\Prothon\pr\test
copy pr\tutorial\*.pr 							win32\Prothon\pr\tutorial

