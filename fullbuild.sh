set -e

function bold { tput bold; }
function unbold { tput rmso; }
function status { bold; echo $1; unbold; }

# build util
#cd util
#status "[-] Building Utilities..."
#make > /dev/null
#cd ..

# build igor and data for windows
cd stage
status "[-] Building igor..."
python stage.py > /dev/null
cd ..

# build dl for mac and windows
cd dl
status "[-] Building dl (device link)..."
make > /dev/null
cd ..

# build mac gui
cd gui
status "[-] Building Mac GUI..."
xcodebuild > /dev/null
cd ..

# build windows gui
cd wingui
status "[-] Building Win32 GUI..."
./everything.sh > /dev/null
make > /dev/null
cd ..
