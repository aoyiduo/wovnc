echo "init vc env"
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
echo "init qt env"
call "C:\Qt\Qt5.6.3\5.6.3\msvc2013\bin\qtenv2.bat"

set path_script=%~dp0
echo %path_script%

cd %path_script%
lrelease %path_script%vncserver_en.ts -qm %path_script%vncserver_en.qm
lrelease %path_script%vncserver_zh.ts -qm %path_script%vncserver_zh.qm
pause