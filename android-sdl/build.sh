cp -R ../*.c jni/src
cp -R ../*.h jni/src
rm assets/*
cp -R ../assets/* assets/
ndk-build && ant debug
