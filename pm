#!/system/bin/sh
if [ "$1" == "path" ]; then
	if [ "$2" == "com.ss.android.ugc.aweme" ]; then
		echo "package:/data/local/tmp/dy105.apk"	
	else
		exec /system/bin/pm_ori $@
	fi
else
	exec /system/bin/pm_ori $@
fi

