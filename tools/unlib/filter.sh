cat LISTFILE.TXT | grep '^[^ ]' | grep -v '\.\.' | grep -o '^[A-Za-z_.]*' \
    | sort | sed 's/\(.*\)/lib OVERLAY.LIB *\1;/' | unix2dos >unpack.bat