cp ../common.c jni/src
cp ../my_app.c jni/src
cp -R ../assets/* assets/
ndk-build && ant debug
