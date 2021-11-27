@echo off
7z u -stl -mx9 ClassicParentalControl-1.X.X-Win32.7z 		@_Release-Win32.lst
7z u -stl -mx9 ClassicParentalControl-1.X.X-Win32-PDB.7z 	_out\Win32\Release\*.pdb
