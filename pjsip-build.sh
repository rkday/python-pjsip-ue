set +x
set -e
svn checkout http://svn.pjsip.org/repos/pjproject/tags/2.4.5/ pjproject
cd pjproject
./configure --prefix=$PWD/../root/
make dep
CFLAGS="-fPIC" make
make install
