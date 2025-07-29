cd cpp
g++ main.cpp -o ../video_processor -fpermissive \
    `pkg-config --static opencv4 --cflags --libs` \
    -lglfw -lGLEW -lGL -lpthread -ldl
cp ../video_processor ../electron/video_processor
#cp ../video_processor.exe ../electron/video_processor.exe
cd ../electron
npm run build