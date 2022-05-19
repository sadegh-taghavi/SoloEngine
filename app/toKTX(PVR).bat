@ECHO OFF
:loop

	FOR %%i in (%1) DO (
		set filepath=%%~di%%~pi
		set filename=%%~ni
		set fileextension=%%~xi
	)
	
	ECHO "C:\Imagination Technologies\PowerVR_Graphics\PowerVR_Tools\PVRTexTool\CLI\Windows_x86_64\PVRTexToolCLI.exe" -i %filepath%%filename%%fileextension% -o %filepath%%filename%.ktx -f r8g8b8a8 -m

	"C:\Imagination Technologies\PowerVR_Graphics\PowerVR_Tools\PVRTexTool\CLI\Windows_x86_64\PVRTexToolCLI.exe" -i %filepath%%filename%%fileextension% -o %filepath%%filename%.ktx -f r8g8b8a8 -m
	
	shift
	
if not "%~1"=="" goto loop

pause