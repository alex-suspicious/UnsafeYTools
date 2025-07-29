cd cpp

#g++ main.cpp -o ../video_processor -fpermissive \
#    -I/media/alex/Big\ Chungus1/Development/Sources/Chrome/new/opencv/modules/videoio/include \
#    `pkg-config --cflags --libs --static opencv4` \
#    -Wl,-Bstatic -lopencv_videoio \
#    -Wl,-Bdynamic -lglfw -lGLEW -lGL -lpthread -ldl

#cp ../video_processor ../electron/video_processor
#cp ../video_processor.exe ../electron/video_processor.exe
#cd ../electron
#npm run build