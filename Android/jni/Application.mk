# This needs to be defined to get the right header directories for egl / etc
APP_PLATFORM 	:= android-19

# This needs to be defined to avoid compile errors like:
# Error: selected processor does not support ARM mode `ldrex r0,[r3]'
APP_ABI 		:= armeabi-v7a


# use STL - note, can't seem to read env vars here....
APP_STL := gnustl_shared
