mplayer tv:// -tv driver=v4l:width=320:height=240:device=/dev/video3 -v
#ffmpeg -t 10 -f video4linux -s 320x240 -r 30 -i /dev/video9 -f oss -i /dev/dsp -f mp4 webcam.mp4
#ffmpeg -t 10 -f video4linux -s 320x240 -r 30 -i /dev/video3 -f mp4 webcam.mp4
