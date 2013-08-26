echo update...
ping -n 10 127.0.0.1 >NUL
svn cleanup
svn update
dmx.exe
