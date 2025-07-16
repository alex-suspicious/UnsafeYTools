python3 generate.py
python3 shuffle.py $1
python3 sound.py temp_output.mp4 result.mp4
ffmpeg -i result.mp4 -stream_loop -1 -i noise.mp3 -filter_complex "[0:a][1:a]amix=inputs=2:duration=shortest[a]" -map 0:v -map "[a]" -c:v copy -c:a aac -b:a 192k output_video.mp4
rm result.mp4