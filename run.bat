@echo off
python generate.py
python shuffle.py %1
python sound.py temp_output.mp4 result.mp4
del temp_output.mp4