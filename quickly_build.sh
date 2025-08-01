cd cpp

#g++ main.cpp -o ../video_processor -fpermissive \
#    -Wl,-Bstatic -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_barcode -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_cvv -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_shape -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_ml -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_imgcodecs -lopencv_calib3d -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core \
#    -L/media/alex/b9bd9a1e-a4cc-4d3f-aae2-8c8c1fd468cb/Sources/opencv/build/lib \
#    -I/usr/include/opencv4 \
#    -Wl,-Bstatic -lz -lopenjp2 -lpng -ljpeg -lswscale \
#    -Wl,-Bdynamic -lglfw -lGLEW -lGL -lpthread -ldl -lavutil -lavcodec -lavformat
    #-Wl,-Bstatic -lwebp \

#cp ../video_processor ../electron/video_processor
#cp ../video_processor.exe ../electron/video_processor.exe
#cd ../electron
#npm run build
