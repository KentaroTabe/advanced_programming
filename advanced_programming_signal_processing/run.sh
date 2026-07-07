#!/bin/sh
# imagemagickで何か画像処理をして，/imgprocにかきこみ，テンプレートマッチング
# 最終テストは，直下のforループを次に変更 for image in $1/final/*.ppm; do
for image in $1/test/*.ppm; do
    bname=`basename ${image}`
    name="imgproc/"$bname
    x=0    	#
    echo $name
#   convert  "${image}" "${name}"  # 何もしない画像処理
#   convert -blur 2x6 "${image}" "${name}"
#   convert  -median 1 -equalize  "${image}" "${name}"
#   convert -auto-level "${image}" "${name}"
    convert -contrast "${image}" "${name}"
    for size in 50 100 200; 
    do
        per="%"
        size_str="${size}%"
        echo "${size}"
    rotation=0
    echo $bname:
    for template in $1/*.ppm; do
	echo `basename ${template}`
    tname=`basename ${template}`
    resize_template="imgproc/"$tname
    #limit=$(echo "1.5 * 100 / ${size}" |bc)
    if [ $size -lt 100 ]
    then
    convert -median 2 -scale ${size_str} "${template}" "${resize_template}"
    if [ $x = 0 ]
	then
	    ./matching $name "${resize_template}" $rotation 1.5 cp 
	    x=1
	else
	    ./matching $name "${resize_template}" $rotation 1.5 p 
	fi
    else
    convert -scale ${size_str} "${template}" "${resize_template}"
    fi
	if [ $x = 0 ]
	then
	    ./matching $name "${resize_template}" $rotation 0.5 cp 
	    x=1
	else
	    ./matching $name "${resize_template}" $rotation 0.5 p 
	fi
    done
    echo ""
done
done
wait
