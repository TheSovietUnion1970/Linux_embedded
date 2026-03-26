
# $HOME/.login* or similar files may first set $DEBUGINFOD_URLS.
# If $DEBUGINFOD_URLS is not set there, we set it from system *.url files.
# $HOME/.*rc or similar files may then amend $DEBUGINFOD_URLS.
# See also [man debuginfod-client-config] for other environment variables
# such as $DEBUGINFOD_MAXSIZE, $DEBUGINFOD_MAXTIME, $DEBUGINFOD_PROGRESS.

if (! $?DEBUGINFOD_URLS) then
    set prefix="/home/vinh/Yocto/poky/build-vinh/tmp/work/all-poky-linux/alsa-topology-conf/1.2.5.1-r0/recipe-sysroot-native/usr"
    set debuginfod_urls=`sh -c "cat /home/vinh/Yocto/poky/build-vinh/tmp/work/all-poky-linux/alsa-topology-conf/1.2.5.1-r0/recipe-sysroot-native/etc/debuginfod/*.urls 2>/dev/null" | tr '\n' ' '`
    if ( "$debuginfod_urls" != "" ) then
        setenv DEBUGINFOD_URLS "$debuginfod_urls"
    endif
    unset debuginfod_urls
    unset prefix
endif
