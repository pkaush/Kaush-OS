#This script is a meant to create the compilation environment over Cygwin
# Please have cygwin installed with (prerequisites)
#	1. GMP
#	2. MPFR
#	3. nasm
#	4. Native gcc and binutils
#	5. make, automake, autoconf ...


PREFIX=/usr/local/cross
TARGET=i386-kosj-elf



#binutils
mkdir build-binutils
cd build-binutils
../../binutils-2.20/configure --target=$TARGET --prefix=$PREFIX --disable-nls
make all
make install
cd ..


#gcc
mkdir build-gcc
cd build-gcc
export PATH=$PATH:$PREFIX/bin
../../gcc-4.5.0/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared 
make all-gcc
make install-gcc

make all-target-libgcc
make install-target-libgcc

cd ..

#newlib
mkdir build-newlib
cd build-newlib
../../newlib-1.19.0/configure --prefix=$PREFIX --target=$TARGET
make all install
cd ..
