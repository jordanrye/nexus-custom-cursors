WMIC.EXE Alias /? >NUL 2>&1 || GOTO s_error

FOR /F "skip=1 tokens=1-6" %%G IN ('WMIC Path Win32_LocalTime Get Day^,Hour^,Minute^,Month^,Second^,Year /Format:table') DO (
   IF "%%~L"=="" goto s_done
      Set _yyyy=%%L
      Set _mm=%%J
      Set _dd=%%G
      Set _hour=%%H
      SET _minute=%%I
)
:s_done

echo #pragma once > src/version.h
echo #define V_MAJOR %_yyyy% >> src/version.h
echo #define V_MINOR %_mm% >> src/version.h
echo #define V_BUILD %_dd% >> src/version.h
SET /A var_res = %_hour% * 60 + %_minute%
echo #define V_REVISION %var_res% >> src/version.h