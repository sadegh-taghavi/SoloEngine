@ECHO OFF
:loop

	FOR %%i in (%1) DO (
		set filepath=%%~di%%~pi
		set filename=%%~ni
		set fileextension=%%~xi
	)
	
	ECHO toktx --genmipmap %filepath%%filename%.ktx %filepath%%filename%%fileextension%

	toktx --genmipmap %filepath%%filename%.ktx %filepath%%filename%%fileextension%
	
	shift
	
if not "%~1"=="" goto loop

pause