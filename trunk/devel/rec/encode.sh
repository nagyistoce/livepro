#mencoder tv:// -tv driver=v4l2:norm=NTSC:input=1:amode=0:width=640:height=480:device=/dev/video9:alsa:forceaudio:amode=0:adevice=hw.0,0 -fps 25 -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=6000 -vf crop=624:464:6:4,pp=lb -oac mp3lame -lameopts cbr:br=64:mode=3 -mc 2.0  -o myOutput.avi
ffmpeg -f video4linux -r 15 -s 240x180 -i /dev/video1 -f mp4 webcam1.mp4 &
ffmpeg -f video4linux -r 15 -s 240x180 -i /dev/video3 -f mp4 webcam2.mp4 &
ffmpeg -f video4linux -r 15 -s 240x180 -i /dev/video5 -f mp4 webcam3.mp4 &
ffmpeg -f video4linux -r 15 -s 240x180 -i /dev/video7 -f mp4 webcam4.mp4 &
#ssh 192.168.0.16 'sh /opt/livepro/devel/rec/encode.sh'

