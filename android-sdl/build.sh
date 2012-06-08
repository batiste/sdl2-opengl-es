cp -R ../*.c jni/src
cp -R ../*.h jni/src
cp -R ../assets/* assets/
ndk-build && ant debug
