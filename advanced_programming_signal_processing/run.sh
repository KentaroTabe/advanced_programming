#!/bin/sh
# imagemagickで何か画像処理をして，/imgprocにかきこみ，テンプレートマッチング
# 最終テストは，直下のforループを次に変更 for image in $1/final/*.ppm; do
for image in $1/test/*.ppm; do
    bname=`basename ${image}`
    name="imgproc/"$bname
    x=0    	#
    echo $name
    # convert "${image}" "${name}"  # 何もしない画像処理
#   convert -blur 2x6 "${image}" "${name}"
#   convert -median 3 "${image}" "${name}"
#   convert -auto-level "${image}" "${name}"
#   convert -equalize "${image}" "${name}"
    convert "${image}" -median 3 -equalize "${name}"
    rotation=0
    echo $bname:
    for template in $1/*.ppm; do
	echo `basename ${template}`
        for rotation in 0 90 180 270; do
            # 回転させたテンプレートの一時保存先
            rot_template="imgproc/tmp_rot_template.ppm"
            
            # convertの -rotate オプションでテンプレートを回転
            convert "${template}" -rotate $rotation "${rot_template}"
            
            if [ $x = 0 ]
            then
                ./matching $name "${rot_template}" $rotation 1.5 cp 
                x=1
            else
                ./matching $name "${rot_template}" $rotation 1.5 p 
            fi
        done
    done
    echo ""
done
wait
