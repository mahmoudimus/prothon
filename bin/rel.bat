cd \prothon

del /F /S /Q win32\Prothon\pr\test\*.*
del /F /S /Q win32\Prothon\pr\pretest\*.*
rmdir win32\Prothon\pr\test
rmdir win32\Prothon\pr\pretest

rem del /F /S /Q win32\Prothon\pr\tutorial\*.*
rem rmdir win32\Prothon\pr\tutorial

del /F /S /Q win32\Prothon\pr\*.*
rmdir win32\Prothon\pr
del /F /S /Q win32\Prothon\*.*
rmdir win32\Prothon
del /F /S /Q win32\*.*
rmdir win32
mkdir win32\Prothon\pr\test
mkdir win32\Prothon\pr\pretest

rem mkdir win32\Prothon\pr\tutorial

copy src\release\prothon.exe 					win32\Prothon

copy modules\DBM\release\DBM.dll 				win32\Prothon
copy modules\File\release\File.dll 				win32\Prothon
copy modules\OS\release\OS.dll 					win32\Prothon
copy modules\Prosist\release\Prosist.dll		win32\Prothon
copy modules\Re\release\Re.dll 					win32\Prothon
copy modules\SQLite\release\SQLite.dll 			win32\Prothon

copy bin\rel.pth 								win32\Prothon
copy bin\msvcr71.dll 							win32\Prothon

copy readme.txt		 							win32\Prothon
copy changes.txt	 							win32\Prothon
copy LICENSE	 								win32\Prothon\LICENSE.txt

copy pr\test\*.pr 								win32\Prothon\pr\test
copy pr\pretest\*.pr 							win32\Prothon\pr\pretest

rem copy pr\tutorial\*.pr 						win32\Prothon\pr\tutorial

