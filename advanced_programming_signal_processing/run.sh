#!/bin/sh
# imagemagickで何か画像処理をして，/imgprocにかきこみ，テンプレートマッチング
# 最終テストは，直下のforループを次に変更 for image in $1/final/*.ppm; do
for image in $1/test/*.ppm; do
    bname=`basename ${image}`
    name="imgproc/"$bname
    x=0    	#
    echo $name
    convert "${image}" "${name}"  # 何もしない画像処理
#   convert -blur 2x6 "${image}" "${name}"
#   convert -median 3 "${image}" "${name}"
#   convert -auto-level "${image}" "${name}"
#   convert -equalize "${image}" "${name}"
#   convert -colorspace gray "${image}" "${name}"
#   convert -canny 0x1+10%+30% "${image}" "${name}"
    rotation=0
    echo $bname:
    for template in $1/*.ppm; do
        btemplate=`basename ${template}`
        templatename="imgproc/template/"$btemplate
        convert "${template}" -fuzz 5% -fill black -opaque black "${templatename}"
	    echo `basename ${template}`
	    if [ $x = 0 ]
	        then
	            ./matching $name $templatename $rotation 0.1 cp 
	            x=1
	        else
	            ./matching $name $templatename $rotation 0.1 p 
	        fi
    done
    echo ""
done
wait
